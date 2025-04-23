# I2C应用编程

## 一、重要结构体

![image-20250411224823344](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250411224823344.png)

### 1、struct i2c_adapter

对应以上的I2C Controller，表示I2C的主设备，也就是控制器，抽象成以下结构体：

```c
struct i2c_adapter {
	struct module *owner;
	unsigned int class;		  /* classes to allow probing for */
	const struct i2c_algorithm *algo; /* the algorithm to access the bus */
	void *algo_data;

	/* data fields that are valid for all devices	*/
	const struct i2c_lock_operations *lock_ops;
	struct rt_mutex bus_lock;
	struct rt_mutex mux_lock;

	int timeout;			/* in jiffies */
	int retries;
	struct device dev;		/* the adapter device */
	unsigned long locked_flags;	/* owned by the I2C core */
#define I2C_ALF_IS_SUSPENDED		0
#define I2C_ALF_SUSPEND_REPORTED	1

	int nr;
	char name[48];
	struct completion dev_released;

	struct mutex userspace_clients_lock;
	struct list_head userspace_clients;

	struct i2c_bus_recovery_info *bus_recovery_info;
	const struct i2c_adapter_quirks *quirks;

	struct irq_domain *host_notify_domain;
};
```

其中：

- algo：里面记录着I2C传输的函数；

  ```c
  struct i2c_algorithm {
  	/*
  	 * If an adapter algorithm can't do I2C-level access, set master_xfer
  	 * to NULL. If an adapter algorithm can do SMBus access, set
  	 * smbus_xfer. If set to NULL, the SMBus protocol is simulated
  	 * using common I2C messages.
  	 *
  	 * master_xfer should return the number of messages successfully
  	 * processed, or a negative value on error
  	 */
  	int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs,
  			   int num);
  	int (*master_xfer_atomic)(struct i2c_adapter *adap,
  				   struct i2c_msg *msgs, int num);
  	int (*smbus_xfer)(struct i2c_adapter *adap, u16 addr,
  			  unsigned short flags, char read_write,
  			  u8 command, int size, union i2c_smbus_data *data);
  	int (*smbus_xfer_atomic)(struct i2c_adapter *adap, u16 addr,
  				 unsigned short flags, char read_write,
  				 u8 command, int size, union i2c_smbus_data *data);
  
  	/* To determine what the adapter supports */
  	u32 (*functionality)(struct i2c_adapter *adap);
  
  #if IS_ENABLED(CONFIG_I2C_SLAVE)
  	int (*reg_slave)(struct i2c_client *client);
  	int (*unreg_slave)(struct i2c_client *client);
  #endif
  };
  ```

  

- nr：表示第几个I2C BUS(I2C Controller)，因为一个SOC可能包含多个控制器；



### 2、struct i2c_client

对应上图的Device A、Device B，表示I2C的从设备，也就是各个客户端，抽象成以下结构体：

```C
struct i2c_client {
	unsigned short flags;		/* div., see below		*/
#define I2C_CLIENT_PEC		0x04	/* Use Packet Error Checking */
#define I2C_CLIENT_TEN		0x10	/* we have a ten bit chip address */
					/* Must equal I2C_M_TEN below */
#define I2C_CLIENT_SLAVE	0x20	/* we are the slave */
#define I2C_CLIENT_HOST_NOTIFY	0x40	/* We want to use I2C host notify */
#define I2C_CLIENT_WAKE		0x80	/* for board_info; true iff can wake */
#define I2C_CLIENT_SCCB		0x9000	/* Use Omnivision SCCB protocol */
					/* Must match I2C_M_STOP|IGNORE_NAK */

	unsigned short addr;		/* chip address - NOTE: 7bit	*/
					/* addresses are stored in the	*/
					/* _LOWER_ 7 bits		*/
	char name[I2C_NAME_SIZE];
	struct i2c_adapter *adapter;	/* the adapter we sit on	*/
	struct device dev;		/* the device structure		*/
	int init_irq;			/* irq set at initialization	*/
	int irq;			/* irq issued by device		*/
	struct list_head detected;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	i2c_slave_cb_t slave_cb;	/* callback for slave mode	*/
#endif
};
```

其中：

- addr：表示I2C的地址；
- adapter：表示I2C对应的适配器，也就是前面的struct i2c_adapter结构体；



### 3、怎么表示要传输的数据

在以上提到的传输函数中：

```c
int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
```

可以看到信息由struct i2c_msg来表示：

```c
struct i2c_msg {
	__u16 addr;	/* slave address			*/
	__u16 flags;
#define I2C_M_RD		0x0001	/* read data, from slave to master */
					/* I2C_M_RD is guaranteed to be 0x0001! */
#define I2C_M_TEN		0x0010	/* this is a ten bit chip address */
#define I2C_M_DMA_SAFE		0x0200	/* the buffer of this message is DMA safe */
					/* makes only sense in kernelspace */
					/* userspace buffers are copied anyway */
#define I2C_M_RECV_LEN		0x0400	/* length will be first received byte */
#define I2C_M_NO_RD_ACK		0x0800	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK	0x1000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR	0x2000	/* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NOSTART		0x4000	/* if I2C_FUNC_NOSTART */
#define I2C_M_STOP		0x8000	/* if I2C_FUNC_PROTOCOL_MANGLING */
	__u16 len;		/* msg length				*/
	__u8 *buf;		/* pointer to msg data			*/
};
```

其中：

- flag：表示数据传输的方向，bit 0 等于 I2C_M_RD 表示读，bit 0 等于 0 表示写；

- 一个 i2c_msg 要么是读，要么是写

  举例：设备地址为 0x50 的 EEPROM，要读取它里面存储地址为 0x10 的一个字节，应该构造几个 i2c_msg？要构造 2 个 i2c_msg：

  - 第一个 i2c_msg 表示写操作，把要访问的存储地址 0x10 发给设备

  - 第二个 i2c_msg 表示读操作

  代码如下：

  ```C
  u8 data_addr = 0x10;
  i8 data;
  struct i2c_msg msgs[2];
  msgs[0].addr = 0x50;
  msgs[0].flags = 0;
  msgs[0].len = 1;
  msgs[0].buf = &data_addr;
  msgs[1].addr = 0x50;
  msgs[1].flags = I2C_M_RD;
  msgs[1].len = 1;
  msgs[1].buf = &data;
  ```

  

## 二、内核中应该怎么传输数据

一句话概括 I2C 传输：

- APP 通过 I2C Controller(即struct i2c_adapter) 与 I2C Device(即struct i2c_client) 传输数据;

- APP 通过 i2c_adapter 与 i2c_client 传输 i2c_msg;

- 内核函数 i2c_transfer;

  ```c
  int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
  ```


i2c_msg 里含有 addr，所以这个函数里不需要 i2c_client:



## 三、APP无需编写驱动即可访问I2C设备

内核**drivers/i2c/i2c-dev.c**是I2C的驱动代码，框架如下：

![image-20250417085712964](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250417085712964.png)

### 1、体验I2C-Tools

**核心思路：APP通过I2C Controller和I2C Device传输数据；**

所以，对于I2C-Tools也是一样，需要指定：

- 哪个I2C控制器
- 哪个I2C设备
- 数据：读还是写、数据本身



#### 1.1 交叉配置

- 配置编译器：
  在Makefile中，将编译器改成带交叉编译链前缀的：

  ```makefile
  CC		= $(CROSS_COMPILE)gcc
  AR		= $(CROSS_COMPILE)ar
  STRIP	= $(CROSS_COMPILE)strip
  ```

- 将生成在Tools和lib下面的bin文件和库文件拷贝到开发板：

  ```shell
  book@100ask:~/resource/open_lib/i2c-tools-4.2/tools$ ls
  i2cbusses.c  i2cdetect    i2cdetect.o  i2cdump.c  i2cget.8  i2cset    i2cset.o       i2ctransfer.c  util.c
  i2cbusses.h  i2cdetect.8  i2cdump      i2cdump.o  i2cget.c  i2cset.8  i2ctransfer    i2ctransfer.o  util.h
  i2cbusses.o  i2cdetect.c  i2cdump.8    i2cget     i2cget.o  i2cset.c  i2ctransfer.8  Module.mk      util.o
  book@100ask:~/resource/open_lib/i2c-tools-4.2/lib$ ls
  libi2c.3  libi2c.a  libi2c.map  libi2c.so  libi2c.so.0  libi2c.so.0.1.1  Module.mk  smbus.ao  smbus.c  smbus.o
  ```

- 开发板上执行bin文件查看效果：

  ```shell
  [root@100ask:/mnt]# i2cdetect -l
  i2c-1   i2c             STM32F7 I2C(0x40013000)                 I2C adapter
  i2c-2   i2c             STM32F7 I2C(0x5c002000)                 I2C adapter
  i2c-0   i2c             STM32F7 I2C(0x40012000)                 I2C adapter
  [root@100ask:/mnt]# i2cdetect -F 0
  Functionalities implemented by /dev/i2c-0:
  I2C                              yes
  SMBus Quick Command              yes
  SMBus Send Byte                  yes
  SMBus Receive Byte               yes
  SMBus Write Byte                 yes
  SMBus Read Byte                  yes
  SMBus Write Word                 yes
  SMBus Read Word                  yes
  SMBus Process Call               yes
  SMBus Block Write                yes
  SMBus Block Read                 yes
  SMBus Block Process Call         yes
  SMBus PEC                        yes
  I2C Block Write                  yes
  I2C Block Read                   yes
  [root@100ask:/sys/class/i2c-dev/i2c-0]# i2cdetect -y -a 0
       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
  00: 00 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  10: -- -- -- -- -- -- -- -- -- -- UU -- -- -- 1e --
  20: -- -- UU -- -- -- -- -- -- -- -- -- -- -- -- --
  30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  [root@100ask:/]# i2cset -f -y 0 0x1e 0 0x4
  [root@100ask:/]# i2cset -f -y 0 0x1e 0 0x3
  [root@100ask:/]# i2cget -f -y 0 0x1e 0xc w
  0x0050
  [root@100ask:/]# i2cget -f -y 0 0x1e 0xe w
  0x0006
  [root@100ask:/]#
  [root@100ask:/]#
  [root@100ask:/]# i2ctransfer -f -y 0 w2@0x1e 0 0x4
  [root@100ask:/]# i2ctransfer -f -y 0 w2@0x1e 0 0x3
  [root@100ask:/]# i2ctransfer -f -y 0 w1@0x1e 0xc r2
  0x50 0x00
  [root@100ask:/]# i2ctransfer -f -y 0 w1@0x1e 0xe r2
  0x05 0x00
  [root@100ask:/]# i2ctransfer -f -y 0 w1@0x1e 0xc r2
  0x55 0x00
  [root@100ask:/]# i2ctransfer -f -y 0 w1@0x1e 0xc r2
  0x55 0x00
  [root@100ask:/]# i2ctransfer -f -y 0 w1@0x1e 0xc r2
  0x54 0x00
  ```



## 四、I2C-Tools框架

### 1、使用I2C方式

示例代码：i2ctransfer.c

![image-20250417105414591](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250417105414591.png)





### 2、使用SMBus方式

示例代码：i2cget.c、i2cset.c

![image-20250417105504101](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250417105504101.png)











