#include <stdio.h>

#include "can.h"

void can_frame_init(CanFrame *frame,
                    uint32_t id)
{
    uint8_t index;

    if(frame == NULL)
    {
        return;
    }

    /*
     * 当前只模拟标准 CAN 帧。
     * 标准帧 ID 最大为 0x7FF。
     */
    if(id > CAN_STANDARD_ID_MAX)
    {
        id = 0U;
    }

    frame->id = id;
    frame->dlc = 0U;

    for(index = 0U;
        index < CAN_MAX_DATA_LEN;
        index++)
    {
        frame->data[index] = 0U;
    }
}

int can_frame_set_data(CanFrame *frame,
                       const uint8_t *data,
                       uint8_t dlc)
{
    uint8_t index;

    if(frame == NULL)
    {
        return 0;
    }

    /*
     * 经典 CAN 数据区最大为 8 字节。
     */
    if(dlc > CAN_MAX_DATA_LEN)
    {
        return 0;
    }

    /*
     * DLC 大于 0 时必须提供有效数据地址。
     * DLC 等于 0 时允许 data 为 NULL。
     */
    if((dlc > 0U) && (data == NULL))
    {
        return 0;
    }

    /*
     * 先清空整个数据区，避免残留旧数据。
     */
    for(index = 0U;
        index < CAN_MAX_DATA_LEN;
        index++)
    {
        frame->data[index] = 0U;
    }

    /*
     * 只复制 DLC 指定的有效字节。
     */
    for(index = 0U;
        index < dlc;
        index++)
    {
        frame->data[index] = data[index];
    }

    frame->dlc = dlc;

    return 1;
}

void can_print_frame(const char *direction,
                     const CanFrame *frame)
{
    uint8_t index;

    if(direction == NULL)
    {
        direction = "?";
    }

    if(frame == NULL)
    {
        printf("[CAN %s] Invalid frame\n",
               direction);
        return;
    }

    printf("[CAN %s] ID=0x%03X DLC=%u DATA=",
           direction,
           (unsigned int)frame->id,
           (unsigned int)frame->dlc);

    if(frame->dlc == 0U)
    {
        printf("<empty>");
    }
    else
    {
        for(index = 0U;
            index < frame->dlc &&
            index < CAN_MAX_DATA_LEN;
            index++)
        {
            printf("%02X",
                   (unsigned int)frame->data[index]);

            if(index + 1U < frame->dlc)
            {
                printf(" ");
            }
        }
    }

    printf("\n");
}

void can_filter_init(CanFilter *filter,
                     uint32_t accepted_id)
{
    if(filter == NULL)
    {
        return;
    }

    if(accepted_id > CAN_STANDARD_ID_MAX)
    {
        accepted_id = 0U;
    }

    filter->accepted_id = accepted_id;
}

int can_filter_accept(const CanFilter *filter,
                      const CanFrame *frame)
{
    if((filter == NULL) || (frame == NULL))
    {
        return 0;
    }

    if(frame->id > CAN_STANDARD_ID_MAX)
    {
        return 0;
    }

    if(frame->id == filter->accepted_id)
    {
        return 1;
    }

    return 0;
}

int can_pack_motor_status(CanFrame *frame,
                          uint16_t speed_rpm,
                          int8_t temperature_c,
                          uint8_t fault_code)
{
    uint8_t payload[4];

    if(frame == NULL)
    {
        return 0;
    }

    /*
     * 16 位转速拆成两个 8 位字节。
     *
     * data[0]：高字节
     * data[1]：低字节
     */
    payload[0] =
        (uint8_t)((speed_rpm >> 8) & 0xFFU);

    payload[1] =
        (uint8_t)(speed_rpm & 0xFFU);

    /*
     * 温度和故障码各占一个字节。
     */
    payload[2] = (uint8_t)temperature_c;
    payload[3] = fault_code;

    can_frame_init(frame,
                   CAN_ID_MOTOR_STATUS);

    return can_frame_set_data(frame,
                              payload,
                              4U);
}

int can_parse_motor_status(const CanFrame *frame,
                           uint16_t *speed_rpm,
                           int8_t *temperature_c,
                           uint8_t *fault_code)
{
    if((frame == NULL) ||
       (speed_rpm == NULL) ||
       (temperature_c == NULL) ||
       (fault_code == NULL))
    {
        return 0;
    }

    /*
     * 必须是电机状态帧。
     */
    if(frame->id != CAN_ID_MOTOR_STATUS)
    {
        return 0;
    }

    /*
     * 电机状态协议需要至少 4 个数据字节。
     */
    if(frame->dlc < 4U)
    {
        return 0;
    }

    /*
     * 将高字节和低字节重新组合成 16 位转速。
     */
    *speed_rpm =
        (uint16_t)(
            ((uint16_t)frame->data[0] << 8) |
            (uint16_t)frame->data[1]);

    *temperature_c =
        (int8_t)frame->data[2];

    *fault_code =
        frame->data[3];

    return 1;
}