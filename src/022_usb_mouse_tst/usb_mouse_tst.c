#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <libusb-1.0/libusb.h>
#include "common_util.h"


static bool keep_running = true;

static void exit_signal_func(int sig)
{
    LOG_INFO("Enter with signal %d", sig);

    keep_running = false;

}

void print_usage()
{
    printf("usage: ./usb_mouse_tst [-sync|-async] [-t%%d]");
    printf("[-sync|-async]: transfer mode.\n");
    printf("[-t%%d]: timeout time, unit is second.\n");
}

/**
 * 
 * usage:
 *  ./usb_mouse_tst [-sync|-async] [-t%%d]
 * 
 * [-sync|-async]：表示要使用同步传输还是异步传输。
 * [-t%%d]：表示传输时等待超时的时间，单位为秒。
 * 
 *  exp:
 *  
 *  ./usb_mouse_tst
 *  ./usb_mouse_tst -sync -t3
 * 
*/
int main(int argc, const char **argv)
{
    bool is_success = false;
    int err = 0, extend_idx = 1, timeout = 5, num_devices = 0, i = 0, j = 0, intf_num = -1;
    struct libusb_context *ctx = NULL;
    struct libusb_device **device_list, *dev = NULL;
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *cfg_desc = NULL;
    const struct libusb_interface_descriptor *intf_desc = NULL;
    const struct libusb_endpoint_descriptor *eq_desc = NULL;
    struct libusb_device_handle *dev_handle = NULL;
    char transfer_mode = 0; // 0 means sync, 1 means async
    unsigned char endpoint = 0, transfer_buf[8] = {0};
    int transferred;
    bool is_found = false;

    LOG_INFO("Enter with argc %d", argc);

    while (extend_idx < argc - 1) {
        if (!strcmp(argv[extend_idx], "-sync")) {
            // position
            transfer_mode = 0;
        } else if (!strcmp(argv[extend_idx], "-async")) {
            // position
            transfer_mode = 1;
        } else if (strstr(argv[extend_idx], "-t")) {
            if (sscanf(argv[extend_idx], "-t%d", &timeout) < 1) {
                LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
                timeout = 5;
            }
        } else {
            print_usage();
            LOG_INFO("Ignore unsupport option: %s.", argv[extend_idx]);
        }
        extend_idx++;
    }
    LOG_INFO("Transfer mode: %s, timeout %ds", transfer_mode ? "async" : "sync", timeout);


    signal(SIGINT, exit_signal_func);
    signal(SIGILL, exit_signal_func);
    signal(SIGSEGV, exit_signal_func);
    signal(SIGTERM, exit_signal_func);
    signal(SIGABRT, exit_signal_func);

    // 初始化libusb: libusb_init <--> libusb_exit
    err = libusb_init(&ctx);
    if (err) {
        LOG_ERR("libusb initialization failed: %s", libusb_strerror(err));
        return EXCUTE_FAILED_EXIT;
    }
    LOG_INFO("libusb_init ok");

    // 获取设备列表：libusb_get_device_list <--> libusb_free_device_list
    num_devices = libusb_get_device_list(ctx, &device_list);
    if (num_devices < 0) {
		LOG_ERR("could not enumerate USB devices: %s", libusb_strerror(num_devices));
		goto out_libusb_exit;
    }
    LOG_INFO("get %d usb devices", num_devices);

    // 遍历设备列表
    for (i = 0; i < num_devices; i++) {
        dev = device_list[i];
        // 获取设备描述符: libusb_get_device_descriptor
        err = libusb_get_device_descriptor(dev, &dev_desc);
        if (err) {
            LOG_ERR("could not get device descriptor for device %d: %s", i, libusb_strerror(err));
            continue;
        }
        // bNumConfigurations、iSerialNumber
        LOG_INFO("There is %d configs in 0x%x devices",
                dev_desc.bNumConfigurations, dev_desc.iSerialNumber);

        // 遍历设备下配置的列表，获取配置描述符: libusb_get_config_descriptor <--> libusb_free_config_descriptor
        for (j = 0; j < dev_desc.bNumConfigurations; j++) {
            err = libusb_get_config_descriptor(dev, j, &cfg_desc);
            if (err) {
                LOG_ERR("could not get config descriptor!");
                libusb_free_config_descriptor(cfg_desc);
                cfg_desc = NULL;
                continue;
            }

            // 遍历当前配置下的所有接口
            for (int intf = 0; intf < cfg_desc->bNumInterfaces; intf++) {
                // 遍历当前接口的所有setting
                for (int set = 0; set < cfg_desc->interface->num_altsetting; set++) {
                    intf_desc = &cfg_desc->interface->altsetting[set];

                    // 判断当前接口的当前setting是否符合鼠标的配置
                    if (intf_desc->bInterfaceClass != 3 || intf_desc->bInterfaceProtocol != 2) {
                        LOG_ERR("Mismatch setting: class=%d, protocol=%d!", intf_desc->bInterfaceClass, intf_desc->bInterfaceProtocol);
                        libusb_free_config_descriptor(cfg_desc);
                        cfg_desc = NULL;
                        continue;
                    }

                    // 找到鼠标
                    intf_num = intf_desc->bInterfaceNumber;
                    LOG_INFO("found usb mouse interface %d", intf_num);

                    //  遍历鼠标接口下的端点
                    for (int eq = 0; eq < intf_desc->bNumEndpoints; eq++) {
                        eq_desc = &intf_desc->endpoint[eq];
                        if ((eq_desc->bmAttributes & 0x3) == LIBUSB_TRANSFER_TYPE_INTERRUPT
                            && (eq_desc->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) {
                            // 找到中断输入的端点
                            endpoint = eq_desc->bEndpointAddress;
                            is_found = true;
                            goto found_case;
                        }
                    }
                }
            }
        }
    }

found_case:
    if (!is_found || !dev || !cfg_desc || intf_num < 0) {
        LOG_ERR("Could not found usb mouse interface!");
        goto out_dev_list_free;
    }
    LOG_INFO("Found usb mouse, interface number=%d, endpoint=0x%x.", intf_num, endpoint);
    // 打开端点：libusb_open <--> libusb_close
    err = libusb_open(dev, &dev_handle);
    if (err) {
        LOG_ERR("could not get config descriptor!");
        goto out_config_free;
    }

    // 卸载内核驱动：libusb_set_auto_detach_kernel_driver
    err |= libusb_set_auto_detach_kernel_driver(dev_handle, 1);

    // 声明当前接口: libusb_claim_interface <--> libusb_release_interface
    err |= libusb_claim_interface(dev_handle, intf_num);
    if (err) {
        LOG_ERR("claim interface failed!");
        goto out_libusb_close;
    }

    // 传输循环
    while (keep_running) {
        // 中断传输: libusb_interrupt_transfer
        err = libusb_interrupt_transfer(dev_handle, endpoint, transfer_buf, sizeof(transfer_buf),
                                    &transferred, timeout * 1000);
        // 判断并打印返回值
        if (!err || err == LIBUSB_ERROR_OVERFLOW) {
            printf("%d bytes transferred(err=%d): ", transferred, err);
            for (int i = 0; i < transferred && i < 8; i++) { // 最多只打印8个字节的数据
                printf("0x%x ", transfer_buf[i]);
            }
            printf("\n");
            is_success = true;
        } else if (err == LIBUSB_ERROR_TIMEOUT) {
            LOG_ERR("libusb_interrupt_transfer timeout!");
            continue;
        } else {
            LOG_ERR("libusb_interrupt_transfer failed!");
            is_success = false;
            goto out_release_intf;
        }
    }

    // 资源释放
    LOG_INFO("The end, release resources!");
out_release_intf:
    if (dev_handle && intf_num >= 0)
        libusb_release_interface(dev_handle, intf_num);

out_libusb_close:
    if (dev_handle)
        libusb_close(dev_handle);

out_config_free:
    if (cfg_desc)
        libusb_free_config_descriptor(cfg_desc);

out_dev_list_free:
    if (device_list)
        libusb_free_device_list(device_list, true);

out_libusb_exit:
    if (ctx)
        libusb_exit(ctx);
    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
