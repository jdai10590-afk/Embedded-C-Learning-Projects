#ifndef I2C_SIM_H
#define I2C_SIM_H

#include <stdint.h>

/*
 * I2C 标准模式时钟频率：
 *
 * 100 kHz = 100000 Hz
 */
#define I2C_STANDARD_CLOCK_HZ          100000U

/*
 * 7 位 I2C 地址的最大数值。
 *
 * 7 位全部为 1：
 * 111 1111 = 0x7F
 */
#define I2C_7BIT_ADDRESS_MAX           0x7FU

/*
 * 生成地址字节时，
 * 7 位设备地址需要左移 1 位。
 */
#define I2C_ADDRESS_SHIFT              1U

/*
 * 地址字节最低位是读写方向位。
 */
#define I2C_RW_MASK                    0x01U

/*
 * Day25 模拟从机的默认 7 位地址。
 */
#define I2C_DEVICE_DEFAULT_ADDRESS     0x48U

/*
 * 模拟设备初始化后保存的数据。
 */
#define I2C_DEVICE_DEFAULT_STORED_DATA 0x00U

/*
 * 主机读取设备时，
 * 模拟从机默认返回的数据。
 */
#define I2C_DEVICE_DEFAULT_READ_DATA   0x5AU

/*
 * I2C 通信方向。
 *
 * 地址字节 bit0：
 *
 * 0：主机写入从机
 * 1：主机读取从机
 */
typedef enum
{
    I2C_DIRECTION_WRITE = 0,
    I2C_DIRECTION_READ = 1
} I2cDirection;

/*
 * I2C 应答状态。
 *
 * ACK 对应 SDA 被接收方拉低。
 * NACK 对应 SDA 保持高电平。
 */
typedef enum
{
    I2C_RESPONSE_ACK = 0,
    I2C_RESPONSE_NACK = 1
} I2cResponse;

/*
 * I2C 总线软件状态。
 */
typedef struct
{
    /*
     * I2C 时钟频率，单位 Hz。
     */
    uint32_t clock_hz;

    /*
     * 是否已经产生 START。
     *
     * 0：总线空闲
     * 1：事务已经开始
     */
    uint8_t started;

    /*
     * 是否已有从机成功响应地址。
     *
     * 0：地址尚未匹配
     * 1：从机已经返回 ACK
     */
    uint8_t addressed;

    /*
     * 当前主机发送的 7 位设备地址。
     */
    uint8_t current_address;

    /*
     * 当前通信方向。
     */
    I2cDirection direction;

    /*
     * 普通 START 产生次数。
     */
    uint32_t start_count;

    /*
     * Repeated START 产生次数。
     *
     * 当前事务没有经过 STOP，
     * 主机再次产生 START 时增加。
     */
    uint32_t repeated_start_count;

    /*
     * STOP 产生次数。
     */
    uint32_t stop_count;

    /*
     * 总线收到或发送的 ACK 次数。
     */
    uint32_t ack_count;

    /*
     * 总线收到或发送的 NACK 次数。
     */
    uint32_t nack_count;

    /*
     * 完成的 8 位地址或数据字段数量。
     *
     * ACK/NACK 是第 9 个时钟，
     * 不计入 byte_count。
     */
    uint32_t byte_count;
} I2cBus;

/*
 * Day25 使用的简单模拟 I2C 从机。
 */
typedef struct
{
    /*
     * 设备的 7 位地址。
     */
    uint8_t address;

    /*
     * 主机写入设备后，
     * 用于保存写入数据。
     */
    uint8_t stored_data;

    /*
     * 主机读取设备时，
     * 从机准备返回的数据。
     */
    uint8_t read_data;
} I2cDevice;

/*
 * 判断通信方向是否合法。
 *
 * WRITE 或 READ 返回 1，
 * 其他值返回 0。
 */
int i2c_direction_is_valid(
    I2cDirection direction);

/*
 * 判断 ACK/NACK 参数是否合法。
 *
 * ACK 或 NACK 返回 1，
 * 其他值返回 0。
 */
int i2c_response_is_valid(
    I2cResponse response);

/*
 * 判断是否为有效的 7 位地址。
 *
 * 本次基础模拟允许 0x00～0x7F。
 *
 * 合法返回 1，非法返回 0。
 */
int i2c_address_is_valid(
    uint8_t address);

/*
 * 根据 7 位设备地址和通信方向，
 * 生成总线上发送的 8 位地址字节。
 *
 * 例如：
 *
 * 地址 0x48 + WRITE：
 * (0x48 << 1) | 0 = 0x90
 *
 * 地址 0x48 + READ：
 * (0x48 << 1) | 1 = 0x91
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_build_address_byte(
    uint8_t address,
    I2cDirection direction,
    uint8_t *address_byte);

/*
 * 初始化 I2C 总线。
 *
 * 初始化后：
 *
 * started = 0
 * addressed = 0
 * 所有统计计数清零
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_init(
    I2cBus *bus,
    uint32_t clock_hz);

/*
 * 初始化模拟 I2C 从机。
 *
 * address：
 * 设备的 7 位地址。
 *
 * read_data：
 * 主机读取设备时，设备准备返回的数据。
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_device_init(
    I2cDevice *device,
    uint8_t address,
    uint8_t read_data);

/*
 * 产生普通 START 起始信号。
 *
 * 普通 START 只允许在总线空闲时产生：
 *
 * started = 0
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_start(
    I2cBus *bus);

/*
 * 产生 Repeated START 重复起始信号。
 *
 * 只允许在当前事务已经开始时产生：
 *
 * started = 1
 *
 * 成功后：
 *
 * started 保持为 1
 * addressed 清零
 * current_address 清零
 * repeated_start_count 加 1
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_repeated_start(
    I2cBus *bus);

/*
 * 产生 STOP 停止信号。
 *
 * STOP 后总线恢复空闲：
 *
 * started = 0
 * addressed = 0
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_stop(
    I2cBus *bus);

/*
 * 主机发送 7 位地址和读写方向。
 *
 * 当发送地址与 device->address 相同时：
 *
 * response = ACK
 * addressed = 1
 *
 * 地址不匹配时：
 *
 * response = NACK
 * addressed = 0
 *
 * 函数返回值表示地址字节操作是否正常执行。
 * 地址不匹配产生 NACK 仍属于正常执行，
 * 因此函数仍返回 1。
 *
 * 参数或通信顺序错误时返回 0。
 */
int i2c_send_address(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t address,
    I2cDirection direction,
    I2cResponse *response);

/*
 * 主机向已经匹配的从机写入一个字节。
 *
 * 要求：
 *
 * 已经产生 START
 * 地址已经匹配
 * 当前方向为 WRITE
 *
 * 写入成功后：
 *
 * device->stored_data = data
 * response = ACK
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_write_byte(
    I2cBus *bus,
    I2cDevice *device,
    uint8_t data,
    I2cResponse *response);

/*
 * 主机从已经匹配的从机读取一个字节。
 *
 * 要求：
 *
 * 已经产生 START
 * 地址已经匹配
 * 当前方向为 READ
 *
 * data：
 * 保存从机返回的数据。
 *
 * master_response：
 * 主机读取完成后发送的 ACK 或 NACK。
 *
 * 单字节读取通常使用 NACK，
 * 表示主机不再继续读取后续字节。
 *
 * 成功返回 1，失败返回 0。
 */
int i2c_read_byte(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t *data,
    I2cResponse master_response);

#endif