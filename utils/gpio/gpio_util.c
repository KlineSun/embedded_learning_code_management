
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "gpio_util.h"
#include "common_util.h"

int is_gpio_num_valid(uint16_t gpio_num)
{

    if ((gpio_num > STM32_GPIOx_NUM_MAX && gpio_num < STM32_GPIOz_NUM_BASE)
         || gpio_num > STM32_GPIOz_NUM_BASE)
    {
        LOG_INFO("Number from gpioa to gpiok is between (%d, %d)", STM32_GPIOx_NUM_BASE, STM32_GPIOx_NUM_MAX);
        LOG_INFO("Number of gpioz is between (%d, %d)", STM32_GPIOz_NUM_BASE, STM32_GPIOz_NUM_MAX);
        return 0;
    }
    return 1;
}

char stm32_get_gpio_group_num(uint16_t gpio_num)
{
    if (!is_gpio_num_valid(gpio_num)) {
        LOG_INFO("Gpio number beyond the limit: %d.", gpio_num);
        return -1;
    }

    if (gpio_num > STM32_GPIOx_NUM_BASE && gpio_num < STM32_GPIOx_NUM_MAX)
        return 'A' + (gpio_num / 16);
    else
        return 'Z';
}

int stm32_get_gpio_group_base(char group, uint32_t *gpio_base)
{
    if (group < 0) {
        LOG_INFO("Invalid parameter!");
        return -1;
    }

    *gpio_base = group == 'Z' ? STM32_GPIOz_ADDR_BASE : (STM32_GPIOx_ADDR_BASE + (group - 'A') * STM32_GPIO_REG_ADDR_GAP);
    return 0;
}

/**
 * @brief control gpio pin output type
 * 
 * @param gpio_num: gpio number
 * @param oprt: "r" means read, "w" means write.
 * @param val: input value if oprt is "w". Input 0 means control output-type as push-pull, or 1 means open-drain
 * 
 * @return 0 means current output-type is push-pull, 1 means open-drain. return negative number on failure
 * 
*/
static int stm32_ctrl_gpio_output_type(uint16_t gpio_num, const char *oprt, int val)
{
    char cmd_buf[MAX_SHELL_CMD_LEN] = {0};
    char cmd_rsp[MAX_SHELL_RESULT_LEN] = {0};
    uint32_t gpio_base = 0;
    uint32_t rcc_en_addr = 0;
    uint32_t gpio_type = 0;
    uint8_t  rcc_en_bit = 0, pin_offset = 0;
    char group = 0;
    uint8_t direction = 0; // 0 means read, 1 means write

    if (!strcasecmp(oprt, "r") || !strcasecmp(oprt, "R")) {
        direction = 0;
    } else if (!strcasecmp(oprt, "w") || !strcasecmp(oprt, "W")) {
        direction = 1;
    } else {
        LOG_INFO("Invalid operate: %s", oprt);
        return -1;
    }

    group = stm32_get_gpio_group_num(gpio_num);
    if (group < 0) {
        LOG_INFO("Get gpio group number failed!");
        return -1;
    }

    if (stm32_get_gpio_group_base(group, &gpio_base)) {
        LOG_INFO("Get gpio base address failed!");
        return -1;
    }
    LOG_INFO("Get gpio%c register base: 0x%x", group, gpio_base);

    if (group == 'Z') {
        rcc_en_addr = STM32_RCC_ADDR_BASE + STM32_RCC_MC_AHB5ENSETR_OFFSET;
        rcc_en_bit = 0;
        pin_offset = gpio_num - STM32_GPIOz_NUM_BASE;
    } else {
        rcc_en_addr = STM32_RCC_ADDR_BASE + STM32_RCC_MP_AHB4ENSETR_OFFSET;
        rcc_en_bit = group - 'A';
        pin_offset = gpio_num - 16 * (group - 'A');
    }

    // check gpio output mode
    snprintf(cmd_buf, MAX_SHELL_CMD_LEN, MKCMD("devmem 0x%x 32 0x%x; devmem 0x%x"),
                                            rcc_en_addr,
                                            1 << rcc_en_bit,
                                            gpio_base + GPIO_OTYPER_OFFSET);
    if (shell_cmd_excute(cmd_buf, cmd_rsp, MAX_SHELL_RESULT_LEN) < 0) {
        LOG_INFO("excute shell cmd failed!");
        return -1;
    }

    // parse result from rsp
    gpio_type = strtoul(cmd_rsp, NULL, 0);
    LOG_INFO("Get GPIO%c type: 0x%x", group, gpio_type);
    if (direction == 0) {
        if (gpio_type & (1 << pin_offset)) {
            LOG_INFO("type of gpio%d is open-drain!", gpio_num);
            return 1;
        } else {
            LOG_INFO("type of gpio%d is push-pull!", gpio_num);
            return 0;
        }
    }

    memset((void *)cmd_buf, 0, MAX_SHELL_CMD_LEN);
    if (val == PUSH_PULL_OUT) {
        gpio_type &= ~(1 << pin_offset);
    } else if (val == OPEN_DRAIN_OUT) {
        gpio_type |= 1 << pin_offset;
    } else {
        LOG_INFO("Unsupport output type: %d", val);
        return 0;
    }
    snprintf(cmd_buf, MAX_SHELL_CMD_LEN, MKCMD("devmem 0x%x 32 0x%x; devmem 0x%x 32 0x%x"),
                                            rcc_en_addr,
                                            1 << rcc_en_bit,
                                            gpio_base + GPIO_OTYPER_OFFSET,
                                            gpio_type);
    if (shell_cmd_excute(cmd_buf, cmd_rsp, MAX_SHELL_RESULT_LEN)) {
        LOG_INFO("excute shell cmd failed!");
        return -1;
    }
    LOG_INFO("Set gpio%d type as %s", gpio_num, val == PUSH_PULL_OUT ? "push-pull" : "open-drain");
    return 0;
}


/**
 * @brief get gpio pin output type
 * 
 * @return 0 means push-pull, 1 means open-drain.
 *  
*/
int stm32_get_gpio_output_type(uint16_t gpio_num)
{
    return stm32_ctrl_gpio_output_type(gpio_num, "r", 0);
}

/**
 * @brief get gpio pin output type
 * 
 * @param gpio_num: gpio number
 * @param val: 0 means push-pull, or 1 means open-drain
 * 
 * @return 0 means push-pull, 1 means open-drain.
 *  
*/
int stm32_set_gpio_output_type(uint16_t gpio_num, int val)
{
    return stm32_ctrl_gpio_output_type(gpio_num, "w", val);
}

int sys_gpio_export(uint16_t gpio_num)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    char cmd_buf[MAX_SHELL_CMD_LEN] = {0};
    char cmd_rsp[MAX_SHELL_RESULT_LEN] = {0};

    if (!is_gpio_num_valid(gpio_num)) {
        LOG_INFO("Gpio number beyond the limit: %d.", gpio_num);
        return -1;
    }

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d", GPIO_SYS_PATH, gpio_num);
    LOG_INFO("Export %s!", path);
    if (!access(path, F_OK))
        return 0; // already exist

    // export
    snprintf(cmd_buf, MAX_SHELL_CMD_LEN, MKCMD("echo %d > %s"), gpio_num, GPIO_SYS_EXPORT_PATH);
    if (shell_cmd_excute(cmd_buf, cmd_rsp, MAX_SHELL_RESULT_LEN) < 0) {
        LOG_INFO("ecute cmd failed!");
        return -1;
    }

    if (access(path, F_OK) != 0) {
        LOG_INFO("Export %s failed!", path);
        return -1;
    }
    return 0;
}

void sys_gpio_unexport(uint16_t gpio_num)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    char cmd_buf[MAX_SHELL_CMD_LEN] = {0};
    char cmd_rsp[MAX_SHELL_RESULT_LEN] = {0};

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d", GPIO_SYS_PATH, gpio_num);
    LOG_INFO("Unexport %s!", path);
    if (access(path, F_OK))
        return; // already not exist

    // unexport
    snprintf(cmd_buf, MAX_SHELL_CMD_LEN, MKCMD("echo %d > %s"), gpio_num, GPIO_SYS_UNEXPORT_PATH);
    if (shell_cmd_excute(cmd_buf, cmd_rsp, MAX_SHELL_RESULT_LEN) < 0)
        LOG_INFO("ecute cmd failed!");
}

int sys_gpio_set_value(uint16_t gpio_num, uint8_t value)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    char buf[8] = {0};
    int ret = 0;

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d/value", GPIO_SYS_PATH, gpio_num);
    // LOG_INFO("set value path %s!", path);
    if (access(path, F_OK)) {
        if (sys_gpio_export(gpio_num) || access(path, F_OK)) {
            LOG_INFO("Export %s failed!", path);
            return -1;
        }
    }

    snprintf(buf, 8, "%d", value);
    ret = write_file_string(path, buf, 8);
    if (ret < 0) {
        LOG_INFO("Write gpio%d value failed!", gpio_num);
        return ret;
    }
    return 0;
}

int sys_gpio_get_value(uint16_t gpio_num, uint8_t *value)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    int ret = 0;
    char buf[8] = {0};

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d/value", GPIO_SYS_PATH, gpio_num);
    // LOG_INFO("get value path %s!", path);
    if (access(path, F_OK)) {
        if (sys_gpio_export(gpio_num) || access(path, F_OK)) {
            LOG_INFO("Export %s failed!", path);
            return -1;
        }
    }

    ret = read_file_string(path, buf, 8);
    if (ret < 0) {
        LOG_INFO("Read gpio%d value failed!", gpio_num);
        return -1;
    }

    *value = atoi(buf);
    // LOG_INFO("current gpio value: %s -> %d", buf, *value);
    return 0;
}

int sys_gpio_set_direction(uint16_t gpio_num, uint8_t dirt)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    int ret = 0;
    char buf[8] = {0};

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d/direction", GPIO_SYS_PATH, gpio_num);
    LOG_INFO("get direction path %s!", path);
    if (access(path, F_OK)) {
        if (sys_gpio_export(gpio_num) || access(path, F_OK)) {
            LOG_INFO("Export %s failed!", path);
            return -1;
        }
    }

    snprintf(buf, 8, "%s", dirt == GPIO_DIRECTION_IN ? "in" : "out");
    ret = write_file_string(path, buf, 8);
    if (ret < 0) {
        LOG_INFO("Write gpio%d direction failed!", gpio_num);
        return ret;
    }
    return 0;
}

int sys_gpio_get_direction(uint16_t gpio_num, uint8_t *dirt)
{
    char path[MAX_SYS_GPIO_PATH_LEN] = {0};
    int ret = 0;
    char buf[8] = {0};

    snprintf(path, MAX_SYS_GPIO_PATH_LEN, "%s/gpio%d/direction", GPIO_SYS_PATH, gpio_num);
    LOG_INFO("get direction path %s!", path);
    if (access(path, F_OK)) {
        if (sys_gpio_export(gpio_num) || access(path, F_OK)) {
            LOG_INFO("Export %s failed!", path);
            return -1;
        }
    }

    ret = read_file_string(path, buf, 8);
    if (ret < 0) {
        LOG_INFO("Read gpio%d direction failed!", gpio_num);
        return -1;
    }

    *dirt = !strcmp(buf, "in") ? 0 : 1;
    LOG_INFO("current direction: %s -> %d", buf, *dirt);
    return 0;
}

