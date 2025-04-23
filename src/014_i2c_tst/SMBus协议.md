

# SMBus协议

## 一、简介

SMBus（System Management Bus， 系统管理总线）是一种基于I²C的通信协议，由Intel在1995年提出，主要用于**低功耗系统管理**；如：

- 电池状态监控（笔记本、手机）。
- 温度传感器读取。
- 电源管理芯片控制。
- 硬件状态监测（如风扇转速）。







## 二、SMBus和I2C的差异

关键差异：

| **特性**         | **SMBus**                      | **I²C**                 |
| :--------------- | :----------------------------- | :---------------------- |
| **电压电平**     | 固定3.3V（兼容1.8V~5V）        | 支持更宽范围（1.8V~5V） |
| **时钟速度**     | 标准模式（100kHz），可选400kHz | 支持高速模式（3.4MHz）  |
| **超时机制**     | 强制要求（35ms总线空闲超时）   | 无硬性超时要求          |
| **协议严格性**   | 严格定义时序、ACK/NACK规则     | 更灵活，依赖器件实现    |
| **电源控制**     | 支持总线唤醒（Alert#信号）     | 无专用唤醒机制          |
| **地址回应**     | 必须发出回应                   | 无强制要求              |
| **数据传输格式** | 会定义格式                     | 不定义                  |
| **重复S信号**    | 在读和写之间可以不发P信号      | 未定义该行为            |

上述的"超时机制"也就是类似于I2C中的Clock Stretching，指设备为了获取更多的处理时间hold on总线的时间(将SCL拉低)；SMBus要求超过35ms时，主设备必须强制将SCL复位成高电平，不继续进行等待；而I2C不会有超时机制，会一直等待；

此机制的作用是为了总线因某些问题导致永久阻塞；

**硬件影响**：

- 主设备需集成硬件超时计数器（如看门狗定时器）。
- 从设备需优化代码，避免长时间占用总线（如处理数据时快速响应）。

**重复S信号**：

![image-20250416094207434](C:\Users\Jianyuan Sun\AppData\Roaming\Typora\typora-user-images\image-20250416094207434.png)





## 三、SMBus物理结构

- **两线制**：SCL（时钟线）、SDA（数据线），与I²C完全兼容。
- **开漏输出**：需上拉电阻（典型值10kΩ）。(开漏输出，无法主动输出，高电平需要外部上拉)
- **特殊信号**：
  - **SMBA（Alert#）**：从设备中断请求线（低电平有效）。
  - **SMBus 2.0**：新增SUSCLK（挂起时钟）用于低功耗模式。



## 四、SMBus协议内容

### **1、数据帧格式**

与I²C类似，但更严格：

```
Start → [7位从地址 + R/W位] → ACK → [命令码] → ACK → [数据] → ACK/NACK → Stop
```

- **命令码（Command Byte）**：指定操作类型（如读取温度、写入配置）。
- **PEC（可选）**：包错误校验（CRC-8），增强可靠性。



#### (1) 格式中的符号说明

symbols（符号）

```markdown
S (1 bit) : Start bit(开始位)

Sr (1 bit) : 重复的开始位

P (1 bit) : Stop bit(停止位)

R/W# (1 bit) : Read/Write bit. Rd equals 1, Wr equals 0.(读写位)

A, N (1 bit) : Accept and reverse accept bit.(回应位)

Address(7 bits): I2C 7 bit address. Note that this can be expanded as usual to get a 10 bit I2C address.(地址	位，7 位地址)

Command Code (8 bits): Command byte, a data byte which often selects a register on the device.(命令字节，一般用	来选择芯片内部的寄存器)

Data Byte (8 bits): A plain data byte. Sometimes, I write DataLow, DataHigh for 16 bit data.(数据字节，8 位；如	果是 16 位数据的话，用 2 个字节来表示：DataLow、DataHigh)

Count (8 bits): A data byte containing the length of a block operation.(在 block 操作总，表示数据长度)

[..]: Data sent by I2C device, as opposed to data sent by the host adapter.(中括号表示 I2C 设备发送的数据，没有中括号表示 host adapter 发送的数据)
```



#### (2) SMBus Quick Command

![image-20250416095231571](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416095231571.png)

作用：

只是用来发送一位数据：R/W#本意是用来表示读或写，但是在 SMBus 里可以用来表示其他含义。比如某些开关设备，可以根据这一位来决定是打开还是关闭。

**Functionality flag: I2C_FUNC_SMBUS_QUICK**



#### (3) SMBus Receive Byte

![image-20250416095354494](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416095354494.png)

作用：

I2C-tools 中的函数：i2c_smbus_read_byte()。读取一个字节，Host adapter 接收到一个字节后不需要发出回应信号(上图中 N 表示不回应)。

**Functionality flag: I2C_FUNC_SMBUS_READ_BYTE**



#### (4) SMBus Send Byte

![image-20250416095707071](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416095707071.png)

作用：

I2C-tools 中的函数：i2c_smbus_write_byte()。发送一个字节。

**Functionality flag: I2C_FUNC_SMBUS_WRITE_BYTE**



#### (5) SMBus Read Byte

![image-20250416095834204](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416095834204.png)

作用：

I2C-tools 中的函数：i2c_smbus_read_byte_data()。先发出 Command Code(它一般表示芯片内部的寄存器地址)，再读取一个字节的数据。上面介绍的 SMBus Receive Byte 是不发送 Comand，直接读取数据。

**Functionality flag: I2C_FUNC_SMBUS_READ_BYTE_DATA**



#### (6) SMBus Read Word

![image-20250416100017814](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416100017814.png)

作用：

I2C-tools 中的函数：i2c_smbus_read_word_data()。先发出 Command Code(它一般表示芯片内部的寄存器地址)，再读取 2 个字节的数据。

**Functionality flag: I2C_FUNC_SMBUS_READ_WORD_DATA**



#### (7) SMBus Write Byte

![image-20250416100210813](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416100210813.png)

作用：

I2C-tools 中 的 函 数 ： i2c_smbus_write_byte_data() 。 先 发 出Command Code(它一般表示芯片内部的寄存器地址)，再发出 1 个字节的数据。

**Functionality flag: I2C_FUNC_SMBUS_WRITE_BYTE_DATA**



#### (8) SMBus Write Word

![image-20250416100321957](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416100321957.png)

作用：

I2C-tools 中 的 函 数 ： i2c_smbus_write_word_data() 。 先 发 出Command Code(它一般表示芯片内部的寄存器地址)，再发出 1 个字节的数据。

**Functionality flag: I2C_FUNC_SMBUS_WRITE_WORD_DATA**



#### (9) SMBus Block Read

![image-20250416100424494](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416100424494.png)

作用：

I2C-tools 中 的 函 数 ： i2c_smbus_read_block_data() 。 先 发 出Command Code(它一般表示芯片内部的寄存器地址)，再发起度操作：

- 先读到一个字节(Block Count)，表示后续要读的字节数

- 然后读取全部数据

**Functionality flag: I2C_FUNC_SMBUS_READ_BLOCK_DATA**

注意：

这里的block read中的count值是从设备上报的，而不是主设备高速从设备的！





#### (10) SMBus Block Write

![image-20250416100833798](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416100833798.png)

作用：

I2C-tools 中 的 函 数 ： i2c_smbus_write_block_data() 。先发出Command Code(它一般表示芯片内部的寄存器地址)，再发出 1 个字节的 Byte Conut(表示后续要发出的数据字节数)，最后发出全部数据。

**Functionality flag: I2C_FUNC_SMBUS_WRITE_BLOCK_DATA**



#### (11) I2C Block Read

在一般的 I2C 协议中，也可以连续读出多个字节。它跟 SMBus Block Read 的差别在于设备发出的第 1 个数据不是长度 N，如下图所示：

![image-20250416105619412](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416105619412.png)

I2C-tools 中的函数：i2c_smbus_read_i2c_block_data()。先发出Command Code(它一般表示芯片内部的寄存器地址)，再发出 1 个字节的 Byte Conut(表示后续要发出的数据字节数)，最后发出全部数据。

**Functionality flag: I2C_FUNC_SMBUS_READ_I2C_BLOCK**



#### (12) I2C Block Write

在一般的 I2C 协议中，也可以连续发出多个字节。它跟 SMBus Block Write 的差别在于发出的第 1 个数据不是长度 N，如下图所示：

![image-20250416105742956](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416105742956.png)

I2C-tools 中的函数：i2c_smbus_write_i2c_block_data()。先发出Command Code(它一般表示芯片内部的寄存器地址)，再发出 1 个字节的 Byte Conut(表示后续要发出的数据字节数)，最后发出全部数据。

**Functionality flag: I2C_FUNC_SMBUS_WRITE_I2C_BLOCK**

 

#### (13) SMBus Block Write - Block Read Process Call

![image-20250416105851243](E:\embedded_learning\embedded_learning_code_management\src\014_i2c_tst\image-20250416105851243.png)



先写一块数据，再读一块数据。

**Functionality flag: I2C_FUNC_SMBUS_BLOCK_PROC_CALL**



#### (14) Packet Error Checking (PEC)

PEC 是一种错误校验码，如果使用 PEC，那么在 P 信号之前，数据发送方要发送一个字节的 PEC 码(它是 CRC-8 码)。以 SMBus Send Byte 为例，下图中，一个未使用 PEC，另一个使用 PEC：

![image-20250416110052860](C:\Users\Jianyuan Sun\AppData\Roaming\Typora\typora-user-images\image-20250416110052860.png)









#### **() 读取电池电量**示例

```apl
主设备（Host） → [地址0x16 + Write] → 命令码0x0F（读电量） → [地址0x16 + Read] → 接收电量数据
```







### **2、关键操作**

- **Quick Command**：仅发送地址+R/W位，无数据（用于快速控制）。
- **Send/Receive Byte**：单字节读写。
- **Block Read/Write**：多字节传输（首字节为长度）。

### **3、超时与重试**

- **总线超时**：若SCL低电平持续>35ms，主设备必须复位总线。
- **从设备超时**：从设备需在10ms内响应，否则主设备终止通信。



## 五、设计注意事项

1. **上拉电阻**：根据总线电容选择（通常4.7kΩ~10kΩ）。
2. **电平兼容**：混合电压系统需电平转换（如1.8V传感器与3.3V主机）。
3. **PEC启用**：高可靠性场景建议启用CRC校验。
4. **中断处理**：SMBA#线需配置为边沿触发中断。





## 六、常见问题

**Q1：SMBus能否与I²C设备混用？**

- 可以，但需确保I²C设备支持SMBus超时和电压要求（如不支持的设备可能导致总线锁死）。

**Q2：如何调试SMBus通信失败？**

- 检查波形（SCL/SDA）是否合规。
- 确认从设备地址和命令码正确。
- 测量上拉电阻是否合适（过大会导致上升沿缓慢）。

**Q3：SMBus的Alert#和I²C的通用IO有何区别？**

- Alert#是专用于从设备中断的线，支持多设备共享（线与逻辑），而I²C需自行实现中断协议。



## 七、总结

SMBus是I²C的“严格子集”，专为系统管理优化，特点包括：

- **严格的时序和超时**确保可靠性。
- **Alert#中断**简化事件处理。
- **PEC校验**提升数据完整性。
  适用于对稳定性和功耗敏感的场景，如电池管理、硬件监控等。

因为很多设备都实现了 SMBus，而不是更宽泛的 I2C 协议，所以优先使用SMBus。即使 I2C 控制器没有实现 SMBus，软件方面也是可以使用 I2C 协议来模拟 SMBus。所以：**Linux 建议优先使用 SMBus**。



