#include <stddef.h>

#include "i2c.h"

int i2c_direction_is_valid(
    I2cDirection direction)
{
    /*
     * I2C 通信方向只有两种：
     *
     * 0：WRITE
     * 1：READ
     */
    if((direction == I2C_DIRECTION_WRITE) ||
       (direction == I2C_DIRECTION_READ))
    {
        return 1;
    }

    return 0;
}

int i2c_response_is_valid(
    I2cResponse response)
{
    /*
     * I2C 应答状态只有两种：
     *
     * 0：ACK
     * 1：NACK
     */
    if((response == I2C_RESPONSE_ACK) ||
       (response == I2C_RESPONSE_NACK))
    {
        return 1;
    }

    return 0;
}

int i2c_address_is_valid(
    uint8_t address)
{
    /*
     * 7 位地址的数值范围为：
     *
     * 0x00～0x7F
     *
     * Day25 暂时只进行 7 位范围检查，
     * 尚未排除 I2C 规范中的保留地址。
     */
    if(address <= I2C_7BIT_ADDRESS_MAX)
    {
        return 1;
    }

    return 0;
}

int i2c_build_address_byte(
    uint8_t address,
    I2cDirection direction,
    uint8_t *address_byte)
{
    if(address_byte == NULL)
    {
        return 0;
    }

    if(i2c_address_is_valid(address) == 0)
    {
        return 0;
    }

    if(i2c_direction_is_valid(direction) == 0)
    {
        return 0;
    }

    /*
     * 地址字节格式：
     *
     * bit7～bit1：7 位设备地址
     * bit0：读写方向
     *
     * 例如地址 0x48：
     *
     * WRITE：
     * (0x48 << 1) | 0 = 0x90
     *
     * READ：
     * (0x48 << 1) | 1 = 0x91
     */
    *address_byte =
        (uint8_t)(
            (uint8_t)(
                address <<
                I2C_ADDRESS_SHIFT) |
            ((uint8_t)direction &
             I2C_RW_MASK));

    return 1;
}

int i2c_init(
    I2cBus *bus,
    uint32_t clock_hz)
{
    if(bus == NULL)
    {
        return 0;
    }

    /*
     * I2C 是同步通信。
     *
     * 时钟频率为 0 时，
     * 无法产生有效 SCL。
     */
    if(clock_hz == 0U)
    {
        return 0;
    }

    bus->clock_hz = clock_hz;

    /*
     * 初始化后总线为空闲状态。
     */
    bus->started = 0U;
    bus->addressed = 0U;
    bus->current_address = 0U;
    bus->direction = I2C_DIRECTION_WRITE;

    /*
     * 所有统计数据清零。
     */
    bus->start_count = 0U;
    bus->repeated_start_count = 0U;
    bus->stop_count = 0U;
    bus->ack_count = 0U;
    bus->nack_count = 0U;
    bus->byte_count = 0U;

    return 1;
}

int i2c_device_init(
    I2cDevice *device,
    uint8_t address,
    uint8_t read_data)
{
    if(device == NULL)
    {
        return 0;
    }

    /*
     * 从机地址必须属于 7 位地址范围。
     */
    if(i2c_address_is_valid(address) == 0)
    {
        return 0;
    }

    device->address = address;

    /*
     * 初始化时，还没有接收到主机写入的数据。
     */
    device->stored_data =
        I2C_DEVICE_DEFAULT_STORED_DATA;

    /*
     * 设置主机读取时，从机准备返回的数据。
     */
    device->read_data = read_data;

    return 1;
}

int i2c_start(
    I2cBus *bus)
{
    if(bus == NULL)
    {
        return 0;
    }

    /*
     * 普通 START 只允许在总线空闲时产生。
     *
     * 当前事务已经开始时，
     * 应调用 i2c_repeated_start()。
     */
    if(bus->started != 0U)
    {
        return 0;
    }

    /*
     * 产生 START 后，事务开始。
     */
    bus->started = 1U;

    /*
     * 新事务开始后，
     * 主机尚未发送设备地址。
     */
    bus->addressed = 0U;
    bus->current_address = 0U;
    bus->direction = I2C_DIRECTION_WRITE;

    bus->start_count++;

    return 1;
}

int i2c_repeated_start(
    I2cBus *bus)
{
    if(bus == NULL)
    {
        return 0;
    }

    /*
     * Repeated START 只能在一个尚未结束的
     * I2C 事务中产生。
     */
    if(bus->started == 0U)
    {
        return 0;
    }

    /*
     * 当前事务仍然继续，
     * 所以 started 保持为 1。
     */
    bus->started = 1U;

    /*
     * Repeated START 后必须重新发送
     * 设备地址和读写方向。
     */
    bus->addressed = 0U;
    bus->current_address = 0U;
    bus->direction = I2C_DIRECTION_WRITE;

    bus->repeated_start_count++;

    return 1;
}

int i2c_stop(
    I2cBus *bus)
{
    if(bus == NULL)
    {
        return 0;
    }

    /*
     * 没有 START 时不能直接产生 STOP。
     */
    if(bus->started == 0U)
    {
        return 0;
    }

    /*
     * STOP 表示当前事务结束，
     * 总线恢复空闲。
     */
    bus->started = 0U;
    bus->addressed = 0U;
    bus->current_address = 0U;
    bus->direction = I2C_DIRECTION_WRITE;

    bus->stop_count++;

    return 1;
}

int i2c_send_address(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t address,
    I2cDirection direction,
    I2cResponse *response)
{
    uint8_t address_byte;

    if((bus == NULL) ||
       (device == NULL) ||
       (response == NULL))
    {
        return 0;
    }

    /*
     * 必须先产生 START，
     * 才能发送设备地址。
     */
    if(bus->started == 0U)
    {
        return 0;
    }

    if(i2c_address_is_valid(address) == 0)
    {
        return 0;
    }

    if(i2c_direction_is_valid(direction) == 0)
    {
        return 0;
    }

    /*
     * 生成实际在总线上发送的 8 位地址字节。
     *
     * 当前函数不直接输出 address_byte，
     * 但仍生成它，以模拟真实地址发送过程。
     */
    if(i2c_build_address_byte(
           address,
           direction,
           &address_byte) == 0)
    {
        return 0;
    }

    /*
     * 地址字节属于一个完整的 8 位字段，
     * 所以 byte_count 增加 1。
     */
    bus->byte_count++;

    /*
     * 保存主机发送的 7 位地址和方向。
     */
    bus->current_address = address;
    bus->direction = direction;

    /*
     * 地址匹配：
     *
     * 从机拉低 SDA，返回 ACK。
     */
    if(address == device->address)
    {
        bus->addressed = 1U;

        *response = I2C_RESPONSE_ACK;

        bus->ack_count++;

        return 1;
    }

    /*
     * 地址不匹配：
     *
     * 没有从机拉低 SDA，
     * 主机收到 NACK。
     *
     * 地址字节仍然正常发送完成，
     * 所以函数返回 1。
     */
    bus->addressed = 0U;

    *response = I2C_RESPONSE_NACK;

    bus->nack_count++;

    return 1;
}

int i2c_write_byte(
    I2cBus *bus,
    I2cDevice *device,
    uint8_t data,
    I2cResponse *response)
{
    if((bus == NULL) ||
       (device == NULL) ||
       (response == NULL))
    {
        return 0;
    }

    /*
     * 必须先产生 START。
     */
    if(bus->started == 0U)
    {
        return 0;
    }

    /*
     * 必须已经有从机响应地址。
     */
    if(bus->addressed == 0U)
    {
        return 0;
    }

    /*
     * 必须处于主机写方向。
     */
    if(bus->direction !=
       I2C_DIRECTION_WRITE)
    {
        return 0;
    }

    /*
     * 防止使用与当前地址不匹配的设备对象。
     */
    if(bus->current_address !=
       device->address)
    {
        return 0;
    }

    /*
     * 模拟从机成功接收主机写入的数据。
     */
    device->stored_data = data;

    /*
     * 一个数据字节发送完成。
     */
    bus->byte_count++;

    /*
     * 从机成功接收数据后返回 ACK。
     */
    *response = I2C_RESPONSE_ACK;

    bus->ack_count++;

    return 1;
}

int i2c_read_byte(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t *data,
    I2cResponse master_response)
{
    if((bus == NULL) ||
       (device == NULL) ||
       (data == NULL))
    {
        return 0;
    }

    /*
     * 主机发送的 ACK/NACK 必须合法。
     */
    if(i2c_response_is_valid(
           master_response) == 0)
    {
        return 0;
    }

    /*
     * 必须先产生 START。
     */
    if(bus->started == 0U)
    {
        return 0;
    }

    /*
     * 必须已有从机响应地址。
     */
    if(bus->addressed == 0U)
    {
        return 0;
    }

    /*
     * 必须处于主机读方向。
     */
    if(bus->direction !=
       I2C_DIRECTION_READ)
    {
        return 0;
    }

    /*
     * 防止读取错误的设备对象。
     */
    if(bus->current_address !=
       device->address)
    {
        return 0;
    }

    /*
     * 模拟从机通过 SDA 返回一个字节。
     */
    *data = device->read_data;

    /*
     * 从机发送了一个完整的 8 位数据字段。
     */
    bus->byte_count++;

    /*
     * 数据接收完成后，由主机发送 ACK 或 NACK。
     *
     * 单字节读取通常发送 NACK，
     * 表示不再继续读取。
     */
    if(master_response ==
       I2C_RESPONSE_ACK)
    {
        bus->ack_count++;
    }
    else
    {
        bus->nack_count++;
    }

    return 1;
}