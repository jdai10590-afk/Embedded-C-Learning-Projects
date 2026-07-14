#include <stddef.h>

#include "i2c_device.h"

/*
 * 当 I2C 事务执行到一半失败时，
 * 尝试发送 STOP，使总线恢复空闲状态。
 */
static void i2c_register_device_abort_transaction(
    I2cBus *bus,
    I2cRegisterDevice *device)
{
    /*
     * 失败后，当前寄存器指针不再可信。
     */
    if(device != NULL)
    {
        device->register_pointer_valid = 0U;
    }

    /*
     * 只有事务已经开始时才需要 STOP。
     */
    if((bus != NULL) &&
       (bus->started != 0U))
    {
        (void)i2c_stop(bus);
    }
}

int i2c_device_register_is_valid(
    uint8_t register_address)
{
    /*
     * 当前寄存器地址范围：
     *
     * 0x00～0x04
     */
    if(register_address <
       I2C_DEVICE_REGISTER_COUNT)
    {
        return 1;
    }

    return 0;
}

int i2c_device_register_is_writable(
    uint8_t register_address)
{
    /*
     * 当前只有 CONFIG 寄存器允许写入。
     */
    if(register_address ==
       I2C_DEVICE_REG_CONFIG)
    {
        return 1;
    }

    return 0;
}

int i2c_register_device_set_temperature_raw(
    I2cRegisterDevice *device,
    uint16_t temperature_raw)
{
    if(device == NULL)
    {
        return 0;
    }

    /*
     * 提取 16 位温度原始值的高字节。
     *
     * 例如：
     *
     * temperature_raw = 0x09E6
     *
     * 0x09E6 >> 8 = 0x0009
     */
    device->registers[
        I2C_DEVICE_REG_TEMP_HIGH] =
        (uint8_t)(
            (temperature_raw >> 8) &
            0x00FFU);

    /*
     * 提取低字节。
     *
     * 0x09E6 & 0x00FF = 0x00E6
     */
    device->registers[
        I2C_DEVICE_REG_TEMP_LOW] =
        (uint8_t)(
            temperature_raw &
            0x00FFU);

    /*
     * 设置温度数据就绪标志。
     */
    device->registers[
        I2C_DEVICE_REG_STATUS] |=
        I2C_DEVICE_STATUS_READY;

    return 1;
}

int i2c_register_device_init(
    I2cRegisterDevice *device,
    uint8_t address)
{
    size_t index;

    if(device == NULL)
    {
        return 0;
    }

    /*
     * 使用 Day25 的基础接口初始化从机地址。
     *
     * read_data 暂时设置为 0，
     * 之后每次读取寄存器前会根据寄存器内容更新。
     */
    if(i2c_device_init(
           &device->bus_device,
           address,
           0U) == 0)
    {
        return 0;
    }

    /*
     * 先清空全部模拟寄存器。
     */
    for(index = 0U;
        index < I2C_DEVICE_REGISTER_COUNT;
        index++)
    {
        device->registers[index] = 0U;
    }

    /*
     * 设置固定设备 ID。
     */
    device->registers[
        I2C_DEVICE_REG_ID] =
        I2C_DEVICE_EXPECTED_ID;

    /*
     * 设置状态寄存器初始值。
     */
    device->registers[
        I2C_DEVICE_REG_STATUS] =
        I2C_DEVICE_STATUS_READY;

    /*
     * 设置配置寄存器初始值。
     */
    device->registers[
        I2C_DEVICE_REG_CONFIG] =
        I2C_DEVICE_DEFAULT_CONFIG;

    /*
     * 设置默认温度：
     *
     * 2534 = 25.34 摄氏度
     */
    if(i2c_register_device_set_temperature_raw(
           device,
           I2C_DEVICE_DEFAULT_TEMP_RAW) == 0)
    {
        return 0;
    }

    /*
     * 初始化时，主机尚未选择任何寄存器。
     */
    device->register_pointer = 0U;
    device->register_pointer_valid = 0U;

    /*
     * 清零寄存器访问计数。
     */
    device->register_read_count = 0U;
    device->register_write_count = 0U;

    return 1;
}

int i2c_register_device_write_register(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t register_address,
    uint8_t register_value)
{
    I2cResponse response;

    response = I2C_RESPONSE_NACK;

    if((bus == NULL) ||
       (device == NULL))
    {
        return 0;
    }

    /*
     * 只允许从总线空闲状态开始一个完整事务。
     */
    if(bus->started != 0U)
    {
        return 0;
    }

    /*
     * 检查寄存器地址。
     */
    if(i2c_device_register_is_valid(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 检查写权限。
     *
     * DEVICE_ID、STATUS 和温度寄存器均为只读。
     */
    if(i2c_device_register_is_writable(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 第一步：START。
     */
    if(i2c_start(bus) == 0)
    {
        return 0;
    }

    /*
     * 第二步：发送设备地址和 WRITE 方向。
     */
    if(i2c_send_address(
           bus,
           &device->bus_device,
           device->bus_device.address,
           I2C_DIRECTION_WRITE,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 第三步：发送寄存器地址。
     *
     * i2c_write_byte() 会先把该字节保存到
     * bus_device.stored_data 中。
     */
    if(i2c_write_byte(
           bus,
           &device->bus_device,
           register_address,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 设备保存主机指定的寄存器地址。
     */
    device->register_pointer =
        register_address;

    device->register_pointer_valid = 1U;

    /*
     * 第四步：发送要写入的寄存器数据。
     */
    if(i2c_write_byte(
           bus,
           &device->bus_device,
           register_value,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 第五步：STOP。
     */
    if(i2c_stop(bus) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 只有整个事务成功后，
     * 才真正更新模拟设备寄存器。
     */
    device->registers[
        register_address] =
        register_value;

    device->register_write_count++;

    return 1;
}

int i2c_register_device_read_registers(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t start_register,
    uint8_t *buffer,
    size_t length)
{
    I2cResponse response;
    I2cResponse master_response;
    uint8_t received_data;
    size_t index;
    size_t remaining_registers;

    response = I2C_RESPONSE_NACK;
    master_response = I2C_RESPONSE_NACK;
    received_data = 0U;

    if((bus == NULL) ||
       (device == NULL) ||
       (buffer == NULL))
    {
        return 0;
    }

    /*
     * 读取长度不能为 0。
     */
    if(length == 0U)
    {
        return 0;
    }

    /*
     * 必须从总线空闲状态开始。
     */
    if(bus->started != 0U)
    {
        return 0;
    }

    /*
     * 起始寄存器必须合法。
     */
    if(i2c_device_register_is_valid(
           start_register) == 0)
    {
        return 0;
    }

    /*
     * 检查连续读取是否超出寄存器数组范围。
     *
     * 例如从 0x03 开始：
     *
     * 剩余合法寄存器为 0x03、0x04，
     * 所以最多读取 2 个字节。
     */
    remaining_registers =
        (size_t)(
            I2C_DEVICE_REGISTER_COUNT -
            start_register);

    if(length > remaining_registers)
    {
        return 0;
    }

    /*
     * 第一步：普通 START。
     */
    if(i2c_start(bus) == 0)
    {
        return 0;
    }

    /*
     * 第二步：发送设备地址和 WRITE 方向。
     *
     * 此阶段不是写寄存器数据，
     * 而是先告诉设备要读取哪个寄存器。
     */
    if(i2c_send_address(
           bus,
           &device->bus_device,
           device->bus_device.address,
           I2C_DIRECTION_WRITE,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 第三步：发送起始寄存器地址。
     */
    if(i2c_write_byte(
           bus,
           &device->bus_device,
           start_register,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 保存寄存器指针。
     */
    device->register_pointer =
        start_register;

    device->register_pointer_valid = 1U;

    /*
     * 第四步：Repeated START。
     *
     * 总线没有经过 STOP，
     * 事务保持连续。
     */
    if(i2c_repeated_start(bus) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 第五步：重新发送设备地址，
     * 这一次使用 READ 方向。
     */
    if(i2c_send_address(
           bus,
           &device->bus_device,
           device->bus_device.address,
           I2C_DIRECTION_READ,
           &response) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    if(response != I2C_RESPONSE_ACK)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    /*
     * 第六步：连续读取寄存器数据。
     */
    for(index = 0U;
        index < length;
        index++)
    {
        if(device->register_pointer_valid == 0U)
        {
            i2c_register_device_abort_transaction(
                bus,
                device);

            return 0;
        }

        /*
         * 把当前寄存器值放入基础从机对象，
         * 供 i2c_read_byte() 返回给主机。
         */
        device->bus_device.read_data =
            device->registers[
                device->register_pointer];

        /*
         * 中间字节发送 ACK，
         * 表示还要继续读取。
         *
         * 最后一个字节发送 NACK，
         * 表示本次读取结束。
         */
        if((index + 1U) < length)
        {
            master_response =
                I2C_RESPONSE_ACK;
        }
        else
        {
            master_response =
                I2C_RESPONSE_NACK;
        }

        if(i2c_read_byte(
               bus,
               &device->bus_device,
               &received_data,
               master_response) == 0)
        {
            i2c_register_device_abort_transaction(
                bus,
                device);

            return 0;
        }

        buffer[index] = received_data;

        device->register_read_count++;

        /*
         * 模拟寄存器指针自动递增。
         */
        if(device->register_pointer <
           (I2C_DEVICE_REGISTER_COUNT - 1U))
        {
            device->register_pointer++;
        }
        else
        {
            /*
             * 当前已经读取到最后一个寄存器，
             * 不允许指针继续越界。
             */
            device->register_pointer_valid = 0U;
        }
    }

    /*
     * 第七步：STOP，结束完整事务。
     */
    if(i2c_stop(bus) == 0)
    {
        i2c_register_device_abort_transaction(
            bus,
            device);

        return 0;
    }

    return 1;
}

int i2c_register_device_read_register(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t register_address,
    uint8_t *register_value)
{
    /*
     * 单寄存器读取复用连续读取接口，
     * 读取长度为 1。
     */
    return i2c_register_device_read_registers(
        bus,
        device,
        register_address,
        register_value,
        1U);
}

int i2c_register_device_read_temperature(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint16_t *temperature_raw,
    float *temperature_c)
{
    uint8_t temperature_bytes[2];

    temperature_bytes[0] = 0U;
    temperature_bytes[1] = 0U;

    if((temperature_raw == NULL) ||
       (temperature_c == NULL))
    {
        return 0;
    }

    /*
     * 从 TEMP_HIGH 开始连续读取两个寄存器：
     *
     * temperature_bytes[0] = TEMP_HIGH
     * temperature_bytes[1] = TEMP_LOW
     */
    if(i2c_register_device_read_registers(
           bus,
           device,
           I2C_DEVICE_REG_TEMP_HIGH,
           temperature_bytes,
           2U) == 0)
    {
        return 0;
    }

    /*
     * 合并高低字节：
     *
     * high = 0x09
     * low  = 0xE6
     *
     * (0x09 << 8) | 0xE6
     * = 0x09E6
     * = 2534
     */
    *temperature_raw =
        (uint16_t)(
            ((uint16_t)
                temperature_bytes[0] << 8) |
            (uint16_t)
                temperature_bytes[1]);

    /*
     * 原始值换算为摄氏温度。
     */
    *temperature_c =
        (float)(*temperature_raw) /
        I2C_DEVICE_TEMP_SCALE;

    return 1;
}