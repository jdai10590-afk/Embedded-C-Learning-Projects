#include <stdio.h>

#include "spi_device.h"

/*
 * 测试 SPI 读写命令的编码方式。
 */
static void test_command_encoding(void)
{
    uint8_t read_id_command;
    uint8_t read_temp_high_command;
    uint8_t write_config_command;

    /*
     * 读命令：
     * bit7 = 1。
     */
    read_id_command =
        SPI_DEVICE_REG_ID |
        SPI_DEVICE_READ_FLAG;

    read_temp_high_command =
        SPI_DEVICE_REG_TEMP_HIGH |
        SPI_DEVICE_READ_FLAG;

    /*
     * 写命令：
     * bit7 = 0。
     */
    write_config_command =
        SPI_DEVICE_REG_CONFIG &
        SPI_DEVICE_ADDRESS_MASK;

    printf("\n");
    printf(
        "----- Test 1: Command Encoding -----\n");

    printf(
        "[COMMAND] Read ID register        : 0x%02X\n",
        (unsigned int)read_id_command);

    printf(
        "[COMMAND] Read temperature high   : 0x%02X\n",
        (unsigned int)read_temp_high_command);

    printf(
        "[COMMAND] Write config register   : 0x%02X\n",
        (unsigned int)write_config_command);

    /*
     * 验证最高位的读写标志。
     */
    if((read_id_command &
        SPI_DEVICE_READ_FLAG) != 0U)
    {
        printf(
            "[CHECK] Read command bit7 = 1\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Read flag missing\n");
    }

    if((write_config_command &
        SPI_DEVICE_READ_FLAG) == 0U)
    {
        printf(
            "[CHECK] Write command bit7 = 0\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Write command incorrect\n");
    }
}

/*
 * 测试设备 ID、状态和配置寄存器。
 */
static void test_device_information(
    SpiBus *spi,
    SpiDevice *device)
{
    uint8_t device_id;
    uint8_t status;
    uint8_t config;

    device_id = 0U;
    status = 0U;
    config = 0U;

    printf("\n");
    printf(
        "----- Test 2: Device Information -----\n");

    /*
     * 读取设备 ID 寄存器 0x00。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_ID,
           &device_id) == 0)
    {
        printf(
            "[ERROR] Failed to read device ID\n");
        return;
    }

    printf(
        "[DEVICE] Device ID = 0x%02X\n",
        (unsigned int)device_id);

    if(device_id == SPI_DEVICE_EXPECTED_ID)
    {
        printf(
            "[CHECK] Device ID matched\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Wrong device ID\n");
    }

    /*
     * 读取状态寄存器 0x01。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_STATUS,
           &status) == 0)
    {
        printf(
            "[ERROR] Failed to read status\n");
        return;
    }

    printf(
        "[DEVICE] Status    = 0x%02X\n",
        (unsigned int)status);

    /*
     * 使用按位与检查数据就绪标志。
     */
    if((status &
        SPI_DEVICE_STATUS_READY) != 0U)
    {
        printf(
            "[CHECK] Temperature data is ready\n");
    }
    else
    {
        printf(
            "[CHECK] Temperature data is not ready\n");
    }

    /*
     * 读取配置寄存器 0x04。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_CONFIG,
           &config) == 0)
    {
        printf(
            "[ERROR] Failed to read configuration\n");
        return;
    }

    printf(
        "[DEVICE] Config    = 0x%02X\n",
        (unsigned int)config);

    printf(
        "[COUNT] Register reads = %lu\n",
        (unsigned long)device->read_count);

    printf(
        "[COUNT] SPI byte transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试温度高低字节读取、组合与换算。
 */
static void test_temperature_read(
    SpiBus *spi,
    SpiDevice *device)
{
    uint16_t temperature_raw;
    float temperature_c;
    uint8_t high_byte;
    uint8_t low_byte;

    temperature_raw = 0U;
    temperature_c = 0.0f;
    high_byte = 0U;
    low_byte = 0U;

    printf("\n");
    printf(
        "----- Test 3: Temperature Read -----\n");

    if(spi_device_read_temperature(
           spi,
           device,
           &temperature_raw,
           &temperature_c) == 0)
    {
        printf(
            "[ERROR] Failed to read temperature\n");
        return;
    }

    /*
     * 从组合完成的 16 位原始值中，
     * 再次提取高低字节用于显示。
     */
    high_byte =
        (uint8_t)(
            (temperature_raw >> 8) &
            0xFFU);

    low_byte =
        (uint8_t)(
            temperature_raw &
            0xFFU);

    printf(
        "[TEMPERATURE] High byte  = 0x%02X\n",
        (unsigned int)high_byte);

    printf(
        "[TEMPERATURE] Low byte   = 0x%02X\n",
        (unsigned int)low_byte);

    printf(
        "[TEMPERATURE] Raw value  = %u\n",
        (unsigned int)temperature_raw);

    printf(
        "[TEMPERATURE] Celsius    = %.2f C\n",
        (double)temperature_c);

    if(temperature_raw ==
       SPI_DEVICE_DEFAULT_TEMP_RAW)
    {
        printf(
            "[CHECK] Temperature raw value matched\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Temperature raw value wrong\n");
    }

    printf(
        "[COUNT] Register reads = %lu\n",
        (unsigned long)device->read_count);

    printf(
        "[COUNT] SPI byte transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试配置寄存器写入。
 */
static void test_config_write(
    SpiBus *spi,
    SpiDevice *device)
{
    uint8_t config_before;
    uint8_t config_after;
    uint8_t new_config;

    config_before = 0U;
    config_after = 0U;
    new_config = 0xA5U;

    printf("\n");
    printf(
        "----- Test 4: Configuration Write -----\n");

    /*
     * 写入前读取配置寄存器。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_CONFIG,
           &config_before) == 0)
    {
        printf(
            "[ERROR] Failed to read config before write\n");
        return;
    }

    printf(
        "[WRITE] Config before = 0x%02X\n",
        (unsigned int)config_before);

    printf(
        "[WRITE] New value     = 0x%02X\n",
        (unsigned int)new_config);

    /*
     * 向唯一可写的 CONFIG 寄存器写入 0xA5。
     */
    if(spi_device_write_register(
           spi,
           device,
           SPI_DEVICE_REG_CONFIG,
           new_config) == 0)
    {
        printf(
            "[ERROR] Failed to write configuration\n");
        return;
    }

    /*
     * 写入后再次读取，验证写入结果。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_CONFIG,
           &config_after) == 0)
    {
        printf(
            "[ERROR] Failed to read config after write\n");
        return;
    }

    printf(
        "[WRITE] Config after  = 0x%02X\n",
        (unsigned int)config_after);

    if(config_after == new_config)
    {
        printf(
            "[CHECK] Configuration write verified\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Configuration mismatch\n");
    }

    printf(
        "[COUNT] Register writes = %lu\n",
        (unsigned long)device->write_count);

    printf(
        "[COUNT] SPI byte transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试非法寄存器和只读寄存器保护。
 */
static void test_invalid_operations(
    SpiBus *spi,
    SpiDevice *device)
{
    uint8_t register_value;
    uint32_t transfer_before;
    uint32_t read_before;
    uint32_t write_before;

    register_value = 0U;

    transfer_before = spi->transfer_count;
    read_before = device->read_count;
    write_before = device->write_count;

    printf("\n");
    printf(
        "----- Test 5: Invalid Operations -----\n");

    /*
     * 地址 0x20 超出当前设备寄存器范围。
     */
    if(spi_device_read_register(
           spi,
           device,
           0x20U,
           &register_value) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Invalid register 0x20 rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Invalid register accepted\n");
    }

    /*
     * DEVICE_ID 是只读寄存器，
     * 不允许写入 0x99。
     */
    if(spi_device_write_register(
           spi,
           device,
           SPI_DEVICE_REG_ID,
           0x99U) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Read-only device ID write rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Read-only device ID changed\n");
    }

    /*
     * 直接检查模拟设备内部寄存器，
     * 确认设备 ID 没有被修改。
     */
    printf(
        "[CHECK] Device ID remains 0x%02X\n",
        (unsigned int)
        device->registers[SPI_DEVICE_REG_ID]);

    /*
     * 所有失败操作都不应改变成功计数。
     */
    if((spi->transfer_count == transfer_before) &&
       (device->read_count == read_before) &&
       (device->write_count == write_before))
    {
        printf(
            "[CHECK] All counters unchanged\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Counter changed after failure\n");
    }
}

/*
 * 测试总线已经处于片选状态时，
 * 设备驱动是否拒绝开始新事务。
 */
static void test_busy_bus(
    SpiBus *spi,
    SpiDevice *device)
{
    uint8_t register_value;
    uint32_t transfer_before;
    uint32_t read_before;

    register_value = 0U;
    transfer_before = spi->transfer_count;
    read_before = device->read_count;

    printf("\n");
    printf(
        "----- Test 6: Busy SPI Bus -----\n");

    /*
     * 手动选择从机，
     * 模拟总线正在执行其他事务。
     */
    if(spi_select(spi) == 0)
    {
        printf(
            "[ERROR] Failed to create busy bus state\n");
        return;
    }

    printf(
        "[SPI] Bus manually selected\n");

    /*
     * 设备驱动要求总线空闲，
     * 所以当前读取必须失败。
     */
    if(spi_device_read_register(
           spi,
           device,
           SPI_DEVICE_REG_CONFIG,
           &register_value) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "New transaction rejected while bus busy\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Transaction started on busy bus\n");
    }

    /*
     * 恢复总线空闲状态。
     */
    if(spi_deselect(spi))
    {
        printf(
            "[SPI] Busy bus state released\n");
    }
    else
    {
        printf(
            "[ERROR] Failed to release busy bus\n");
    }

    if((spi->transfer_count == transfer_before) &&
       (device->read_count == read_before))
    {
        printf(
            "[CHECK] Busy rejection changed no counters\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Busy rejection changed counters\n");
    }
}

int main(void)
{
    SpiBus spi;
    SpiDevice device;

    printf("\n");
    printf(
        "===== Day24 SPI Register Protocol Test =====\n");

    /*
     * 初始化 SPI 总线：
     * 1 MHz、Mode 0。
     */
    if(spi_init(
           &spi,
           1000000U,
           SPI_MODE_0) == 0)
    {
        printf(
            "[FATAL] SPI initialization failed\n");

        return 1;
    }

    /*
     * 初始化模拟 SPI 温度传感器。
     */
    if(spi_device_init(&device) == 0)
    {
        printf(
            "[FATAL] SPI device initialization failed\n");

        return 1;
    }

    printf(
        "[SPI CONFIG] Clock = %lu Hz\n",
        (unsigned long)spi.clock_hz);

    printf(
        "[SPI CONFIG] Mode = %u\n",
        (unsigned int)spi.mode);

    printf(
        "[DEVICE CONFIG] Expected ID = 0x%02X\n",
        (unsigned int)SPI_DEVICE_EXPECTED_ID);

    printf(
        "[DEVICE CONFIG] Register count = %u\n",
        (unsigned int)SPI_DEVICE_REGISTER_COUNT);

    test_command_encoding();

    test_device_information(
        &spi,
        &device);

    test_temperature_read(
        &spi,
        &device);

    test_config_write(
        &spi,
        &device);

    test_invalid_operations(
        &spi,
        &device);

    test_busy_bus(
        &spi,
        &device);

    printf("\n");
    printf(
        "----- Final Protocol Status -----\n");

    printf(
        "[FINAL] SPI byte transfers = %lu\n",
        (unsigned long)spi.transfer_count);

    printf(
        "[FINAL] Register reads     = %lu\n",
        (unsigned long)device.read_count);

    printf(
        "[FINAL] Register writes    = %lu\n",
        (unsigned long)device.write_count);

    printf(
        "[FINAL] Device selected    = %s\n",
        spi.chip_selected ?
        "YES" :
        "NO");

    printf(
        "[FINAL] Config register    = 0x%02X\n",
        (unsigned int)
        device.registers[
            SPI_DEVICE_REG_CONFIG]);

    /*
     * 根据当前测试流程，预计：
     *
     * 7 次寄存器读取：
     * ID、状态、配置、温度高、温度低、
     * 写入前配置、写入后配置。
     *
     * 1 次寄存器写入：
     * CONFIG = 0xA5。
     *
     * 每次寄存器读写包含 2 次字节交换，
     * 因此总交换次数：
     *
     * 7 × 2 + 1 × 2 = 16。
     */
    if((spi.transfer_count == 16U) &&
       (device.read_count == 7U) &&
       (device.write_count == 1U) &&
       (spi.chip_selected == 0U) &&
       (device.registers[
            SPI_DEVICE_REG_CONFIG] == 0xA5U))
    {
        printf(
            "[FINAL CHECK] All protocol tests passed\n");
    }
    else
    {
        printf(
            "[FINAL CHECK ERROR] "
            "Protocol result mismatch\n");
    }

    printf("\n");
    printf(
        "===== SPI Register Test Finished =====\n");

    return 0;
}