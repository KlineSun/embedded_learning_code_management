# UART学习

## 一、UART介绍

UART(Universal Asynchronous Receiver and Transmitter，通用异步收发器)

用途：

- 打印调试信息
- 外接各种模块
  - GPS
  - 蓝牙等



### 1、硬件介绍

串口结构简单且可靠，通过TX、RX通路即可完成两个器件之间的通信；

![UART串口通信浅谈之(一)--基础概述_uart的下载速率-CSDN博客](https://ts1.tc.mm.bing.net/th/id/R-C.8574aba425d168015b0cb743ba2c8ee0?rik=h5YBTgZHnXoErQ&riu=http%3a%2f%2fc.51hei.com%2fd%2fforum%2f201309%2f28%2f143051k09f8ffohm8m9c83.jpg&ehk=8FpY1uuNCdK79pxZ8jDCfk6%2f9VveSQ7yNifpfAgf4Z8%3d&risl=&pid=ImgRaw&r=0)



### 2、串口协议

串口数据要素：

- 波特率：双方约定传输数据的速度，单位bps，表示每秒传输多少位数据，也规定了每传输一位数据占用的时间，每位数据的值使用高低电平表示，高电平表示1，低电平表示0；
- 格式：数据位、停止位、校验位、流控



传输规则：
UART数据帧格式：

**起始位（1） | 数据位（5-8） | 校验位（1，可选） | 停止位（1-2）**

- UART线原来处于高电平状态，ARM(发送方)拉低，保持1bit时间
- PC(接收方)在低电平开始处计时；第一个电平时间就是开始位；
- ARM根据数据驱动TXD电平，数据从低位往高位发， 比如要发送 [0100 0001] 这一字节数据，则先发送bit[0]，输出高电平；这八位称为数据位；
  与此同时，PC端在每个数据位的中间时刻读取引脚状态，也就是读取RXD引脚的状态；
- 校验位(分为奇/偶校验位：数据位 + 校验位中为1的数据是奇/偶数)
- 将电平拉高，即停止位，双方可以约定停止位占用的时间(1T/1.5T/2T，T为位传输时间)；



### 知识点：

**奇偶校验位：**帮助简单检查数据传输过程中是否发生错误

**奇校验：**数据位 + 校验位 中为1的位数总为奇数

**偶校验：**数据 + 校验位 中为1的位数总为偶数

**无校验：**不添加校验位，现代高速通信中，依赖更高级的错误检测机制，通常会弃用掉校验位；

**局限性：**只能检查奇数个bit传输出错，不能检查偶数个bit传输出错；



**逻辑电平**

**常规逻辑电平：**常用于TTL/CMOS电路中，高电平(比如高于3V或5V)表示逻辑1，低电平(0V)表示逻辑0；不适用于长距离传输；

**RS-232逻辑电平：**适用于长距离传输，能有效降低数据失真率，+3V 到 +12V表示逻辑0，-3V 到 -12V表示逻辑1；

在ARM芯片中，串口引脚使用的是TTL逻辑电平，而电脑常用RS-232电平，两种逻辑电平之间需要电平转换芯片来完成转化；



### 串口数据在CPU中的数据流

ARM --> PC

- 内存中的1byte数据需要发送
- 将数据存入FIFO(先入先出)
- FIFO的数据通过CPU的UART控制单元，其中的Transmit Shifter(移位器)逐位发送电平到TXD上，
- 接收方设备通过串口线获取到逻辑电平信号；



PC --> ARM

- 发送方设备逐位发送数据，RXD收到逻辑电平；
- Receive Shifter逐位接受逻辑电平信号，并将数据存入FIFO；
- 控制程序从FIFO中读取数据；
- 逐位获取数据并写入内存；



## 二、TTY设备节点

### 1、背景

电传机teletype

teletype，更准确地说是teleprinter，是一种通信设备，可以用来发送、接收文本信息；

teletype是一家公司的名字，但是他生产的teleprinter太出名，以至于teleprinter都被称为teletype了；

teletype是被用来传输商业电报的；



后来，teletype的数据被传输给电脑处理，所以需要在电脑上有teletype相关的驱动来完成电脑和远程设备(称为terminal)之间的数据传输，这个驱动被命名为teletype，也就是tty；

后来，terminal泛指的终端设备越来越多，比如打印机等，而tty驱动的名称被保留下来，它用来完成计算机和终端之间的数据传输；

随着发展，软件终端也出现了，不再局限于硬件终端设备，比如命令行就是一个终端，也就是terminal；在ubuntu中可以通过快捷键ctrl+alt+F3打开TTY3终端，ctrl+alt+F4打开TTY4终端



### 2、**TTY驱动对应关系**：

shell0：/dev/ttyS0，打开的是UART，是真实的终端

shell1：/dev/tty3。打开的是虚拟终端

shell2：/dev/tty4

此外，/dev/tty0始终代表前台终端；

每个终端对应的/dev/tty，都指向当前应用自己的终端，例如：

在tty3中执行"while [ 1 ]; do echo msg_from_tty3 > /dev/tty; sleep 3; done"，这个命令中指定的/dev/tty始终是当前终端



### 3、/dev/console是什么？

terminal一般是远端设备，权限一般来说是有限的；而console是控制台，它也是一个终端，只是权限较高；我们也可以选中当前终端中的一个作为控制台，但并非所有终端都可以作为控制台；在内核启动的cmdline终可以指定console是哪一个，如:

console=ttyS0 console=tty

注意：

- 如果我们在cmdline中指定了多个console，则**/dev/console只会取最后一个console配置**‘

- console=tty表示前台程序的虚拟终端，与console=tty0作用相同；
- 如果/proc/cmdline没有指定console，那么/dev/console对应的是第一个tty设备，根据内核安装驱动的顺序来决定第一个是谁，比如串口驱动或者虚拟终端驱动，谁先安装用谁；

- 我们并不需要了解启动时配置的console对应的是哪一个，直接用/dev/console代替即可，这样就可以用到指定的终端了；





## 三、总结

区分：

/dev/ttyS0：代表串口

/dev/ttySAC0：代表串口，但是不同的开发板，对应的串口驱动名称可能不同

/dev/tty：对应当前终端设备自己对应的tty驱动

/dev/tty0：对应最前面的tty终端

/dev/tty1：对应tty1，其他的如tty3、tty4都类似

/dev/console：

- 控制台，可以用来输出日志(如内核打印信息)和接收输入；
- 本身也是一个终端，但是权限比一般终端更高；
- 在开机的时候设置在cmdline中，比如：
  BOOT_IMAGE=/vmlinuz-5.4.0-150-generic root=UUID=56e2ca8b-3d73-46df-9a2a-58fa2247002f ro console=ttyS0 console=tty3 splash
- 在Ubuntu开机时一直按住esc键，可以进入设置界面，选择option，将对应linux系统的开机参数由原来的quiet设置为指定console的位置







