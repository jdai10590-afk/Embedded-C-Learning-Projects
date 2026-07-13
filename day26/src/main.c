#include <stdio.h>

#include "i2c_device.h"

/*
 * 测试普通 START、Repeated START 和 STOP
 * 对总线状态的影响。
 *
 * 使用独立测试总线，不影响主测试总线计数。
 */
static void test_repeated_start_state(void)
{
    I2cBus test_bus;

    printf("\n");
    printf(
        "----- Test 1: Repeated START State -----\n");

    if(i2c_init(
           &test_bus,
           I2C_STANDARD_CLOCK_HZ) == 0)
    {
        printf(
            "[ERROR] Temporary bus initialization failed\n");

        return;
    }

    /*
     * 总线空闲时，不能直接产生 Repeated START。
     */
    if(i2c_repeated_start(&test_bus) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Repeated START rejected on idle bus\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Repeated START accepted on idle bus\n");
    }

    /*
     * 普通 START。
     */
    if(i2c_start(&test_bus) == 0)
    {
        printf(
            "[ERROR] Normal START failed\n");

        return;
    }

    printf(
        "[I2C] Normal START generated\n");

    /*
     * 当前事务已经开始，
     * 此时允许产生 Repeated START。
     */
    if(i2c_repeated_start(
           &test_bus) == 0)
    {
        printf(
            "[ERROR] Repeated START failed\n");

        (void)i2c_stop(&test_bus);

        return;
    }

    printf(
        "[I2C] Repeated START generated\n");

    printf(
        "[STATE] Bus started = %s\n",
        test_bus.started ?
        "YES" :
        "NO");

    printf(
        "[STATE] Addressed   = %s\n",
        test_bus.addressed ?
        "YES" :
        "NO");

    /*
     * Repeated START 后：
     *
     * started 仍为 1
     * addressed 被清零
     */
    if((test_bus.started == 1U) &&
       (test_bus.addressed == 0U) &&
       (test_bus.start_count == 1U) &&
       (test_bus.repeated_start_count == 1U))
    {
        printf(
            "[CHECK] Repeated START state is correct\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Repeated START state mismatch\n");
    }

    if(i2c_stop(&test_bus) == 0)
    {
        printf(
            "[ERROR] Temporary STOP failed\n");

        return;
    }

    printf(
        "[I2C] STOP generated\n");

    if((test_bus.started == 0U) &&
       (test_bus.stop_count == 1U))
    {
        printf(
            "[CHECK] Temporary bus returned to idle\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Temporary bus did not return to idle\n");
    }
}

/*
 * 测试设备 ID 和状态寄存器。
 */
static void test_device_information(
    I2cBus *bus,
    I2cRegisterDevice *device)
{
    uint8_t device_id;
    uint8_t status;

    device_id = 0U;
    status = 0U;

    printf("\n");
    printf(
        "----- Test 2: Device Information -----\n");

    /*
     * 读取 DEVICE_ID 寄存器 0x00。
     */
    if(i2c_register_device_read_register(
           bus,
           device,
           I2C_DEVICE_REG_ID,
           &device_id) == 0)
    {
        printf(
            "[ERROR] Failed to read device ID\n");

        return;
    }

    printf(
        "[DEVICE] ID register     = 0x%02X\n",
        (unsigned int)device_id);

    if(device_id ==
       I2C_DEVICE_EXPECTED_ID)
    {
        printf(
            "[CHECK] Device ID matched\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Device ID mismatch\n");
    }

    /*
     * 读取 STATUS 寄存器 0x01。
     */
    if(i2c_register_device_read_register(
           bus,
           device,
           I2C_DEVICE_REG_STATUS,
           &status) == 0)
    {
        printf(
            "[ERROR] Failed to read status register\n");

        return;
    }

    printf(
        "[DEVICE] Status register = 0x%02X\n",
        (unsigned int)status);

    /*
     * 使用按位与检查 READY 状态位。
     */
    if((status &
        I2C_DEVICE_STATUS_READY) != 0U)
    {
        printf(
            "[CHECK] Temperature data is ready\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Temperature data is not ready\n");
    }

    printf(
        "[COUNT] Register reads = %lu\n",
        (unsigned long)
        device->register_read_count);

    printf(
        "[COUNT] Repeated START = %lu\n",
        (unsigned long)
        bus->repeated_start_count);
}

/*
 * 测试从 TEMP_HIGH 开始连续读取两个字节。
 */
static void test_temperature_read(
    I2cBus *bus,
    I2cRegisterDevice *device)
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
        "----- Test 3: Continuous Temperature Read -----\n");

    if(i2c_register_device_read_temperature(
           bus,
           device,
           &temperature_raw,
           &temperature_c) == 0)
    {
        printf(
            "[ERROR] Failed to read temperature\n");

        return;
    }

    /*
     * 从组合完成的 16 位数据中，
     * 提取高低字节用于显示。
     */
    high_byte =
        (uint8_t)(
            (temperature_raw >> 8) &
            0x00FFU);

    low_byte =
        (uint8_t)(
            temperature_raw &
            0x00FFU);

    printf(
        "[TEMPERATURE] High byte = 0x%02X\n",
        (unsigned int)high_byte);

    printf(
        "[TEMPERATURE] Low byte  = 0x%02X\n",
        (unsigned int)low_byte);

    printf(
        "[TEMPERATURE] Raw value = %u\n",
        (unsigned int)temperature_raw);

    printf(
        "[TEMPERATURE] Celsius   = %.2f C\n",
        (double)temperature_c);

    if(temperature_raw ==
       I2C_DEVICE_DEFAULT_TEMP_RAW)
    {
        printf(
            "[CHECK] Temperature value matched\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Temperature value mismatch\n");
    }

    printf(
        "[COUNT] Register reads = %lu\n",
        (unsigned long)
        device->register_read_count);

    printf(
        "[COUNT] ACK count      = %lu\n",
        (unsigned long)
        bus->ack_count);

    printf(
        "[COUNT] NACK count     = %lu\n",
        (unsigned long)
        bus->nack_count);
}

/*
 * 测试配置寄存器的读取、写入和回读验证。
 */
static void test_config_register(
    I2cBus *bus,
    I2cRegisterDevice *device)
{
    uint8_t config_before;
    uint8_t config_after;
    uint8_t new_config;

    config_before = 0U;
    config_after = 0U;
    new_config = 0xA5U;

    printf("\n");
    printf(
        "----- Test 4: Configuration Register -----\n");

    /*
     * 写入前读取配置寄存器。
     */
    if(i2c_register_device_read_register(
           bus,
           device,
           I2C_DEVICE_REG_CONFIG,
           &config_before) == 0)
    {
        printf(
            "[ERROR] Failed to read config before write\n");

        return;
    }

    printf(
        "[CONFIG] Before write = 0x%02X\n",
        (unsigned int)config_before);

    printf(
        "[CONFIG] Write value  = 0x%02X\n",
        (unsigned int)new_config);

    /*
     * 向唯一可写的 CONFIG 寄存器写入数据。
     */
    if(i2c_register_device_write_register(
           bus,
           device,
           I2C_DEVICE_REG_CONFIG,
           new_config) == 0)
    {
        printf(
            "[ERROR] Failed to write config register\n");

        return;
    }

    /*
     * 写入完成后重新读取，
     * 验证寄存器值。
     */
    if(i2c_register_device_read_register(
           bus,
           device,
           I2C_DEVICE_REG_CONFIG,
           &config_after) == 0)
    {
        printf(
            "[ERROR] Failed to read config after write\n");

        return;
    }

    printf(
        "[CONFIG] After write  = 0x%02X\n",
        (unsigned int)config_after);

    if((config_before ==
        I2C_DEVICE_DEFAULT_CONFIG) &&
       (config_after == new_config))
    {
        printf(
            "[CHECK] Configuration write verified\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Configuration verification failed\n");
    }

    printf(
        "[COUNT] Register reads  = %lu\n",
        (unsigned long)
        device->register_read_count);

    printf(
        "[COUNT] Register writes = %lu\n",
        (unsigned long)
        device->register_write_count);
}

/*
 * 测试非法寄存器、越界读取和只读保护。
 *
 * 所有失败操作都应该在 START 前被拒绝，
 * 因此不能改变主总线通信计数。
 */
static void test_invalid_register_operations(
    I2cBus *bus,
    I2cRegisterDevice *device)
{
    uint8_t register_value;
    uint8_t read_buffer[3];

    uint32_t start_before;
    uint32_t repeated_before;
    uint32_t stop_before;
    uint32_t ack_before;
    uint32_t nack_before;
    uint32_t byte_before;
    uint32_t read_before;
    uint32_t write_before;

    register_value = 0U;

    read_buffer[0] = 0U;
    read_buffer[1] = 0U;
    read_buffer[2] = 0U;

    start_before =
        bus->start_count;

    repeated_before =
        bus->repeated_start_count;

    stop_before =
        bus->stop_count;

    ack_before =
        bus->ack_count;

    nack_before =
        bus->nack_count;

    byte_before =
        bus->byte_count;

    read_before =
        device->register_read_count;

    write_before =
        device->register_write_count;

    printf("\n");
    printf(
        "----- Test 5: Invalid Register Operations -----\n");

    /*
     * 0x20 超出合法寄存器范围 0x00～0x04。
     */
    if(i2c_register_device_read_register(
           bus,
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
            "Invalid register 0x20 accepted\n");
    }

    /*
     * 从 0x03 开始读取 3 个字节会访问：
     *
     * 0x03
     * 0x04
     * 0x05
     *
     * 其中 0x05 越界。
     */
    if(i2c_register_device_read_registers(
           bus,
           device,
           I2C_DEVICE_REG_TEMP_LOW,
           read_buffer,
           3U) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Out-of-range continuous read rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Out-of-range continuous read accepted\n");
    }

    /*
     * DEVICE_ID 是只读寄存器，
     * 不允许写入 0x99。
     */
    if(i2c_register_device_write_register(
           bus,
           device,
           I2C_DEVICE_REG_ID,
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
            "Read-only device ID write accepted\n");
    }

    /*
     * 连续读取长度不能为 0。
     */
    if(i2c_register_device_read_registers(
           bus,
           device,
           I2C_DEVICE_REG_ID,
           read_buffer,
           0U) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Zero-length read rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Zero-length read accepted\n");
    }

    /*
     * 失败操作后设备 ID 应保持不变。
     */
    printf(
        "[CHECK] Device ID remains 0x%02X\n",
        (unsigned int)
        device->registers[
            I2C_DEVICE_REG_ID]);

    /*
     * 所有通信和寄存器计数都应该保持不变。
     */
    if((bus->start_count ==
        start_before) &&
       (bus->repeated_start_count ==
        repeated_before) &&
       (bus->stop_count ==
        stop_before) &&
       (bus->ack_count ==
        ack_before) &&
       (bus->nack_count ==
        nack_before) &&
       (bus->byte_count ==
        byte_before) &&
       (device->register_read_count ==
        read_before) &&
       (device->register_write_count ==
        write_before))
    {
        printf(
            "[CHECK] All counters unchanged\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Counters changed after rejected operation\n");
    }
}

/*
 * 测试设备驱动在总线忙时是否拒绝开始新事务。
 *
 * 使用临时总线和临时设备，
 * 不影响主总线最终统计。
 */
static void test_busy_bus_protection(void)
{
    I2cBus test_bus;
    I2cRegisterDevice test_device;
    uint8_t register_value;

    register_value = 0U;

    printf("\n");
    printf(
        "----- Test 6: Busy Bus Protection -----\n");

    if(i2c_init(
           &test_bus,
           I2C_STANDARD_CLOCK_HZ) == 0)
    {
        printf(
            "[ERROR] Temporary bus initialization failed\n");

        return;
    }

    if(i2c_register_device_init(
           &test_device,
           I2C_DEVICE_DEFAULT_ADDRESS) == 0)
    {
        printf(
            "[ERROR] Temporary device initialization failed\n");

        return;
    }

    /*
     * 手动产生 START，
     * 模拟总线正被其他事务占用。
     */
    if(i2c_start(&test_bus) == 0)
    {
        printf(
            "[ERROR] Failed to create busy bus state\n");

        return;
    }

    printf(
        "[I2C] Temporary bus marked busy\n");

    /*
     * 寄存器读取要求总线最初为空闲，
     * 因此当前操作必须失败。
     */
    if(i2c_register_device_read_register(
           &test_bus,
           &test_device,
           I2C_DEVICE_REG_ID,
           &register_value) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Register read rejected while bus busy\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Register read accepted while bus busy\n");
    }

    if(i2c_stop(&test_bus) == 0)
    {
        printf(
            "[ERROR] Failed to release temporary bus\n");

        return;
    }

    printf(
        "[I2C] Temporary bus released\n");

    if((test_bus.started == 0U) &&
       (test_device.register_read_count == 0U))
    {
        printf(
            "[CHECK] Busy bus protection verified\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Busy bus protection failed\n");
    }
}

int main(void)
{
    I2cBus bus;
    I2cRegisterDevice device;

    printf("\n");
    printf(
        "===== Day26 I2C Register Protocol Test =====\n");

    /*
     * 初始化 100 kHz I2C 总线。
     */
    if(i2c_init(
           &bus,
           I2C_STANDARD_CLOCK_HZ) == 0)
    {
        printf(
            "[FATAL] I2C bus initialization failed\n");

        return 1;
    }

    /*
     * 初始化地址为 0x48 的寄存器设备。
     */
    if(i2c_register_device_init(
           &device,
           I2C_DEVICE_DEFAULT_ADDRESS) == 0)
    {
        printf(
            "[FATAL] I2C register device initialization failed\n");

        return 1;
    }

    printf(
        "[I2C CONFIG] Clock = %lu Hz\n",
        (unsigned long)bus.clock_hz);

    printf(
        "[DEVICE CONFIG] 7-bit address = 0x%02X\n",
        (unsigned int)
        device.bus_device.address);

    printf(
        "[DEVICE CONFIG] Register count = %u\n",
        (unsigned int)
        I2C_DEVICE_REGISTER_COUNT);

    printf(
        "[DEVICE CONFIG] Expected ID = 0x%02X\n",
        (unsigned int)
        I2C_DEVICE_EXPECTED_ID);

    test_repeated_start_state();

    test_device_information(
        &bus,
        &device);

    test_temperature_read(
        &bus,
        &device);

    test_config_register(
        &bus,
        &device);

    test_invalid_register_operations(
        &bus,
        &device);

    test_busy_bus_protection();

    printf("\n");
    printf(
        "----- Final I2C Register Status -----\n");

    printf(
        "[FINAL] START count          = %lu\n",
        (unsigned long)
        bus.start_count);

    printf(
        "[FINAL] Repeated START count = %lu\n",
        (unsigned long)
        bus.repeated_start_count);

    printf(
        "[FINAL] STOP count           = %lu\n",
        (unsigned long)
        bus.stop_count);

    printf(
        "[FINAL] ACK count            = %lu\n",
        (unsigned long)
        bus.ack_count);

    printf(
        "[FINAL] NACK count           = %lu\n",
        (unsigned long)
        bus.nack_count);

    printf(
        "[FINAL] Byte count           = %lu\n",
        (unsigned long)
        bus.byte_count);

    printf(
        "[FINAL] Register reads       = %lu\n",
        (unsigned long)
        device.register_read_count);

    printf(
        "[FINAL] Register writes      = %lu\n",
        (unsigned long)
        device.register_write_count);

    printf(
        "[FINAL] Device selected      = %s\n",
        bus.addressed ?
        "YES" :
        "NO");

    printf(
        "[FINAL] Bus started          = %s\n",
        bus.started ?
        "YES" :
        "NO");

    printf(
        "[FINAL] Config register      = 0x%02X\n",
        (unsigned int)
        device.registers[
            I2C_DEVICE_REG_CONFIG]);

    /*
     * 主总线共完成：
     *
     * 5 次读取事务：
     *
     * 1. DEVICE_ID
     * 2. STATUS
     * 3. 温度高低字节连续读取
     * 4. 写入前读取 CONFIG
     * 5. 写入后读取 CONFIG
     *
     * 1 次写入事务：
     *
     * 6. 写入 CONFIG = 0xA5
     *
     * 计数结果：
     *
     * START          = 6
     * Repeated START = 5
     * STOP           = 6
     * ACK            = 19
     * NACK           = 5
     * BYTE           = 24
     *
     * 寄存器读取：
     *
     * DEVICE_ID：1
     * STATUS：1
     * TEMP_HIGH + TEMP_LOW：2
     * CONFIG 写入前：1
     * CONFIG 写入后：1
     *
     * 总计 6 个寄存器。
     */
    if((bus.start_count == 6U) &&
       (bus.repeated_start_count == 5U) &&
       (bus.stop_count == 6U) &&
       (bus.ack_count == 19U) &&
       (bus.nack_count == 5U) &&
       (bus.byte_count == 24U) &&
       (device.register_read_count == 6U) &&
       (device.register_write_count == 1U) &&
       (bus.started == 0U) &&
       (bus.addressed == 0U) &&
       (device.registers[
            I2C_DEVICE_REG_ID] ==
        I2C_DEVICE_EXPECTED_ID) &&
       (device.registers[
            I2C_DEVICE_REG_CONFIG] ==
        0xA5U))
    {
        printf(
            "[FINAL CHECK] "
            "All I2C register tests passed\n");
    }
    else
    {
        printf(
            "[FINAL CHECK ERROR] "
            "I2C register result mismatch\n");
    }

    printf("\n");
    printf(
        "===== I2C Register Test Finished =====\n");

    return 0;
}