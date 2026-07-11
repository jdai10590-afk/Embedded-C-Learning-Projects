#ifndef CAN_SIM_H
#define CAN_SIM_H

#include <stdint.h>

/*
 * 经典 CAN 一帧最多携带 8 字节数据。
 */
#define CAN_MAX_DATA_LEN       8U

/*
 * 标准 CAN 使用 11 位标识符。
 * 最大标准帧 ID 为 0x7FF。
 */
#define CAN_STANDARD_ID_MAX    0x7FFU

/*
 * 本项目使用的消息 ID。
 */
#define CAN_ID_MOTOR_STATUS    0x101U
#define CAN_ID_BATTERY_STATUS  0x201U

/*
 * CAN 数据帧。
 */
typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN_MAX_DATA_LEN];
} CanFrame;

/*
 * 简单的 CAN 接收过滤器。
 *
 * 当前阶段只接收一个指定 ID。
 */
typedef struct
{
    uint32_t accepted_id;
} CanFilter;

/*
 * 初始化 CAN 数据帧。
 */
void can_frame_init(CanFrame *frame,
                    uint32_t id);

/*
 * 向 CAN 帧写入数据。
 *
 * 成功返回 1，失败返回 0。
 */
int can_frame_set_data(CanFrame *frame,
                       const uint8_t *data,
                       uint8_t dlc);

/*
 * 打印 CAN 帧。
 */
void can_print_frame(const char *direction,
                     const CanFrame *frame);

/*
 * 初始化接收过滤器。
 */
void can_filter_init(CanFilter *filter,
                     uint32_t accepted_id);

/*
 * 判断当前 CAN 帧是否通过过滤器。
 *
 * 接收返回 1，拒绝返回 0。
 */
int can_filter_accept(const CanFilter *filter,
                      const CanFrame *frame);

/*
 * 将电机状态打包成 CAN 数据帧。
 *
 * data[0]：转速高字节
 * data[1]：转速低字节
 * data[2]：温度
 * data[3]：故障码
 */
int can_pack_motor_status(CanFrame *frame,
                          uint16_t speed_rpm,
                          int8_t temperature_c,
                          uint8_t fault_code);

/*
 * 从电机状态帧中解析数据。
 *
 * 成功返回 1，失败返回 0。
 */
int can_parse_motor_status(const CanFrame *frame,
                           uint16_t *speed_rpm,
                           int8_t *temperature_c,
                           uint8_t *fault_code);

#endif