#include <stdio.h>

#include "i2c.h"

/*
 * 将 ACK/NACK 枚举转换为便于打印的字符串。
 *
 * 该函数只在当前 main.c 中使用，
 * 因此使用 static 修饰。
 */
static const char *i2c_response_to_string(
    I2cResponse response)
{
    if(response == I2C_RESPONSE_ACK)
    {
        return "ACK";
    }

    return "NACK";
}

/*
 * 测试 7 位设备地址与读写方向位的组合。
 */
static void test_address_byte_build(void)
{
    uint8_t write_address_byte;
    uint8_t read_address_byte;

    write_address_byte = 0U;
    read_address_byte = 0U;

    printf("\n");
    printf(
        "----- Test 1: Address Byte Build -----\n");

    /*
     * 设备地址 0x48，写方向：
     *
     * (0x48 << 1) | 0 = 0x90
     */
    if(i2c_build_address_byte(
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DIRECTION_WRITE,
           &write_address_byte) == 0)
    {
        printf(
            "[ERROR] Failed to build write address\n");
        return;
    }

    /*
     * 设备地址 0x48，读方向：
     *
     * (0x48 << 1) | 1 = 0x91
     */
    if(i2c_build_address_byte(
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DIRECTION_READ,
           &read_address_byte) == 0)
    {
        printf(
            "[ERROR] Failed to build read address\n");
        return;
    }

    printf(
        "[ADDRESS] 7-bit device address = 0x%02X\n",
        (unsigned int)
        I2C_DEVICE_DEFAULT_ADDRESS);

    printf(
        "[ADDRESS] Write address byte   = 0x%02X\n",
        (unsigned int)write_address_byte);

    printf(
        "[ADDRESS] Read address byte    = 0x%02X\n",
        (unsigned int)read_address_byte);

    if((write_address_byte == 0x90U) &&
       (read_address_byte == 0x91U))
    {
        printf(
            "[CHECK] Address bytes are correct\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Address bytes are wrong\n");
    }

    /*
     * 验证地址字节最低位。
     */
    if((write_address_byte &
        I2C_RW_MASK) == 0U)
    {
        printf(
            "[CHECK] Write byte bit0 = 0\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Write direction bit wrong\n");
    }

    if((read_address_byte &
        I2C_RW_MASK) == 1U)
    {
        printf(
            "[CHECK] Read byte bit0 = 1\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Read direction bit wrong\n");
    }
}

/*
 * 测试 I2C 单字节写入。
 *
 * 通信流程：
 *
 * START
 * 地址 0x48 + WRITE
 * 从机 ACK
 * 写入数据 0xA5
 * 从机 ACK
 * STOP
 */
static void test_single_byte_write(
    I2cBus *bus,
    I2cDevice *device)
{
    I2cResponse address_response;
    I2cResponse data_response;
    uint8_t address_byte;
    uint8_t write_data;

    address_response = I2C_RESPONSE_NACK;
    data_response = I2C_RESPONSE_NACK;
    address_byte = 0U;
    write_data = 0xA5U;

    printf("\n");
    printf(
        "----- Test 2: Single Byte Write -----\n");

    if(i2c_build_address_byte(
           device->address,
           I2C_DIRECTION_WRITE,
           &address_byte) == 0)
    {
        printf(
            "[ERROR] Failed to build write address\n");
        return;
    }

    /*
     * 产生 START。
     */
    if(i2c_start(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate START\n");
        return;
    }

    printf("[I2C] START\n");

    /*
     * 发送 7 位地址和写方向。
     */
    if(i2c_send_address(
           bus,
           device,
           device->address,
           I2C_DIRECTION_WRITE,
           &address_response) == 0)
    {
        printf(
            "[ERROR] Failed to send device address\n");

        (void)i2c_stop(bus);
        return;
    }

    printf(
        "[I2C] Address byte 0x%02X -> %s\n",
        (unsigned int)address_byte,
        i2c_response_to_string(
            address_response));

    /*
     * 地址没有 ACK 时，不应继续发送数据。
     */
    if(address_response != I2C_RESPONSE_ACK)
    {
        printf(
            "[ERROR] Device did not acknowledge address\n");

        (void)i2c_stop(bus);
        return;
    }

    /*
     * 主机写入一个数据字节。
     */
    if(i2c_write_byte(
           bus,
           device,
           write_data,
           &data_response) == 0)
    {
        printf(
            "[ERROR] Failed to write data byte\n");

        (void)i2c_stop(bus);
        return;
    }

    printf(
        "[I2C] Write data 0x%02X -> %s\n",
        (unsigned int)write_data,
        i2c_response_to_string(
            data_response));

    /*
     * 产生 STOP，结束事务。
     */
    if(i2c_stop(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate STOP\n");
        return;
    }

    printf("[I2C] STOP\n");

    printf(
        "[DEVICE] Stored data = 0x%02X\n",
        (unsigned int)device->stored_data);

    if(device->stored_data == write_data)
    {
        printf(
            "[CHECK] Single byte write verified\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Stored data mismatch\n");
    }
}

/*
 * 测试 I2C 单字节读取。
 *
 * 通信流程：
 *
 * START
 * 地址 0x48 + READ
 * 从机 ACK
 * 从机发送 0x5A
 * 主机发送 NACK
 * STOP
 */
static void test_single_byte_read(
    I2cBus *bus,
    const I2cDevice *device)
{
    I2cResponse address_response;
    uint8_t address_byte;
    uint8_t read_data;

    address_response = I2C_RESPONSE_NACK;
    address_byte = 0U;
    read_data = 0U;

    printf("\n");
    printf(
        "----- Test 3: Single Byte Read -----\n");

    if(i2c_build_address_byte(
           device->address,
           I2C_DIRECTION_READ,
           &address_byte) == 0)
    {
        printf(
            "[ERROR] Failed to build read address\n");
        return;
    }

    if(i2c_start(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate START\n");
        return;
    }

    printf("[I2C] START\n");

    if(i2c_send_address(
           bus,
           device,
           device->address,
           I2C_DIRECTION_READ,
           &address_response) == 0)
    {
        printf(
            "[ERROR] Failed to send read address\n");

        (void)i2c_stop(bus);
        return;
    }

    printf(
        "[I2C] Address byte 0x%02X -> %s\n",
        (unsigned int)address_byte,
        i2c_response_to_string(
            address_response));

    if(address_response != I2C_RESPONSE_ACK)
    {
        printf(
            "[ERROR] Device did not acknowledge address\n");

        (void)i2c_stop(bus);
        return;
    }

    /*
     * 单字节读取结束后，
     * 主机发送 NACK，表示不再读取后续数据。
     */
    if(i2c_read_byte(
           bus,
           device,
           &read_data,
           I2C_RESPONSE_NACK) == 0)
    {
        printf(
            "[ERROR] Failed to read data byte\n");

        (void)i2c_stop(bus);
        return;
    }

    printf(
        "[I2C] Read data 0x%02X\n",
        (unsigned int)read_data);

    printf(
        "[I2C] Master response -> NACK\n");

    if(i2c_stop(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate STOP\n");
        return;
    }

    printf("[I2C] STOP\n");

    if(read_data ==
       I2C_DEVICE_DEFAULT_READ_DATA)
    {
        printf(
            "[CHECK] Single byte read verified\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] Read data mismatch\n");
    }
}

/*
 * 测试错误的设备地址。
 *
 * 模拟设备地址为 0x48，
 * 主机故意访问 0x50。
 */
static void test_wrong_address(
    I2cBus *bus,
    I2cDevice *device)
{
    const uint8_t wrong_address = 0x50U;

    I2cResponse address_response;
    I2cResponse data_response;
    uint8_t address_byte;
    uint8_t stored_before;

    address_response = I2C_RESPONSE_ACK;
    data_response = I2C_RESPONSE_ACK;
    address_byte = 0U;
    stored_before = device->stored_data;

    printf("\n");
    printf(
        "----- Test 4: Wrong Device Address -----\n");

    if(i2c_build_address_byte(
           wrong_address,
           I2C_DIRECTION_WRITE,
           &address_byte) == 0)
    {
        printf(
            "[ERROR] Failed to build wrong address byte\n");
        return;
    }

    if(i2c_start(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate START\n");
        return;
    }

    printf("[I2C] START\n");

    /*
     * 地址字节会正常发送，
     * 但设备地址不匹配，所以返回 NACK。
     */
    if(i2c_send_address(
           bus,
           device,
           wrong_address,
           I2C_DIRECTION_WRITE,
           &address_response) == 0)
    {
        printf(
            "[ERROR] Address operation failed\n");

        (void)i2c_stop(bus);
        return;
    }

    printf(
        "[I2C] Address 0x%02X "
        "(byte 0x%02X) -> %s\n",
        (unsigned int)wrong_address,
        (unsigned int)address_byte,
        i2c_response_to_string(
            address_response));

    if(address_response == I2C_RESPONSE_NACK)
    {
        printf(
            "[EXPECTED] Wrong address was not acknowledged\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] Wrong address received ACK\n");
    }

    /*
     * 地址没有匹配时，
     * 继续写入数据应该被驱动拒绝。
     */
    if(i2c_write_byte(
           bus,
           device,
           0x33U,
           &data_response) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Data write rejected after address NACK\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Data written after address NACK\n");
    }

    if(i2c_stop(bus) == 0)
    {
        printf(
            "[ERROR] Failed to generate STOP\n");
        return;
    }

    printf("[I2C] STOP\n");

    if(device->stored_data == stored_before)
    {
        printf(
            "[CHECK] Stored data unchanged: 0x%02X\n",
            (unsigned int)device->stored_data);
    }
    else
    {
        printf(
            "[CHECK ERROR] Stored data was changed\n");
    }
}

/*
 * 测试错误的 I2C 调用顺序。
 *
 * 使用独立的临时总线，
 * 避免影响主测试总线的最终统计。
 */
static void test_invalid_sequences(void)
{
    I2cBus test_bus;
    I2cDevice test_device;
    I2cResponse response;
    uint8_t read_data;

    response = I2C_RESPONSE_ACK;
    read_data = 0U;

    printf("\n");
    printf(
        "----- Test 5: Invalid Sequences -----\n");

    if(i2c_init(
           &test_bus,
           I2C_STANDARD_CLOCK_HZ) == 0)
    {
        printf(
            "[ERROR] Temporary bus init failed\n");
        return;
    }

    if(i2c_device_init(
           &test_device,
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DEVICE_DEFAULT_READ_DATA) == 0)
    {
        printf(
            "[ERROR] Temporary device init failed\n");
        return;
    }

    /*
     * 没有 START 时不能发送地址。
     */
    if(i2c_send_address(
           &test_bus,
           &test_device,
           test_device.address,
           I2C_DIRECTION_WRITE,
           &response) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Address rejected before START\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Address accepted before START\n");
    }

    /*
     * 没有 START 时不能产生 STOP。
     */
    if(i2c_stop(&test_bus) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "STOP rejected before START\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "STOP accepted before START\n");
    }

    /*
     * 第一次 START 应成功。
     */
    if(i2c_start(&test_bus) == 0)
    {
        printf(
            "[ERROR] First START failed\n");
        return;
    }

    printf(
        "[I2C TEST] First START succeeded\n");

    /*
     * Day25 不支持 Repeated START，
     * 因此重复 START 应失败。
     */
    if(i2c_start(&test_bus) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Repeated START rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Repeated START accepted\n");
    }

    /*
     * START 后还没有发送地址，
     * 不能直接写数据。
     */
    if(i2c_write_byte(
           &test_bus,
           &test_device,
           0x11U,
           &response) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Write rejected before address\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Write accepted before address\n");
    }

    /*
     * 发送 READ 地址。
     */
    if(i2c_send_address(
           &test_bus,
           &test_device,
           test_device.address,
           I2C_DIRECTION_READ,
           &response) == 0)
    {
        printf(
            "[ERROR] Read address operation failed\n");

        (void)i2c_stop(&test_bus);
        return;
    }

    /*
     * 当前方向为 READ，
     * 因此写函数应被拒绝。
     */
    if(i2c_write_byte(
           &test_bus,
           &test_device,
           0x22U,
           &response) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Write rejected in READ direction\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Write accepted in READ direction\n");
    }

    if(i2c_stop(&test_bus) == 0)
    {
        printf(
            "[ERROR] Temporary STOP failed\n");
        return;
    }

    /*
     * 再开始一个 WRITE 事务。
     */
    if(i2c_start(&test_bus) == 0)
    {
        printf(
            "[ERROR] Second START failed\n");
        return;
    }

    if(i2c_send_address(
           &test_bus,
           &test_device,
           test_device.address,
           I2C_DIRECTION_WRITE,
           &response) == 0)
    {
        printf(
            "[ERROR] Write address operation failed\n");

        (void)i2c_stop(&test_bus);
        return;
    }

    /*
     * 当前方向为 WRITE，
     * 因此读函数应被拒绝。
     */
    if(i2c_read_byte(
           &test_bus,
           &test_device,
           &read_data,
           I2C_RESPONSE_NACK) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Read rejected in WRITE direction\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Read accepted in WRITE direction\n");
    }

    if(i2c_stop(&test_bus) == 0)
    {
        printf(
            "[ERROR] Final temporary STOP failed\n");
        return;
    }

    printf(
        "[CHECK] Invalid sequence test completed\n");
}

/*
 * 测试无效初始化参数。
 */
static void test_invalid_parameters(void)
{
    I2cBus test_bus;
    I2cDevice test_device;
    uint8_t address_byte;

    address_byte = 0U;

    printf("\n");
    printf(
        "----- Test 6: Invalid Parameters -----\n");

    if(i2c_init(
           NULL,
           I2C_STANDARD_CLOCK_HZ) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "NULL bus rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "NULL bus accepted\n");
    }

    if(i2c_init(
           &test_bus,
           0U) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Zero clock rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Zero clock accepted\n");
    }

    if(i2c_device_init(
           NULL,
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DEVICE_DEFAULT_READ_DATA) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "NULL device rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "NULL device accepted\n");
    }

    if(i2c_build_address_byte(
           I2C_DEVICE_DEFAULT_ADDRESS,
           (I2cDirection)2,
           &address_byte) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Invalid direction rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Invalid direction accepted\n");
    }

    if(i2c_build_address_byte(
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DIRECTION_WRITE,
           NULL) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "NULL address output rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "NULL address output accepted\n");
    }

    /*
     * 防止未使用变量警告。
     */
    (void)test_device;
}

int main(void)
{
    I2cBus bus;
    I2cDevice device;

    printf("\n");
    printf(
        "===== Day25 I2C Basic Communication Test =====\n");

    /*
     * 初始化 I2C 总线，使用标准模式 100 kHz。
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
     * 初始化地址为 0x48 的模拟从机。
     *
     * 主机读取时，从机返回 0x5A。
     */
    if(i2c_device_init(
           &device,
           I2C_DEVICE_DEFAULT_ADDRESS,
           I2C_DEVICE_DEFAULT_READ_DATA) == 0)
    {
        printf(
            "[FATAL] I2C device initialization failed\n");

        return 1;
    }

    printf(
        "[I2C CONFIG] Clock = %lu Hz\n",
        (unsigned long)bus.clock_hz);

    printf(
        "[DEVICE CONFIG] 7-bit address = 0x%02X\n",
        (unsigned int)device.address);

    printf(
        "[DEVICE CONFIG] Read data = 0x%02X\n",
        (unsigned int)device.read_data);

    test_address_byte_build();

    test_single_byte_write(
        &bus,
        &device);

    test_single_byte_read(
        &bus,
        &device);

    test_wrong_address(
        &bus,
        &device);

    test_invalid_sequences();

    test_invalid_parameters();

    printf("\n");
    printf(
        "----- Final I2C Status -----\n");

    printf(
        "[FINAL] START count = %lu\n",
        (unsigned long)bus.start_count);

    printf(
        "[FINAL] STOP count  = %lu\n",
        (unsigned long)bus.stop_count);

    printf(
        "[FINAL] ACK count   = %lu\n",
        (unsigned long)bus.ack_count);

    printf(
        "[FINAL] NACK count  = %lu\n",
        (unsigned long)bus.nack_count);

    printf(
        "[FINAL] Byte count  = %lu\n",
        (unsigned long)bus.byte_count);

    printf(
        "[FINAL] Bus started = %s\n",
        bus.started ?
        "YES" :
        "NO");

    printf(
        "[FINAL] Addressed   = %s\n",
        bus.addressed ?
        "YES" :
        "NO");

    printf(
        "[FINAL] Stored data = 0x%02X\n",
        (unsigned int)device.stored_data);

    /*
     * 主总线共完成三个事务：
     *
     * 1. 单字节写入
     *    START 1
     *    STOP  1
     *    ACK   2
     *    BYTE  2
     *
     * 2. 单字节读取
     *    START 1
     *    STOP  1
     *    ACK   1
     *    NACK  1
     *    BYTE  2
     *
     * 3. 错误地址
     *    START 1
     *    STOP  1
     *    NACK  1
     *    BYTE  1
     *
     * 总计：
     *
     * START = 3
     * STOP  = 3
     * ACK   = 3
     * NACK  = 2
     * BYTE  = 5
     */
    if((bus.start_count == 3U) &&
       (bus.stop_count == 3U) &&
       (bus.ack_count == 3U) &&
       (bus.nack_count == 2U) &&
       (bus.byte_count == 5U) &&
       (bus.started == 0U) &&
       (bus.addressed == 0U) &&
       (device.stored_data == 0xA5U))
    {
        printf(
            "[FINAL CHECK] All I2C tests passed\n");
    }
    else
    {
        printf(
            "[FINAL CHECK ERROR] "
            "I2C result mismatch\n");
    }

    printf("\n");
    printf(
        "===== I2C Test Finished =====\n");

    return 0;
}