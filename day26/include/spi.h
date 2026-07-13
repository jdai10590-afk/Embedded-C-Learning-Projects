#ifndef SPI_SIM_H
#define SPI_SIM_H

#include <stdint.h>

/*
 * SPI 模式总数。
 *
 * SPI 一共有 Mode 0～Mode 3 四种模式。
 */
#define SPI_MODE_COUNT 4U

/*
 * SPI 常用占位字节。
 *
 * 当主机只想读取从机数据时，
 * 仍然需要发送一个字节以产生时钟。
 */
#define SPI_DUMMY_BYTE 0xFFU

/*
 * SPI 模式定义。
 *
 * Mode 0：CPOL=0，CPHA=0
 * Mode 1：CPOL=0，CPHA=1
 * Mode 2：CPOL=1，CPHA=0
 * Mode 3：CPOL=1，CPHA=1
 */
typedef enum
{
    SPI_MODE_0 = 0,
    SPI_MODE_1,
    SPI_MODE_2,
    SPI_MODE_3
} SpiMode;

/*
 * SPI 总线状态结构体。
 */
typedef struct
{
    /*
     * SPI 时钟频率，单位 Hz。
     */
    uint32_t clock_hz;

    /*
     * SPI 工作模式。
     */
    SpiMode mode;

    /*
     * 片选状态。
     *
     * 0：从机未被选择，CS 为高电平
     * 1：从机已被选择，CS 为低电平
     */
    uint8_t chip_selected;

    /*
     * 模拟从机下一次准备返回的字节。
     */
    uint8_t slave_response;

    /*
     * 成功完成的传输次数。
     */
    uint32_t transfer_count;
} SpiBus;

/*
 * 判断 SPI 模式是否合法。
 *
 * 合法返回 1，非法返回 0。
 */
int spi_mode_is_valid(SpiMode mode);

/*
 * 初始化 SPI 总线。
 *
 * spi：
 * SPI 总线结构体地址。
 *
 * clock_hz：
 * SPI 时钟频率，单位 Hz。
 *
 * mode：
 * SPI 工作模式。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_init(SpiBus *spi,
             uint32_t clock_hz,
             SpiMode mode);

/*
 * 选择 SPI 从机。
 *
 * 模拟将 CS 拉低。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_select(SpiBus *spi);

/*
 * 释放 SPI 从机。
 *
 * 模拟将 CS 拉高。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_deselect(SpiBus *spi);

/*
 * 设置从机下一次准备返回的数据。
 *
 * 这个函数只用于当前软件模拟。
 */
int spi_set_slave_response(
    SpiBus *spi,
    uint8_t response);

/*
 * 进行一次单字节 SPI 全双工传输。
 *
 * tx_data：
 * 主机本次发送的数据。
 *
 * rx_data：
 * 用于保存从机同时返回的数据。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_transfer_byte(
    SpiBus *spi,
    uint8_t tx_data,
    uint8_t *rx_data);

#endif
