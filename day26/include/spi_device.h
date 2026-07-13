#ifndef SPI_DEVICE_SIM_H
#define SPI_DEVICE_SIM_H

#include <stdint.h>

#include "spi.h"

/*
 * SPI 设备命令格式：
 *
 * bit7：
 *     1 表示读操作
 *     0 表示写操作
 *
 * bit6～bit0：
 *     表示寄存器地址
 */
#define SPI_DEVICE_READ_FLAG        0x80U
#define SPI_DEVICE_ADDRESS_MASK     0x7FU

/*
 * 模拟温度传感器的寄存器地址。
 */
#define SPI_DEVICE_REG_ID           0x00U
#define SPI_DEVICE_REG_STATUS       0x01U
#define SPI_DEVICE_REG_TEMP_HIGH    0x02U
#define SPI_DEVICE_REG_TEMP_LOW     0x03U
#define SPI_DEVICE_REG_CONFIG       0x04U

/*
 * 当前设备共有 5 个寄存器，
 * 有效地址范围为 0x00～0x04。
 */
#define SPI_DEVICE_REGISTER_COUNT   5U

/*
 * 模拟设备的固定设备 ID。
 */
#define SPI_DEVICE_EXPECTED_ID      0x42U

/*
 * 状态寄存器中的数据就绪标志。
 *
 * bit0 = 1：
 * 温度数据已经准备完成。
 */
#define SPI_DEVICE_STATUS_READY     0x01U

/*
 * 初始配置寄存器值。
 */
#define SPI_DEVICE_DEFAULT_CONFIG   0x00U

/*
 * 初始温度原始值。
 *
 * 2534 表示 25.34 摄氏度。
 */
#define SPI_DEVICE_DEFAULT_TEMP_RAW 2534U

/*
 * 温度换算比例。
 *
 * 原始值每增加 1，
 * 表示温度增加 0.01 摄氏度。
 */
#define SPI_DEVICE_TEMP_SCALE       100.0f

/*
 * 模拟 SPI 温度传感器。
 */
typedef struct
{
    /*
     * 设备内部寄存器表。
     *
     * registers[0]：设备 ID
     * registers[1]：状态寄存器
     * registers[2]：温度高字节
     * registers[3]：温度低字节
     * registers[4]：配置寄存器
     */
    uint8_t registers[SPI_DEVICE_REGISTER_COUNT];

    /*
     * 成功完成的寄存器读取次数。
     */
    uint32_t read_count;

    /*
     * 成功完成的寄存器写入次数。
     */
    uint32_t write_count;
} SpiDevice;

/*
 * 判断寄存器地址是否合法。
 *
 * 合法地址为 0x00～0x04。
 *
 * 合法返回 1，非法返回 0。
 */
int spi_device_register_is_valid(
    uint8_t register_address);

/*
 * 判断寄存器是否允许写入。
 *
 * 当前只有配置寄存器 0x04 可写。
 *
 * 可写返回 1，不可写返回 0。
 */
int spi_device_register_is_writable(
    uint8_t register_address);

/*
 * 初始化模拟 SPI 温度传感器。
 *
 * 初始化内容：
 *
 * 设备 ID = 0x42
 * 状态 = 数据就绪
 * 温度 = 25.34 摄氏度
 * 配置 = 0x00
 * 读写计数清零
 *
 * 成功返回 1，失败返回 0。
 */
int spi_device_init(SpiDevice *device);

/*
 * 修改设备内部的温度原始值。
 *
 * 例如：
 *
 * temperature_raw = 2534
 * 表示 25.34 摄氏度。
 *
 * 该函数用于模拟传感器产生新的测量结果。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_device_set_temperature_raw(
    SpiDevice *device,
    uint16_t temperature_raw);

/*
 * 通过 SPI 读取一个设备寄存器。
 *
 * 完整事务为：
 *
 * CS 拉低
 *     ↓
 * 发送读命令
 *     ↓
 * 发送 Dummy Byte
 *     ↓
 * 接收寄存器值
 *     ↓
 * CS 拉高
 *
 * 成功返回 1，失败返回 0。
 */
int spi_device_read_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t *register_value);

/*
 * 通过 SPI 写入一个设备寄存器。
 *
 * 当前只有配置寄存器 0x04 允许写入。
 *
 * 完整事务为：
 *
 * CS 拉低
 *     ↓
 * 发送写命令
 *     ↓
 * 发送写入数据
 *     ↓
 * CS 拉高
 *
 * 成功返回 1，失败返回 0。
 */
int spi_device_write_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t register_value);

/*
 * 读取并解析温度。
 *
 * 函数会分别读取：
 *
 * 0x02：温度高字节
 * 0x03：温度低字节
 *
 * 然后组合成 16 位温度原始值，
 * 并换算成摄氏度。
 *
 * temperature_raw：
 * 保存温度原始值，例如 2534。
 *
 * temperature_c：
 * 保存实际温度，例如 25.34。
 *
 * 成功返回 1，失败返回 0。
 */
int spi_device_read_temperature(
    SpiBus *spi,
    SpiDevice *device,
    uint16_t *temperature_raw,
    float *temperature_c);

#endif