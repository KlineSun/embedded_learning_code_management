# TTY驱动框架

## 一、框架图

![tty_driver_framework](E:\embedded_learning\embedded_learning_code_management\src\012_uart_tst\tty_driver_framework.jpg)



### 1、框架设计理念：

我们的IO设备/终端设备多种多样，每种设备对应的驱动程序也多种多样，为了统一操作多种终端/IO设备的用法，设计了TTY驱动，这使得上层APP操作硬件变得简单，只需要知道tty设备的操作方法即可；例如应用程序中，如果我们要往UART中输出字符，则可以打开/dev/ttyS0（可能有其他对应串口设备的路径），对文件句柄进行读写即可；



### 2、重要概念：行规程

我们人为操作时，很可能发生输入错误的情况，需要等用户确认清楚输入之后，再发送给APP执行，因此需要在APP与IO设备之间有一个整理的功能，功能如下：

![行规程](E:\embedded_learning\embedded_learning_code_management\src\012_uart_tst\行规程.jpg)

### 3、修改设备树文件

- 在stm32mp15xx-100ask.dtsi(具体位置在~/100ask_stm32mp157_pro-sdk/Linux-5.4/arch/arm/boot/dts下)文件中，找到uart4的位置，在其后添加UART8的运行状态(okay或者disable)和pin脚：

  ![image-20250405214302120](C:\Users\Jianyuan Sun\AppData\Roaming\Typora\typora-user-images\image-20250405214302120.png)

  ```dtd
  &uart8 {
          pinctrl-names = "default", "sleep";
          pinctrl-0 = <&uart8_pins_mx>;
          pinctrl-1 = <&uart8_sleep_pins_mx>;
          status = "okay";
  };
  ```

- 在stm32mp157-100ask-pinctrl.dtsi文件中，已经规定了uart8_pins_mx、uart8_sleep_pins_mx的具体配置，对应的是：
  ![image-20250405215403733](C:\Users\Jianyuan Sun\AppData\Roaming\Typora\typora-user-images\image-20250405215403733.png)

  ```dtd
  	uart8_pins_mx: uart8_mx-0 {
  		pins1 {
  			pinmux = <STM32_PINMUX('E', 1, AF8)>; /* UART8_TX */
  			bias-pull-up;
  			drive-push-pull;
  			slew-rate = <2>;
  		};
  		pins2 {
  			pinmux = <STM32_PINMUX('E', 0, AF8)>; /* UART8_RX */
  			bias-pull-up;
  		};
  	};
  	uart8_sleep_pins_mx: uart8_sleep_mx-0 {
  		pins {
  			pinmux = <STM32_PINMUX('E', 0, ANALOG)>, /* UART8_RX */
  			 <STM32_PINMUX('E', 1, ANALOG)>; /* UART8_TX */
  			};
  		};
  ```

  其中，

  pinmux = <STM32_PINMUX('E', 1, AF8)>;

  这一句表示指定PE1接口的功能为AF8(意思为串口的发送引脚)，也就是GPIO口PE1(对应串口的TX口)：

  ![image-20250405215635660](C:\Users\Jianyuan Sun\AppData\Roaming\Typora\typora-user-images\image-20250405215635660.png)

- 在stm32mp157c-100ask-512d-v1.dts文件中指定UART8设备的别名为serial3：

  ```dtd
  	aliases {
  		ethernet0 = &ethernet0;
  		serial0 = &uart4; //debug
  		serial1 = &usart6; //rs485
  		serial2 = &usart1; //bluetooth
  		serial3 = &uart8;
  	};
  ```

- 编译：

  - 先设置工具链：

    export ARCH=arm
    export CROSS_COMPILE=arm-buildroot-linux-gnueabihf-

    export PATH=$PATH:/home/book/100ask_stm32mp157_pro-sdk/ToolChain/armbuildroot-linux-gnueabihf_sdk-buildroot/bin

  - 编译dtb：

    make dtbs

  - 生成结果：arch/arm/boot/dts/stm32mp157c-100ask-512d-lcd-v1.dtb

- 更新开发板上的dtb

  - 将stm32mp157c-100ask-512d-lcd-v1.dtb文件拷贝到开发板上

  - 更新设备树（开发板上执行）：
    mount /dev/mmcblk2p2 /boot

    cp /mnt/stm32mp157c-100ask-512d-lcd-v1.dtb /boot
    sync

    reboot

- 更新后的驱动状态，在/dev/下可以看到以下设备节点：
  /dev/ttySTM0
  /dev/ttySTM1
  /dev/ttySTM3

  以上ttySTM3就是新增的串口设备，对应UART8；























