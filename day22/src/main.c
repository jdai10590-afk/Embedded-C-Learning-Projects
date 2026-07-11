#include <stdio.h>

#include "adc.h"

/*
 * 测试 ADC 原始值与电压之间的换算。
 */
static void test_voltage_conversion(void)
{
    uint16_t raw_samples[3] =
    {
        0U,
        2048U,
        4095U
    };

    float voltage;
    uint8_t index;

    printf("\n");
    printf("----- Test 1: ADC Voltage Conversion -----\n");

    for(index = 0U; index < 3U; index++)
    {
        if(adc_raw_to_voltage(raw_samples[index],
                              &voltage))
        {
            printf(
                "[ADC] Raw = %4u, Voltage = %.3f V\n",
                (unsigned int)raw_samples[index],
                voltage);
        }
        else
        {
            printf(
                "[ADC] Raw = %4u, conversion failed\n",
                (unsigned int)raw_samples[index]);
        }
    }
}

/*
 * 测试一组 ADC 数据的批量平均滤波。
 */
static void test_batch_average(void)
{
    uint16_t samples[8] =
    {
        2048U,
        2055U,
        2038U,
        2060U,
        2044U,
        2052U,
        2040U,
        2058U
    };

    uint16_t average_raw;
    float average_voltage;
    uint8_t index;

    printf("\n");
    printf("----- Test 2: Batch Average Filter -----\n");

    printf("[SAMPLES] ");

    for(index = 0U; index < 8U; index++)
    {
        printf("%u",
               (unsigned int)samples[index]);

        if(index + 1U < 8U)
        {
            printf(", ");
        }
    }

    printf("\n");

    if(adc_average_raw(samples,
                       8U,
                       &average_raw))
    {
        printf(
            "[AVERAGE] Raw average = %u\n",
            (unsigned int)average_raw);

        if(adc_raw_to_voltage(average_raw,
                              &average_voltage))
        {
            printf(
                "[AVERAGE] Voltage     = %.3f V\n",
                average_voltage);
        }
    }
    else
    {
        printf("[AVERAGE] Calculation failed\n");
    }
}

/*
 * 测试连续采样情况下的滑动平均滤波。
 */
static void test_moving_average(void)
{
    uint16_t samples[8] =
    {
        2048U,
        2055U,
        2038U,
        2060U,
        2070U,
        2025U,
        2050U,
        2045U
    };

    AdcMovingAverage filter;

    uint16_t filtered_raw;
    float raw_voltage;
    float filtered_voltage;

    uint8_t index;

    adc_moving_average_init(&filter);

    printf("\n");
    printf("----- Test 3: Moving Average Filter -----\n");

    for(index = 0U; index < 8U; index++)
    {
        if(adc_moving_average_update(
               &filter,
               samples[index],
               &filtered_raw))
        {
            adc_raw_to_voltage(samples[index],
                               &raw_voltage);

            adc_raw_to_voltage(filtered_raw,
                               &filtered_voltage);

            printf(
                "[SAMPLE %u] Raw=%4u (%.3f V), "
                "Filtered=%4u (%.3f V), "
                "Count=%u\n",
                (unsigned int)(index + 1U),
                (unsigned int)samples[index],
                raw_voltage,
                (unsigned int)filtered_raw,
                filtered_voltage,
                (unsigned int)filter.count);
        }
        else
        {
            printf(
                "[SAMPLE %u] Moving average failed\n",
                (unsigned int)(index + 1U));
        }
    }
}

/*
 * 测试超出 12 位 ADC 范围的非法数据。
 */
static void test_invalid_samples(void)
{
    uint16_t invalid_raw;
    uint16_t invalid_samples[3];

    uint16_t average_raw;
    uint16_t filtered_raw;

    float voltage;

    AdcMovingAverage filter;

    invalid_raw = 5000U;

    invalid_samples[0] = 2048U;
    invalid_samples[1] = 5000U;
    invalid_samples[2] = 2050U;

    adc_moving_average_init(&filter);

    printf("\n");
    printf("----- Test 4: Invalid ADC Data -----\n");

    if(adc_raw_to_voltage(invalid_raw,
                          &voltage) == 0)
    {
        printf(
            "[INVALID] Raw value %u rejected "
            "during voltage conversion\n",
            (unsigned int)invalid_raw);
    }

    if(adc_average_raw(invalid_samples,
                       3U,
                       &average_raw) == 0)
    {
        printf(
            "[INVALID] Sample array rejected "
            "because it contains value 5000\n");
    }

    if(adc_moving_average_update(
           &filter,
           invalid_raw,
           &filtered_raw) == 0)
    {
        printf(
            "[INVALID] Raw value %u rejected "
            "by moving average filter\n",
            (unsigned int)invalid_raw);
    }
}

int main(void)
{
    printf("\n");
    printf("===== Day22 ADC Sampling and Filter Test =====\n");

    printf(
        "[ADC CONFIG] Resolution = %u bits\n",
        (unsigned int)ADC_RESOLUTION_BITS);

    printf(
        "[ADC CONFIG] Maximum value = %u\n",
        (unsigned int)ADC_MAX_VALUE);

    printf(
        "[ADC CONFIG] Reference voltage = %.1f V\n",
        ADC_REFERENCE_VOLTAGE);

    printf(
        "[ADC CONFIG] Moving average window = %u\n",
        (unsigned int)ADC_MOVING_AVERAGE_WINDOW);

    test_voltage_conversion();
    test_batch_average();
    test_moving_average();
    test_invalid_samples();

    printf("\n");
    printf("===== ADC Test Finished =====\n");

    return 0;
}