# 串口应用编程

## 1、串口API

之前提到了tty的框架大致如下：

![tty_driver_framework](E:\embedded_learning\embedded_learning_code_management\src\012_uart_tst\tty_driver_framework.jpg)

在linux系统中，操作设备的统一接口就是：open/ioctl/read/write

对于UART，又在ioctl上封装了很多函数，主要用来设置行规程；

所以UART编程的流程就是：

- open设备节点
  可选flag：
  O_RDWR：可读可写
  O_NOCTTY：不要将当前打开的终端设置成console控制台；

- 设置行规程，用来规定波特率、数据位、停止位、校验位、RAW模式、一有数据就返回

- 可以选择设置串口为阻塞状态：

  fcntl(fd, F_SETFL, FNDELAY);
  // 读数据时不等待，没有数据就返回0

  fcntl(fd, F_SETFL, 0);

  // 读数据时，没有数据阻塞

- read/write



## 2、如何设置行规程？

串口抽象出了一个结构体用来表示串口传输所需要用到的参数：struct termios；原型如下：

```c
#include <termios.h>

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

struct termios {
	tcflag_t c_iflag;		/* input mode flags */
	tcflag_t c_oflag;		/* output mode flags */
	tcflag_t c_cflag;		/* control mode flags */
	tcflag_t c_lflag;		/* local mode flags */
	cc_t c_line;			/* line discipline */
	cc_t c_cc[NCCS];		/* control characters */
    /*
    c_cc[VMIN]，表示需要读几个位再返回
    c_cc[VTIME]，表示需要等待第一位数据的时间，单位为10S，等待到第一位数据的之后，会一直等待到满足c_cc[VMIN]规定的位数；
    */ 
};
```

以上这些元素命名有一些惯例:

- tc：terminal control
- cf：control flag



#### 关键字段详解

#### **`c_cflag`**（控制模式）

| 标志      | 说明                                                       | 常用值                     |
| :-------- | :--------------------------------------------------------- | :------------------------- |
| `CBAUD`   | 波特率掩码（已废弃，改用 `cfsetispeed()`/`cfsetospeed()`） | -                          |
| `CSIZE`   | 数据位位数掩码                                             | `CS5`, `CS6`, `CS7`, `CS8` |
| `CSTOPB`  | 停止位位数：`0`=1位，`1`=2位                               | `0` 或 `1`                 |
| `PARENB`  | 启用校验位：`1`=启用，`0`=禁用                             | `0` 或 `1`                 |
| `PARODD`  | 校验类型：`1`=奇校验，`0`=偶校验（需 `PARENB=1`）          | `0` 或 `1`                 |
| `CLOCAL`  | 忽略调制解调器控制线（`1`=始终视为就绪）                   | `CLOCAL`                   |
| `CREAD`   | 允许接收数据（`1`=启用）                                   | `CREAD`                    |
| `CRTSCTS` | 硬件流控（RTS/CTS）：`1`=启用                              | `0` 或 `CRTSCTS`           |



#### **`c_iflag`**（输入模式）

| 标志     | 说明                                    | 常用值          |
| :------- | :-------------------------------------- | :-------------- |
| `IGNBRK` | 忽略输入中的BREAK条件                   | `0` 或 `IGNBRK` |
| `BRKINT` | BREAK产生中断信号（需与 `IGNBRK` 互斥） | `0` 或 `BRKINT` |
| `IGNPAR` | 忽略奇偶校验错误                        | `0` 或 `IGNPAR` |
| `PARMRK` | 标记奇偶校验错误（通过特殊字符）        | `0` 或 `PARMRK` |
| `INPCK`  | 启用输入奇偶校验检查                    | `0` 或 `INPCK`  |
| `ISTRIP` | 剥离输入字符的第8位（强制7位数据）      | `0` 或 `ISTRIP` |
| `IXON`   | 启用软件输出流控（XON/XOFF）            | `0` 或 `IXON`   |
| `IXOFF`  | 启用软件输入流控（XON/XOFF）            | `0` 或 `IXOFF`  |



#### **`c_oflag`（输出模式）**

| 标志    | 说明                                     | 常用值         |
| :------ | :--------------------------------------- | :------------- |
| `OPOST` | 启用输出处理（如转换换行符）             | `0` 或 `OPOST` |
| `ONLCR` | 将换行符（`\n`）转换为回车换行（`\r\n`） | `0` 或 `ONLCR` |
| `OCRNL` | 将回车符（`\r`）转换为换行符（`\n`）     | `0` 或 `OCRNL` |



#### **`c_lflag`（本地模式）**

| 标志     | 说明                                                       | 常用值          |
| :------- | :--------------------------------------------------------- | :-------------- |
| `ICANON` | 启用规范模式（行缓冲，需按回车键才提交数据，也就是行规程） | `0` 或 `ICANON` |
| `ECHO`   | 回显输入字符                                               | `0` 或 `ECHO`   |
| `ECHOE`  | 将擦除字符（Backspace）显示为 `\b \b`                      | `0` 或 `ECHOE`  |
| `ISIG`   | 启用信号（如 Ctrl+C 中断）                                 | `0` 或 `ISIG`   |
| `IEXTEN` | 启用扩展输入处理（如特殊字符）                             | `0` 或 `IEXTEN` |



#### **`c_cc[NCCS]`（特殊控制字符）**

| 字符索引 | 说明                                | 默认值（键位） |
| :------- | :---------------------------------- | :------------- |
| `VINTR`  | 中断（Ctrl+C）                      | `0x03` (`^C`)  |
| `VQUIT`  | 退出（Ctrl+\）                      | `0x1C` (`^\`)  |
| `VERASE` | 擦除（Backspace）                   | `0x7F` (`DEL`) |
| `VTIME`  | 非规范模式下的超时时间（0.1秒单位） | -              |
| `VMIN`   | 非规范模式下的最小读取字符数        |                |



#### 设置示例：

```c
struct termios options;
tcgetattr(fd, &options);  // 获取当前配置
options.c_cflag &= ~CSIZE;  // 清除数据位掩码
options.c_cflag |= CS8;     // 8位数据
options.c_cflag &= ~PARENB; // 无校验
options.c_cflag &= ~CSTOPB; // 1位停止位
options.c_cflag &= ~CRTSCTS; // 无硬件流控
options.c_cflag |= (CLOCAL | CREAD); // 忽略调制解调器，启用接收

options.c_iflag = 0; // 禁用所有输入处理
options.c_oflag = 0; // 禁用所有输出处理

options.c_lflag &= ~(ICANON | ECHO | ISIG);  // 原始模式，无回显

options.c_cc[VTIME] = 0;  // 无超时
options.c_cc[VMIN]  = 0;  // 非阻塞，直接返回

if (tcsetattr(fd, TCSANOW, &options) != 0) {
    perror("tcsetattr failed");
    return -1;
}

```







通过ioctl可以设置以上结构体，但是工具链提供了一些函数用于快捷设置：

| 函数名      | 作用                                    |
| ----------- | --------------------------------------- |
| tcgetattr   | get terminal attributes，获得终端的属性 |
| tcsetattr   | set terminal attributes，修改终端参数   |
| tcflush     | 清空终端未完成的输入、输出请求及数据    |
| cfsetispeed | 设置输入波特率                          |
| cfsetospeed | 设置输出波特率                          |
| cfsetspeed  | 同时设置输入、输出波特率                |











