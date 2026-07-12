#ifndef ADC_SIM_H
#define ADC_SIM_H

#include <stdint.h>

/*
 * ADC 基本参数。
 *
 * 12 位 ADC 一共有 2^12 = 4096 个量化等级，
 * 数字量范围为 0～4095。
 */
#define ADC_RESOLUTION_BITS       12U

#define ADC_MAX_VALUE             \
    ((1U << ADC_RESOLUTION_BITS) - 1U)

/*
 * ADC 参考电压为 3.3 V。
 *
 * 后缀 f 表示该常量为 float 类型。
 */
#define ADC_REFERENCE_VOLTAGE     3.3f

/*
 * 滑动平均滤波窗口大小。
 *
 * 当前保存最近 4 个 ADC 采样值。
 */
#define ADC_MOVING_AVERAGE_WINDOW 4U

/*
 * ADC 滑动平均滤波器。
 */
typedef struct
{
    uint16_t buffer[ADC_MOVING_AVERAGE_WINDOW];

    uint32_t sum;

    uint8_t index;
    uint8_t count;
} AdcMovingAverage;

/*
 * 判断 ADC 原始值是否合法。
 *
 * 合法返回 1，非法返回 0。
 */
int adc_raw_is_valid(uint16_t raw_value);

/*
 * 将 ADC 原始值换算成电压。
 *
 * raw_value：
 * ADC 原始数字量，范围为 0～4095。
 *
 * voltage：
 * 用于保存换算后的电压。
 *
 * 成功返回 1，失败返回 0。
 */
int adc_raw_to_voltage(uint16_t raw_value,
                       float *voltage);

/*
 * 计算一组 ADC 原始采样值的平均值。
 *
 * samples：
 * 指向采样数组。
 *
 * sample_count：
 * 数组中的有效采样数量。
 *
 * average_value：
 * 用于保存最终平均值。
 *
 * 成功返回 1，失败返回 0。
 */
int adc_average_raw(const uint16_t *samples,
                    uint8_t sample_count,
                    uint16_t *average_value);

/*
 * 初始化滑动平均滤波器。
 */
void adc_moving_average_init(
    AdcMovingAverage *filter);

/*
 * 向滑动平均滤波器加入一个新采样值。
 *
 * raw_value：
 * 新的 ADC 原始值。
 *
 * filtered_value：
 * 用于保存当前滤波结果。
 *
 * 成功返回 1，失败返回 0。
 */
int adc_moving_average_update(
    AdcMovingAverage *filter,
    uint16_t raw_value,
    uint16_t *filtered_value);

#endif