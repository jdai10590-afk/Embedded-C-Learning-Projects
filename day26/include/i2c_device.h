#ifndef I2C_REGISTER_DEVICE_H
#define I2C_REGISTER_DEVICE_H

#include <stddef.h>
#include <stdint.h>

#include "i2c.h"

/*
 * 模拟 I2C 温度传感器的内部寄存器表。
 */
#define I2C_DEVICE_REG_ID           0x00U
#define I2C_DEVICE_REG_STATUS       0x01U
#define I2C_DEVICE_REG_TEMP_HIGH    0x02U
#define I2C_DEVICE_REG_TEMP_LOW     0x03U
#define I2C_DEVICE_REG_CONFIG       0x04U

/*
 * 当前设备共有 5 个寄存器：
 *
 * 0x00～0x04
 */
#define I2C_DEVICE_REGISTER_COUNT   5U

/*
 * 模拟设备 ID。
 */
#define I2C_DEVICE_EXPECTED_ID      0x42U

/*
 * STATUS 寄存器 bit0：
 *
 * 1：温度数据已经准备完成
 * 0：温度数据尚未准备完成
 */
#define I2C_DEVICE_STATUS_READY     0x01U

/*
 * CONFIG 寄存器默认值。
 */
#define I2C_DEVICE_DEFAULT_CONFIG   0x00U

/*
 * 默认温度原始值：
 *
 * 2534 / 100.0 = 25.34 摄氏度
 *
 * 2534 = 0x09E6
 *
 * TEMP_HIGH = 0x09
 * TEMP_LOW  = 0xE6
 */
#define I2C_DEVICE_DEFAULT_TEMP_RAW 2534U

/*
 * 温度换算比例。
 *
 * 一个原始单位表示 0.01 摄氏度。
 */
#define I2C_DEVICE_TEMP_SCALE       100.0f

/*
 * 带内部寄存器的模拟 I2C 设备。
 */
typedef struct
{
    /*
     * Day25 定义的基础 I2C 从机对象。
     *
     * 其中包含：
     *
     * address
     * stored_data
     * read_data
     *
     * 总线层使用这个对象完成：
     *
     * 地址匹配
     * ACK/NACK
     * 单字节发送和接收
     */
    I2cDevice bus_device;

    /*
     * 模拟设备内部寄存器。
     *
     * registers[0]：DEVICE_ID
     * registers[1]：STATUS
     * registers[2]：TEMP_HIGH
     * registers[3]：TEMP_LOW
     * registers[4]：CONFIG
     */
    uint8_t registers[
        I2C_DEVICE_REGISTER_COUNT];

    /*
     * 当前寄存器指针。
     *
     * 主机先向设备写入寄存器地址，
     * 设备会把该地址保存到这里。
     */
    uint8_t register_pointer;

    /*
     * 寄存器指针是否有效。
     *
     * 0：主机尚未设置寄存器地址
     * 1：寄存器指针已经设置
     */
    uint8_t register_pointer_valid;

    /*
     * 成功完成的寄存器读取次数。
     *
     * 连续读取两个寄存器时增加 2。
     */
    uint32_t register_read_count;

    /*
     * 成功完成的寄存器写入次数。
     */
    uint32_t register_write_count;
} I2cRegisterDevice;

/*
 * 判断寄存器地址是否合法。
 *
 * 当前合法范围：
 *
 * 0x00～0x04
 *
 * 合法返回 1，非法返回 0。
 */
int i2c_device_register_is_valid(
    uint8_t register_address);

/*
 * 判断寄存器是否允许写入。
 *
 * 当前只有 CONFIG 寄存器 0x04
 * 允许主机写入。
 *
 * 可写返回 1，只读返回 0。
 */
int i2c_device_register_is_writable(
    uint8_t register_address);

/*
 * 初始化带寄存器的 I2C 设备。
 *
 * 初始化内容：
 *
 * 设备地址设置为 address
 * DEVICE_ID 设置为 0x42
 * STATUS 设置为 READY
 * 默认温度设置为 25.34 摄氏度
 * CONFIG 设置为 0x00
 * 寄存器指针设为无效
 * 读写计数清零
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_init(
    I2cRegisterDevice *device,
    uint8_t address);

/*
 * 修改模拟设备的温度原始值。
 *
 * 例如：
 *
 * temperature_raw = 2534
 *
 * TEMP_HIGH = 0x09
 * TEMP_LOW  = 0xE6
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_set_temperature_raw(
    I2cRegisterDevice *device,
    uint16_t temperature_raw);

/*
 * 通过完整 I2C 事务写入一个寄存器。
 *
 * 事务：
 *
 * START
 * Address + WRITE
 * Register Address
 * Register Data
 * STOP
 *
 * 当前只允许写 CONFIG 寄存器。
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_write_register(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t register_address,
    uint8_t register_value);

/*
 * 通过 Repeated START 读取一个寄存器。
 *
 * 事务：
 *
 * START
 * Address + WRITE
 * Register Address
 * Repeated START
 * Address + READ
 * Register Data
 * Master NACK
 * STOP
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_read_register(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t register_address,
    uint8_t *register_value);

/*
 * 从指定寄存器开始连续读取多个字节。
 *
 * 读取过程中寄存器指针自动递增。
 *
 * 中间数据字节后主机发送 ACK，
 * 最后一个数据字节后主机发送 NACK。
 *
 * 例如从 0x02 连续读取两个字节：
 *
 * 0x02：TEMP_HIGH
 * 0x03：TEMP_LOW
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_read_registers(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t start_register,
    uint8_t *buffer,
    size_t length);

/*
 * 连续读取温度高字节和低字节，
 * 然后组合并换算温度。
 *
 * temperature_raw：
 * 保存 16 位温度原始值。
 *
 * temperature_c：
 * 保存摄氏温度。
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_register_device_read_temperature(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint16_t *temperature_raw,
    float *temperature_c);

#endif