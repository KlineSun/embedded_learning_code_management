#ifndef GPIO_UTIL_H
#define GPIO_UTIL_H

#include <stdint.h>

// stm32 address
#define STM32_RCC_ADDR_BASE (0x50000000)
#define STM32_RCC_MP_AHB4ENSETR_OFFSET (0xa28) // gpio a~k clk enable
#define STM32_RCC_MC_AHB5ENSETR_OFFSET (0x290) // gpioz clk enable

#define STM32_GPIOx_ADDR_BASE (0x50002000)
#define STM32_GPIOz_ADDR_BASE (0x54004000)
#define STM32_GPIO_REG_ADDR_GAP  (0x1000)
#define GPIO_OTYPER_OFFSET (0x4)
#define GROUP_NGPIO (16)

// gpioa ~ gpiok
#define STM32_GPIOx_NUM_BASE (0)
#define STM32_GPIOx_GROUPS (11)
#define STM32_GPIOx_NUM_MAX (STM32_GPIOx_NUM_BASE+(STM32_GPIOx_GROUPS*GROUP_NGPIO))

// gpioz
#define STM32_GPIOz_NUM_BASE (400)
#define STM32_GPIOz_NUM_MAX  (STM32_GPIOz_NUM_BASE + GROUP_NGPIO)

// number defines
#define PUSH_PULL_OUT   (0)
#define OPEN_DRAIN_OUT  (1)
#define MAX_SYS_GPIO_PATH_LEN (64)
#define GPIO_DIRECTION_IN (0)
#define GPIO_DIRECTION_OUT (1)


// path
#define GPIO_SYS_PATH               "/sys/class/gpio"
#define GPIO_SYS_EXPORT_PATH        GPIO_SYS_PATH "/export"
#define GPIO_SYS_UNEXPORT_PATH      GPIO_SYS_PATH "/unexport"

int is_gpio_num_valid(uint16_t gpio_num);
char stm32_get_gpio_group_num(uint16_t gpio_num);
int stm32_get_gpio_group_base(char group, uint32_t *gpio_base);
int stm32_get_gpio_output_type(uint16_t gpio_num);
int stm32_set_gpio_output_type(uint16_t gpio_num, int val);

int sys_gpio_export(uint16_t gpio_num);
void sys_gpio_unexport(uint16_t gpio_num);
int sys_gpio_set_value(uint16_t gpio_num, uint8_t value);
int sys_gpio_get_value(uint16_t gpio_num, uint8_t *value);
int sys_gpio_set_direction(uint16_t gpio_num, uint8_t dirt);
int sys_gpio_get_direction(uint16_t gpio_num, uint8_t *dirt);
#endif //GPIO_UTIL_H