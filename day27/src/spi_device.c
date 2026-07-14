#include <stddef.h>

#include "spi_device.h"

/*
 * SPI 事务异常结束时，尝试释放从机。
 *
 * 该函数只在本文件内部使用，
 * 所以使用 static 修饰。
 */
static void spi_device_abort_transaction(
    SpiBus *spi)
{
    if((spi != NULL) &&
       (spi->chip_selected != 0U))
    {
        /*
         * 这里不处理返回值，
         * 目的是尽量让 CS 恢复为高电平。
         */
        (void)spi_deselect(spi);
    }
}

int spi_device_register_is_valid(
    uint8_t register_address)
{
    /*
     * 当前寄存器数量为 5，
     * 因此合法地址是 0x00～0x04。
     */
    if(register_address <
       SPI_DEVICE_REGISTER_COUNT)
    {
        return 1;
    }

    return 0;
}

int spi_device_register_is_writable(
    uint8_t register_address)
{
    /*
     * 非法地址当然不能写入。
     */
    if(spi_device_register_is_valid(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 当前只有配置寄存器 0x04
     * 允许主机修改。
     */
    if(register_address ==
       SPI_DEVICE_REG_CONFIG)
    {
        return 1;
    }

    return 0;
}

int spi_device_set_temperature_raw(
    SpiDevice *device,
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
     * 2534 = 0x09E6
     *
     * 高字节为 0x09。
     */
    device->registers[
        SPI_DEVICE_REG_TEMP_HIGH] =
        (uint8_t)(
            (temperature_raw >> 8) &
            0xFFU);

    /*
     * 提取低字节。
     *
     * 2534 = 0x09E6
     *
     * 低字节为 0xE6。
     */
    device->registers[
        SPI_DEVICE_REG_TEMP_LOW] =
        (uint8_t)(
            temperature_raw &
            0xFFU);

    /*
     * 新的温度数据已经写入，
     * 将状态寄存器 bit0 置为 1。
     */
    device->registers[
        SPI_DEVICE_REG_STATUS] |=
        SPI_DEVICE_STATUS_READY;

    return 1;
}

int spi_device_init(SpiDevice *device)
{
    uint8_t index;

    if(device == NULL)
    {
        return 0;
    }

    /*
     * 先清空整个寄存器表，
     * 避免其中存在未初始化的随机值。
     */
    for(index = 0U;
        index < SPI_DEVICE_REGISTER_COUNT;
        index++)
    {
        device->registers[index] = 0U;
    }

    /*
     * 初始化固定设备 ID。
     */
    device->registers[
        SPI_DEVICE_REG_ID] =
        SPI_DEVICE_EXPECTED_ID;

    /*
     * 初始化状态寄存器。
     *
     * bit0 = 1 表示数据已就绪。
     */
    device->registers[
        SPI_DEVICE_REG_STATUS] =
        SPI_DEVICE_STATUS_READY;

    /*
     * 初始化配置寄存器。
     */
    device->registers[
        SPI_DEVICE_REG_CONFIG] =
        SPI_DEVICE_DEFAULT_CONFIG;

    /*
     * 初始化读写统计。
     */
    device->read_count = 0U;
    device->write_count = 0U;

    /*
     * 将初始温度 2534 写入
     * TEMP_HIGH 和 TEMP_LOW。
     */
    if(spi_device_set_temperature_raw(
           device,
           SPI_DEVICE_DEFAULT_TEMP_RAW) == 0)
    {
        return 0;
    }

    return 1;
}

int spi_device_read_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t *register_value)
{
    uint8_t command;
    uint8_t ignored_rx;

    /*
     * 检查所有必要指针。
     */
    if((spi == NULL) ||
       (device == NULL) ||
       (register_value == NULL))
    {
        return 0;
    }

    /*
     * 拒绝非法寄存器地址。
     */
    if(spi_device_register_is_valid(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 当前驱动要求总线处于空闲状态。
     *
     * 如果其他代码已经拉低 CS，
     * 本函数不会强行接管当前事务。
     */
    if(spi->chip_selected != 0U)
    {
        return 0;
    }

    /*
     * 构造读命令。
     *
     * bit7 = 1
     * bit6～bit0 = 寄存器地址
     *
     * 例如读取 0x02：
     * 0x02 | 0x80 = 0x82
     */
    command =
        register_address |
        SPI_DEVICE_READ_FLAG;

    /*
     * 开始完整 SPI 寄存器读取事务。
     *
     * CS 拉低后，在整个事务结束前
     * 始终保持低电平。
     */
    if(spi_select(spi) == 0)
    {
        return 0;
    }

    /*
     * 第一次交换：
     *
     * 主机发送读命令。
     * 从机暂时返回一个无效占位字节。
     */
    if(spi_set_slave_response(
           spi,
           SPI_DUMMY_BYTE) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    if(spi_transfer_byte(
           spi,
           command,
           &ignored_rx) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    /*
     * 从机解析完命令后，
     * 准备返回对应寄存器的值。
     */
    if(spi_set_slave_response(
           spi,
           device->registers[
               register_address]) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    /*
     * 第二次交换：
     *
     * 主机发送 Dummy Byte 产生时钟，
     * 同时读取真正的寄存器值。
     */
    if(spi_transfer_byte(
           spi,
           SPI_DUMMY_BYTE,
           register_value) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    /*
     * 完整事务结束，CS 拉高。
     */
    if(spi_deselect(spi) == 0)
    {
        return 0;
    }

    /*
     * 只有完整读取成功后，
     * 才增加寄存器读取次数。
     */
    device->read_count++;

    return 1;
}

int spi_device_write_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t register_value)
{
    uint8_t command;
    uint8_t ignored_rx;

    if((spi == NULL) ||
       (device == NULL))
    {
        return 0;
    }

    /*
     * 首先检查地址是否合法。
     */
    if(spi_device_register_is_valid(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 检查该寄存器是否允许写入。
     *
     * 当前只有 CONFIG 寄存器可写。
     */
    if(spi_device_register_is_writable(
           register_address) == 0)
    {
        return 0;
    }

    /*
     * 驱动要求总线当前处于空闲状态。
     */
    if(spi->chip_selected != 0U)
    {
        return 0;
    }

    /*
     * 构造写命令。
     *
     * bit7 = 0
     * bit6～bit0 = 寄存器地址
     *
     * 例如写入 0x04：
     * 0x04 & 0x7F = 0x04
     */
    command =
        register_address &
        SPI_DEVICE_ADDRESS_MASK;

    /*
     * 开始写寄存器事务。
     */
    if(spi_select(spi) == 0)
    {
        return 0;
    }

    /*
     * 第一次交换：
     * 主机发送写命令。
     */
    if(spi_set_slave_response(
           spi,
           SPI_DUMMY_BYTE) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    if(spi_transfer_byte(
           spi,
           command,
           &ignored_rx) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    /*
     * 第二次交换：
     * 主机发送需要写入的寄存器数据。
     */
    if(spi_set_slave_response(
           spi,
           SPI_DUMMY_BYTE) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    if(spi_transfer_byte(
           spi,
           register_value,
           &ignored_rx) == 0)
    {
        spi_device_abort_transaction(spi);
        return 0;
    }

    /*
     * 完整写事务结束。
     */
    if(spi_deselect(spi) == 0)
    {
        return 0;
    }

    /*
     * 只有整个 SPI 事务成功后，
     * 才真正更新模拟设备寄存器。
     */
    device->registers[
        register_address] =
        register_value;

    device->write_count++;

    return 1;
}

int spi_device_read_temperature(
    SpiBus *spi,
    SpiDevice *device,
    uint16_t *temperature_raw,
    float *temperature_c)
{
    uint8_t high_byte;
    uint8_t low_byte;

    if((spi == NULL) ||
       (device == NULL) ||
       (temperature_raw == NULL) ||
       (temperature_c == NULL))
    {
        return 0;
    }

    /*
     * 读取温度高字节寄存器 0x02。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_TEMP_HIGH,
           &high_byte) == 0)
    {
        return 0;
    }

    /*
     * 读取温度低字节寄存器 0x03。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_TEMP_LOW,
           &low_byte) == 0)
    {
        return 0;
    }

    /*
     * 组合高低字节。
     *
     * high = 0x09
     * low  = 0xE6
     *
     * 0x09 << 8 = 0x0900
     * 0x0900 | 0x00E6 = 0x09E6
     */
    *temperature_raw =
        (uint16_t)(
            ((uint16_t)high_byte << 8) |
            (uint16_t)low_byte);

    /*
     * 原始值每 1 个单位表示 0.01 摄氏度。
     *
     * 2534 / 100.0 = 25.34
     */
    *temperature_c =
        (float)(*temperature_raw) /
        SPI_DEVICE_TEMP_SCALE;

    return 1;
}