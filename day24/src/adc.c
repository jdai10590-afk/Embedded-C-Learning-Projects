#include <stddef.h>

#include "adc.h"

int adc_raw_is_valid(uint16_t raw_value)
{
    if(raw_value <= ADC_MAX_VALUE)
    {
        return 1;
    }

    return 0;
}

int adc_raw_to_voltage(uint16_t raw_value,
                       float *voltage)
{
    if(voltage == NULL)
    {
        return 0;
    }

    if(adc_raw_is_valid(raw_value) == 0)
    {
        return 0;
    }

    /*
     * ADC 电压换算公式：
     *
     * voltage =
     *     raw_value / ADC_MAX_VALUE
     *     × ADC_REFERENCE_VOLTAGE
     *
     * 必须转换为 float，避免整数除法丢失小数。
     */
    *voltage =
        ((float)raw_value /
         (float)ADC_MAX_VALUE) *
        ADC_REFERENCE_VOLTAGE;

    return 1;
}

int adc_average_raw(const uint16_t *samples,
                    uint8_t sample_count,
                    uint16_t *average_value)
{
    uint32_t sum;
    uint8_t index;

    if((samples == NULL) ||
       (average_value == NULL))
    {
        return 0;
    }

    if(sample_count == 0U)
    {
        return 0;
    }

    sum = 0U;

    for(index = 0U;
        index < sample_count;
        index++)
    {
        if(adc_raw_is_valid(samples[index]) == 0)
        {
            return 0;
        }

        sum += samples[index];
    }

    /*
     * 当前使用整数平均值。
     *
     * 除法产生的小数部分会被舍弃。
     */
    *average_value =
        (uint16_t)(sum / sample_count);

    return 1;
}

void adc_moving_average_init(
    AdcMovingAverage *filter)
{
    uint8_t index;

    if(filter == NULL)
    {
        return;
    }

    for(index = 0U;
        index < ADC_MOVING_AVERAGE_WINDOW;
        index++)
    {
        filter->buffer[index] = 0U;
    }

    filter->sum = 0U;
    filter->index = 0U;
    filter->count = 0U;
}

int adc_moving_average_update(
    AdcMovingAverage *filter,
    uint16_t raw_value,
    uint16_t *filtered_value)
{
    uint16_t old_value;

    if((filter == NULL) ||
       (filtered_value == NULL))
    {
        return 0;
    }

    if(adc_raw_is_valid(raw_value) == 0)
    {
        return 0;
    }

    /*
     * 读取当前位置原来的旧数据。
     *
     * 窗口尚未装满时，旧数据初始为 0。
     */
    old_value =
        filter->buffer[filter->index];

    /*
     * 滑动窗口高效更新：
     *
     * 新总和 = 旧总和 - 被替换的旧值 + 新值
     */
    filter->sum -= old_value;

    filter->buffer[filter->index] =
        raw_value;

    filter->sum += raw_value;

    /*
     * 移动到下一个数组位置。
     */
    filter->index++;

    if(filter->index >=
       ADC_MOVING_AVERAGE_WINDOW)
    {
        filter->index = 0U;
    }

    /*
     * 窗口没有装满时，count 逐步增加。
     *
     * 窗口装满后，count 保持为窗口大小。
     */
    if(filter->count <
       ADC_MOVING_AVERAGE_WINDOW)
    {
        filter->count++;
    }

    /*
     * 使用当前有效采样数量计算平均值。
     *
     * 第一次加入数据时除以 1，
     * 第二次加入数据时除以 2，
     * 窗口装满后始终除以 4。
     */
    *filtered_value =
        (uint16_t)(
            filter->sum /
            filter->count);

    return 1;
}