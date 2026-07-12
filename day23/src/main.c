#include <stdio.h>

#include "spi.h"

/*
 * 测试正常的 SPI 单字节全双工传输。
 *
 * 主机发送 0xA5，
 * 从机在同一组时钟周期内返回 0x5A。
 */
static void test_full_duplex_transfer(
    SpiBus *spi)
{
    uint8_t tx_data;
    uint8_t rx_data;

    tx_data = 0xA5U;
    rx_data = 0U;

    printf("\n");
    printf(
        "----- Test 1: Full-Duplex Transfer -----\n");

    /*
     * 设置模拟从机下一次返回 0x5A。
     */
    if(spi_set_slave_response(spi,
                              0x5AU) == 0)
    {
        printf(
            "[ERROR] Failed to set slave response\n");
        return;
    }

    /*
     * 开始一次 SPI 事务。
     *
     * 对低电平有效的片选信号来说，
     * 这里相当于将 CS 拉低。
     */
    if(spi_select(spi) == 0)
    {
        printf(
            "[ERROR] Failed to select SPI device\n");
        return;
    }

    printf(
        "[SPI] CS LOW - Device selected\n");

    /*
     * 发送 0xA5 的同时接收从机返回的数据。
     */
    if(spi_transfer_byte(spi,
                         tx_data,
                         &rx_data))
    {
        printf(
            "[SPI TX] Master sent     : 0x%02X\n",
            (unsigned int)tx_data);

        printf(
            "[SPI RX] Master received : 0x%02X\n",
            (unsigned int)rx_data);
    }
    else
    {
        printf(
            "[ERROR] SPI transfer failed\n");
    }

    /*
     * 结束本次 SPI 事务。
     *
     * 对低电平有效的片选信号来说，
     * 这里相当于将 CS 拉高。
     */
    if(spi_deselect(spi))
    {
        printf(
            "[SPI] CS HIGH - Device released\n");
    }
    else
    {
        printf(
            "[ERROR] Failed to release SPI device\n");
    }

    printf(
        "[SPI COUNT] Successful transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试主机只想读取数据时的处理方式。
 *
 * 主机必须发送一个占位字节，
 * 才能产生 SPI 时钟，让从机移出数据。
 */
static void test_dummy_byte_read(
    SpiBus *spi)
{
    uint8_t device_id;

    device_id = 0U;

    printf("\n");
    printf(
        "----- Test 2: Dummy Byte Read -----\n");

    /*
     * 模拟从机准备返回设备 ID 0x42。
     */
    if(spi_set_slave_response(spi,
                              0x42U) == 0)
    {
        printf(
            "[ERROR] Failed to prepare device ID\n");
        return;
    }

    if(spi_select(spi) == 0)
    {
        printf(
            "[ERROR] Failed to select SPI device\n");
        return;
    }

    printf(
        "[SPI] CS LOW - Device selected\n");

    /*
     * 主机发送 0xFF，只是为了产生 8 个时钟。
     *
     * 主机真正关心的是从机返回的 device_id。
     */
    if(spi_transfer_byte(spi,
                         SPI_DUMMY_BYTE,
                         &device_id))
    {
        printf(
            "[SPI TX] Dummy byte       : 0x%02X\n",
            (unsigned int)SPI_DUMMY_BYTE);

        printf(
            "[SPI RX] Device ID        : 0x%02X\n",
            (unsigned int)device_id);
    }
    else
    {
        printf(
            "[ERROR] SPI read failed\n");
    }

    if(spi_deselect(spi))
    {
        printf(
            "[SPI] CS HIGH - Device released\n");
    }
    else
    {
        printf(
            "[ERROR] Failed to release SPI device\n");
    }

    printf(
        "[SPI COUNT] Successful transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试从机没有被选择时直接传输。
 *
 * 初始化完成后 chip_selected 为 0，
 * 因此传输应该被拒绝。
 */
static void test_transfer_without_select(
    SpiBus *spi)
{
    uint8_t rx_data;
    uint32_t count_before;

    rx_data = 0U;
    count_before = spi->transfer_count;

    printf("\n");
    printf(
        "----- Test 3: Transfer Without CS -----\n");

    if(spi_transfer_byte(spi,
                         0x33U,
                         &rx_data) == 0)
    {
        printf(
            "[EXPECTED ERROR] Transfer rejected "
            "because CS is HIGH\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] Transfer succeeded\n");
    }

    /*
     * 失败的传输不能增加成功传输次数。
     */
    if(spi->transfer_count == count_before)
    {
        printf(
            "[CHECK] Transfer count unchanged: %lu\n",
            (unsigned long)spi->transfer_count);
    }
    else
    {
        printf(
            "[CHECK ERROR] Transfer count changed\n");
    }
}

/*
 * 测试不规范的片选调用顺序。
 */
static void test_chip_select_sequence(
    SpiBus *spi)
{
    printf("\n");
    printf(
        "----- Test 4: Chip Select Sequence -----\n");

    /*
     * 第一次选择应该成功。
     */
    if(spi_select(spi))
    {
        printf(
            "[SPI] First select succeeded\n");
    }
    else
    {
        printf(
            "[ERROR] First select failed\n");
        return;
    }

    /*
     * 当前设备已经被选择，
     * 再次选择应该失败。
     */
    if(spi_select(spi) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Duplicate select rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] Duplicate select succeeded\n");
    }

    /*
     * 第一次释放应该成功。
     */
    if(spi_deselect(spi))
    {
        printf(
            "[SPI] First deselect succeeded\n");
    }
    else
    {
        printf(
            "[ERROR] First deselect failed\n");
        return;
    }

    /*
     * 当前设备已经被释放，
     * 再次释放应该失败。
     */
    if(spi_deselect(spi) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Duplicate deselect rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Duplicate deselect succeeded\n");
    }

    printf(
        "[SPI COUNT] Successful transfers = %lu\n",
        (unsigned long)spi->transfer_count);
}

/*
 * 测试无效 SPI 初始化参数。
 */
static void test_invalid_configuration(void)
{
    SpiBus test_spi;

    printf("\n");
    printf(
        "----- Test 5: Invalid Configuration -----\n");

    /*
     * SPI 时钟为 0 时，初始化应该失败。
     */
    if(spi_init(&test_spi,
                0U,
                SPI_MODE_0) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Zero SPI clock rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Zero SPI clock accepted\n");
    }

    /*
     * SPI 只有 Mode 0～Mode 3。
     *
     * 强制传入 5，用于验证模式检查。
     */
    if(spi_init(&test_spi,
                1000000U,
                (SpiMode)5) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Invalid SPI mode rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "Invalid SPI mode accepted\n");
    }

    /*
     * 空指针初始化也必须失败。
     */
    if(spi_init(NULL,
                1000000U,
                SPI_MODE_0) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "NULL SPI pointer rejected\n");
    }
    else
    {
        printf(
            "[UNEXPECTED] "
            "NULL SPI pointer accepted\n");
    }
}

int main(void)
{
    SpiBus spi;

    printf("\n");
    printf(
        "===== Day23 SPI Full-Duplex Test =====\n");

    /*
     * 使用 1 MHz、Mode 0 初始化 SPI。
     */
    if(spi_init(&spi,
                1000000U,
                SPI_MODE_0) == 0)
    {
        printf(
            "[FATAL] SPI initialization failed\n");

        return 1;
    }

    printf(
        "[SPI CONFIG] Clock = %lu Hz\n",
        (unsigned long)spi.clock_hz);

    printf(
        "[SPI CONFIG] Mode = %u\n",
        (unsigned int)spi.mode);

    printf(
        "[SPI CONFIG] Dummy byte = 0x%02X\n",
        (unsigned int)SPI_DUMMY_BYTE);

    printf(
        "[SPI CONFIG] Initial CS state = %s\n",
        spi.chip_selected ?
        "LOW / selected" :
        "HIGH / released");

    test_full_duplex_transfer(&spi);
    test_dummy_byte_read(&spi);
    test_transfer_without_select(&spi);
    test_chip_select_sequence(&spi);
    test_invalid_configuration();

    printf("\n");
    printf(
        "----- Final SPI Status -----\n");

    printf(
        "[SPI STATUS] Successful transfers = %lu\n",
        (unsigned long)spi.transfer_count);

    printf(
        "[SPI STATUS] Device selected = %s\n",
        spi.chip_selected ?
        "YES" :
        "NO");

    printf("\n");
    printf(
        "===== SPI Test Finished =====\n");

    return 0;
}