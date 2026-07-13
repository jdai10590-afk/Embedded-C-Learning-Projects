#include <stddef.h>

#include "spi.h"

int spi_mode_is_valid(SpiMode mode)
{
    /*
     * SpiMode 的合法值为 0～3。
     *
     * 转换成 uint32_t 后再比较，
     * 可以同时排除负数和大于 3 的非法值。
     */
    if((uint32_t)mode < SPI_MODE_COUNT)
    {
        return 1;
    }

    return 0;
}

int spi_init(SpiBus *spi,
             uint32_t clock_hz,
             SpiMode mode)
{
    /*
     * SPI 总线结构体地址不能为空。
     */
    if(spi == NULL)
    {
        return 0;
    }

    /*
     * SPI 必须具有有效时钟。
     *
     * 时钟为 0 时无法完成同步传输。
     */
    if(clock_hz == 0U)
    {
        return 0;
    }

    /*
     * 只允许使用 Mode 0～Mode 3。
     */
    if(spi_mode_is_valid(mode) == 0)
    {
        return 0;
    }

    spi->clock_hz = clock_hz;
    spi->mode = mode;

    /*
     * 初始化完成后不默认选择任何从机。
     *
     * chip_selected = 0：
     * 从机未选中，模拟 CS 为高电平。
     */
    spi->chip_selected = 0U;

    /*
     * 默认从机返回占位字节 0xFF。
     */
    spi->slave_response = SPI_DUMMY_BYTE;

    /*
     * 初始化后尚未进行任何成功传输。
     */
    spi->transfer_count = 0U;

    return 1;
}

int spi_select(SpiBus *spi)
{
    if(spi == NULL)
    {
        return 0;
    }

    /*
     * 已经选择从机时，不允许重复选择。
     *
     * 这可以帮助检查不规范的调用顺序。
     */
    if(spi->chip_selected != 0U)
    {
        return 0;
    }

    /*
     * chip_selected = 1 表示设备已被选择。
     *
     * 对于低电平有效的 CS，
     * 这相当于模拟将 CS 拉低。
     */
    spi->chip_selected = 1U;

    return 1;
}

int spi_deselect(SpiBus *spi)
{
    if(spi == NULL)
    {
        return 0;
    }

    /*
     * 当前没有选择从机时，
     * 不允许再次执行释放操作。
     */
    if(spi->chip_selected == 0U)
    {
        return 0;
    }

    /*
     * chip_selected = 0 表示设备被释放。
     *
     * 对于低电平有效的 CS，
     * 这相当于模拟将 CS 拉高。
     */
    spi->chip_selected = 0U;

    return 1;
}

int spi_set_slave_response(
    SpiBus *spi,
    uint8_t response)
{
    if(spi == NULL)
    {
        return 0;
    }

    /*
     * 保存从机在下一次传输中准备返回的字节。
     */
    spi->slave_response = response;

    return 1;
}

int spi_transfer_byte(
    SpiBus *spi,
    uint8_t tx_data,
    uint8_t *rx_data)
{
    if((spi == NULL) ||
       (rx_data == NULL))
    {
        return 0;
    }

    /*
     * 从机没有被选择时，不允许传输。
     */
    if(spi->chip_selected == 0U)
    {
        return 0;
    }

    /*
     * 防止结构体内部状态被意外破坏。
     */
    if((spi->clock_hz == 0U) ||
       (spi_mode_is_valid(spi->mode) == 0))
    {
        return 0;
    }

    /*
     * 当前软件模型不逐位模拟 MOSI。
     *
     * tx_data 表示主机本次通过 MOSI
     * 发出的一个完整字节。
     */
    (void)tx_data;

    /*
     * 在发送 tx_data 的同一组时钟周期中，
     * 主机通过 MISO 接收到 slave_response。
     */
    *rx_data = spi->slave_response;

    /*
     * 从机响应按“下一次响应”处理。
     *
     * 完成一次传输后恢复为默认占位字节，
     * 下一次需要特殊返回值时应重新设置。
     */
    spi->slave_response = SPI_DUMMY_BYTE;

    /*
     * 只有成功完成传输才增加计数。
     */
    spi->transfer_count++;

    return 1;
}