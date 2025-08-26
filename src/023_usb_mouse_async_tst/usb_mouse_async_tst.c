#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include "common_util.h"


#define SYNC_USB_TRNSFER (0)
#define ASYNC_USB_TRNSFER (1)

#define MOUSE_BUF_LEN (16)
struct mouse_desc {
    int timeout; // uint: s
    char buf[MOUSE_BUF_LEN];
    int transferred;
    struct libusb_device_handle *dev_handle;
    bool is_claimed;
    char endpoint;
    int intf_num;
    struct mouse_desc *next;
    pthread_t sync_pid;

    struct libusb_transfer *async_transfer;
};

static bool keep_running = true;
static struct libusb_context *g_ctx;
static struct libusb_device **g_device_list;
static struct mouse_desc *g_mouses = NULL;
static int g_completed;

static int libusb_comb_init(struct libusb_context **ctx, struct libusb_device ***device_list)
{
    struct libusb_context *tmp_ctx = NULL;
    struct libusb_device **tmp_dev_list = NULL;
    int num_dev, err = 0;

    LOG_INFO("Enter");
    // 初始化libusb: libusb_init <--> libusb_exit
    err = libusb_init(&tmp_ctx);
    if (err) {
        LOG_ERR("libusb initialization failed: %s", libusb_strerror(err));
        return -1;
    }
    LOG_INFO("libusb_init ok");

    // 获取设备列表：libusb_get_device_list <--> libusb_free_device_list
    num_dev = libusb_get_device_list(tmp_ctx, &tmp_dev_list);
    if (num_dev < 0) {
        LOG_ERR("could not enumerate USB devices: %s", libusb_strerror(num_dev));
        libusb_exit(tmp_ctx);
        return -1;
    }

    *ctx = tmp_ctx;
    *device_list = tmp_dev_list;
    return num_dev;
}

static void libusb_comb_free(struct libusb_context *ctx, struct libusb_device **device_list)
{
    LOG_INFO("Enter");
    if (device_list)
        libusb_free_device_list(device_list, true);
    if (ctx)
        libusb_exit(ctx);
}

static void free_mouses(struct mouse_desc *mouses)
{
    struct mouse_desc *node = NULL;
    int free_cnt = 0;

    LOG_INFO("Enter!");
    while (mouses) {
        node = mouses;
        mouses = mouses->next;

        if (node->dev_handle)
            libusb_close(node->dev_handle);
        free(node);
    }
}

static int search_mouses(struct mouse_desc **mouses, struct libusb_device **device_list, int num_dev)
{
    int err = 0, i = 0, j = 0;
    struct libusb_device *dev = NULL;
    struct libusb_device_descriptor dev_desc;
    struct libusb_config_descriptor *cfg_desc = NULL;
    const struct libusb_interface_descriptor *intf_desc = NULL;
    const struct libusb_endpoint_descriptor *eq_desc = NULL;
    struct libusb_device_handle *dev_handle = NULL;
    unsigned char endpoint = 0, transfer_buf[8] = {0};
    int intf_num = -1, mouse_cnt = 0;
    struct mouse_desc *mouse = NULL, *list_head = NULL;

    *mouses = NULL;
    LOG_INFO("Searching in %d usb devices", num_dev);
    for (i = 0; i < num_dev; i++) {
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
                continue;
            }

            // 遍历当前配置下的所有接口
            LOG_INFO("\tThere is %d intefaces in config", cfg_desc->bNumInterfaces);
            for (int intf = 0; intf < cfg_desc->bNumInterfaces; intf++) {
                // 遍历当前接口的所有setting
                LOG_INFO("\t\tThere is %d settings in inteface", cfg_desc->interface->num_altsetting);
                for (int set = 0; set < cfg_desc->interface->num_altsetting; set++) {
                    intf_desc = &cfg_desc->interface->altsetting[set];

                    // 判断当前接口的当前setting是否符合鼠标的配置
                    if (intf_desc->bInterfaceClass != 3 || intf_desc->bInterfaceProtocol != 2) {
                        LOG_WARN("\t\tMismatch setting: class=%d, protocol=%d!", intf_desc->bInterfaceClass, intf_desc->bInterfaceProtocol);
                        continue;
                    }

                    // 找到鼠标
                    intf_num = intf_desc->bInterfaceNumber;
                    //  遍历鼠标接口下的端点
                    LOG_INFO("\t\t\tThere is %d settings in mouse %d", intf_desc->bNumEndpoints, intf_num);
                    for (int eq = 0; eq < intf_desc->bNumEndpoints; eq++) {
                        eq_desc = &intf_desc->endpoint[eq];
                        if ((eq_desc->bmAttributes & 0x3) == LIBUSB_TRANSFER_TYPE_INTERRUPT
                            && (eq_desc->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) {
                            // 找到中断输入的端点
                            endpoint = eq_desc->bEndpointAddress;
                        } else {
                            LOG_WARN("\t\t\tMismatch endpoint: type=%d, addr=%s!", eq_desc->bmAttributes, eq_desc->bEndpointAddress);
                            continue;
                        }

                        // 打开终端端点
                        err = libusb_open(dev, &dev_handle);
                        if (err) {
                            LOG_ERR("libusb_open failed!");
                            continue;
                        }
                        if (!dev || !cfg_desc || intf_num < 0) {
                            LOG_ERR("Could not found usb mouse interface!");
                            continue;
                        }

                        // 分配、填充、链接一个mouse结构体
                        mouse = calloc(1, sizeof(* mouse));
                        if (!mouse) {
                            LOG_ERR("Alloc mouse memory failed!");
                            err = -1;
                            goto search_end; // 没有内存时直接退出，不再进行搜索
                        }
                        mouse->intf_num = intf_num;
                        mouse->endpoint = endpoint;
                        mouse->dev_handle = dev_handle;

                        mouse->next = list_head;
                        list_head = mouse;
                        mouse_cnt++;
                    }
                }
            }

            // 每遍历完一个config_descripto, 先释放掉
            libusb_free_config_descriptor(cfg_desc);
            cfg_desc = NULL;
        }
    }

search_end:
    LOG_INFO("Search end, %d devices is found!", mouse_cnt);

    if (mouse_cnt > 0) {
        if (err)
            free_mouses(list_head); // 仅遍历到部分鼠标就出错了，释放并返回错误
        else
            *mouses = list_head;
    } else
        err = -1; // 如果没找到任何鼠标设备，就返回错误

    if (cfg_desc)
        libusb_free_config_descriptor(cfg_desc);

    return (err || !mouses) ? -1 : mouse_cnt;
}

void mouse_transfer_free(struct mouse_desc *mouses)
{
    struct mouse_desc *node = mouses;

    LOG_INFO("Enter!");
    while (node) {
        if (node->async_transfer) {
            libusb_cancel_transfer(node->async_transfer);
            libusb_free_transfer(node->async_transfer);
        }
        node = node->next;
    }
}

static void exit_signal_func(int sig)
{
    LOG_INFO("Enter with signal %d", sig);

    keep_running = false;
}

void *sync_transfer_thread(void *data)
{
    int err = 0;
    struct mouse_desc *mouse = data;

    if (!data) {
        LOG_ERR("Invalid thread parameter!");
        return NULL;
    }

    pthread_detach(pthread_self());

    LOG_INFO("Enter with mouse endpoint: 0x%x!", mouse->endpoint);
    while (keep_running) {
        // 中断传输: libusb_interrupt_transfer
        err = libusb_interrupt_transfer(mouse->dev_handle, mouse->endpoint, mouse->buf, sizeof(mouse->buf),
                                    &mouse->transferred, mouse->timeout * 1000);
        // 判断并打印返回值
        if (!err || err == LIBUSB_ERROR_OVERFLOW) {
            printf("%d bytes transferred(err=%d): ", mouse->transferred, err);
            for (int i = 0; i < mouse->transferred && i < 8; i++) { // 最多只打印8个字节的数据
                printf("0x%x ", mouse->buf[i]);
            }
            printf("\n");
        } else if (err == LIBUSB_ERROR_TIMEOUT) {
            LOG_ERR("libusb_interrupt_transfer timeout!");
            continue;
        } else {
            LOG_ERR("libusb_interrupt_transfer failed!");
            break;
        }
    }

    LOG_ERR("SYNC transfer failed: %d", err);
    return NULL;
}

int mouse_sync_transfer(struct mouse_desc *mouses, int timeout)
{
    bool is_success = false;
    int err = 0, thread_cnt = 0, ret = 0;
    struct mouse_desc *mouse = NULL;
    pthread_t pid = -1;
    if (!mouses || !mouses->dev_handle
        || mouses->intf_num < 0 || mouses->endpoint < 0) {
        LOG_ERR("Invalid parameter!");
        return -1;
    }

    LOG_INFO("Enter!");
    // 为每个usb设备逐个创建线程进行传输循环
    mouse = mouses;
    while (mouse) {
        mouse->timeout = timeout;
        ret = pthread_create(&pid, NULL, sync_transfer_thread, mouse);
        if (ret < 0) {
            LOG_ERR("Create thread failed!");
            return -1;
        }
        mouse->sync_pid = pid;
        LOG_ERR("Transfer thread %d create!", thread_cnt++);
        mouse = mouse->next;
    }
    return 0;
}

static void mouse_async_transfer_cb(struct libusb_transfer *transfer)
{
    struct mouse_desc *mouse = transfer->user_data;
    int i = 0, err = 0;
    if (!mouse) {
        LOG_ERR("Invalid parameter!");
        return;
    }

    // LOG_INFO("Enter!");
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED || transfer->status == LIBUSB_TRANSFER_OVERFLOW) {
        // 打印数据
        printf("%d bytes transferred(status=%d): ", transfer->actual_length, transfer->status);
        for (i = 0; i < transfer->actual_length && i < 8; i++) { // 最多只打印8个字节的数据
            printf("0x%x ", mouse->buf[i]);
        }
        printf("\n");
    } else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
        LOG_WARN("Transer timeout, retry!");
    } else {
        LOG_ERR("ASYCN transfer exception stauts: %d", transfer->status );
        return;
    }

    // 提交新的一次传输
    err = libusb_submit_transfer(mouse->async_transfer);
    if (err) {
        LOG_ERR("Submit async transfer failed!");
        return;
    }
}

int mouse_async_transfer(struct mouse_desc *mouses, int timeout)
{
    struct mouse_desc *mouse = NULL;
    int err = 0;
    struct timeval tv;
    if (!mouses || !mouses->dev_handle
        || mouses->intf_num < 0 || mouses->endpoint < 0) {
        LOG_ERR("Invalid parameter!");
        return -1;
    }

    LOG_INFO("Enter!");
    // 循环遍历所有的mouse设备
    mouse = mouses;
    while (mouse) {
        // 分配transfer: libusb_alloc_transfer
        mouse->async_transfer = libusb_alloc_transfer(0);
        if (!mouse->async_transfer) {
            LOG_ERR("Alloc async transfer failed!");
            return -1;
        }

        // 填充transfer: libusb_fill_interrupt_transfer
        libusb_fill_interrupt_transfer(mouse->async_transfer,
                                    mouse->dev_handle,
                                    mouse->endpoint,
                                    mouse->buf,
                                    sizeof(mouse->buf),
                                    mouse_async_transfer_cb,
                                    mouse,
                                    timeout*1000
                                    );

        // 提交传输: libusb_submit_transfer
        err = libusb_submit_transfer(mouse->async_transfer);
        if (err) {
            LOG_ERR("Submit async transfer failed!");
            return -1;
        }

        mouse = mouse->next;
    }

    // 循环处理事件，在出现异常事件时处理: libusb_handle_events_timeout_completed
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    while (keep_running) {
        err = libusb_handle_events_timeout_completed(NULL, &tv, &g_completed);
        if (err) {
            LOG_ERR("libusb_handle_events_timeout_completed failed!");
            return -1;
        }
    }
    return 0;
}

void mouse_interface_release(struct mouse_desc *mouses)
{
    struct mouse_desc *node = mouses;

    LOG_INFO("Enter!");
    while (node) {
        if (node->is_claimed && node->dev_handle && node->intf_num >= 0)
            libusb_release_interface(node->dev_handle, node->intf_num);

        node = node->next;
    }
}

int mouse_interface_claim(struct mouse_desc *mouses)
{
    int err = 0;
    struct mouse_desc *mouse = mouses;

    while (mouse) {
        LOG_INFO("mouse info: intf_num=%d, endpoint=0x%x", mouse->intf_num, mouse->endpoint);
        err = libusb_set_auto_detach_kernel_driver(mouse->dev_handle, 1);
        if (err) {
            LOG_ERR("Detach kernel drive failed: %d", err);
            break;
        }

        // 声明当前接口: libusb_claim_interface <--> libusb_release_interface
        err = libusb_claim_interface(mouse->dev_handle, mouse->intf_num);
        if (err) {
            LOG_ERR("claim interface failed: %d", err);
            break;
        }
        LOG_INFO("mouse claim success!");
        mouse->is_claimed = true;
        mouse = mouse->next;
    }

    if (err)
        mouse_interface_release(mouses);

    return 0;
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
    int err = 0, extend_idx = 1, timeout = 5;
    char transfer_mode = 0; // 0 means sync, 1 means async
    // int transferred;
    struct mouse_desc *mouse = NULL;
    int num_dev = 0;

    LOG_INFO("Enter with argc %d", argc);

    while (extend_idx <= argc - 1) {
        if (!strcmp(argv[extend_idx], "-sync")) {
            // position
            transfer_mode = SYNC_USB_TRNSFER;
        } else if (!strcmp(argv[extend_idx], "-async")) {
            // position
            transfer_mode = ASYNC_USB_TRNSFER;
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
    // signal(SIGSEGV, exit_signal_func);
    signal(SIGTERM, exit_signal_func);
    signal(SIGABRT, exit_signal_func);

    num_dev = libusb_comb_init(&g_ctx, &g_device_list);
    if (num_dev < 0) {
        LOG_INFO("Init libusb failed: %d", num_dev);
        return -1;
    }

    err = search_mouses(&g_mouses, g_device_list, num_dev);
    if (err < 0) {
        LOG_ERR("Search mouse device failed: %d", err);
        return EXCUTE_FAILED_EXIT;
    }

    // 卸载内核驱动并认领端口：libusb_set_auto_detach_kernel_driver
    err = mouse_interface_claim(g_mouses);
    if (err) {
        LOG_ERR("mouse_interface_claim failed: %d", err);
        goto out_mouse_free;
    }

    if (transfer_mode == SYNC_USB_TRNSFER) {
        // 同步传输
        mouse_sync_transfer(g_mouses, timeout);
    } else if (transfer_mode == ASYNC_USB_TRNSFER) {
        // 异步传输
        mouse_async_transfer(g_mouses, timeout);
    } else {
        LOG_ERR("unsupport trnasfer mode: %d", transfer_mode);
        goto intf_release;
    }
    is_success = true;

    while (keep_running) {
        sleep(5);
    }
    // 资源释放
    LOG_INFO("The end, release resources!");

// out_transfer_free:
    if (g_mouses)
        mouse_transfer_free(g_mouses);

intf_release:
    if (g_mouses)
        mouse_interface_release(g_mouses);

out_mouse_free:
    if (g_mouses)
        free_mouses(g_mouses);

libusb_free:
    if (g_ctx && g_device_list)
        libusb_comb_free(g_ctx, g_device_list);

    return is_success ? EXCUTE_SUCCESS_EXIT : EXCUTE_FAILED_EXIT;
}
