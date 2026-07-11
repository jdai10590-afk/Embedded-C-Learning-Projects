#include <stdio.h>

#include "can.h"

/*
 * 模拟接收节点处理一帧 CAN 消息。
 *
 * filter：
 * 接收节点使用的 CAN 过滤器。
 *
 * bus_frame：
 * 当前出现在模拟 CAN 总线上的数据帧。
 */
static void process_received_frame(
    const CanFilter *filter,
    const CanFrame *bus_frame)
{
    CanFrame rx_frame;

    uint16_t motor_speed_rpm;
    int8_t motor_temperature_c;
    uint8_t motor_fault_code;

    /*
     * 模拟 CAN 控制器把总线数据复制到接收缓冲区。
     *
     * CanFrame 是结构体，因此可以直接整体赋值。
     */
    rx_frame = *bus_frame;

    can_print_frame("RX", &rx_frame);

    /*
     * 先根据 CAN ID 判断该帧是否应该接收。
     */
    if(can_filter_accept(filter, &rx_frame) == 0)
    {
        printf(
            "[CAN RX] ID=0x%03X rejected by filter\n",
            (unsigned int)rx_frame.id);

        return;
    }

    printf(
        "[CAN RX] ID=0x%03X accepted by filter\n",
        (unsigned int)rx_frame.id);

    /*
     * 当前过滤器只接收电机状态帧，
     * 因此通过过滤后进行电机状态解析。
     */
    if(can_parse_motor_status(
           &rx_frame,
           &motor_speed_rpm,
           &motor_temperature_c,
           &motor_fault_code))
    {
        printf(
            "[PARSE] Motor speed       = %u rpm\n",
            (unsigned int)motor_speed_rpm);

        printf(
            "[PARSE] Motor temperature = %d C\n",
            (int)motor_temperature_c);

        printf(
            "[PARSE] Motor fault code  = %u\n",
            (unsigned int)motor_fault_code);
    }
    else
    {
        printf("[PARSE] Motor status parse failed\n");
    }
}

int main(void)
{
    CanFrame motor_tx_frame;
    CanFrame battery_tx_frame;

    CanFilter motor_status_filter;

    uint8_t battery_payload[4] =
    {
        0x04U,
        0xE4U,
        0x00U,
        0x78U
    };

    printf("\n");
    printf("===== Day21 CAN Frame Simulation =====\n");

    /*
     * 接收节点只接受电机状态消息 0x101。
     */
    can_filter_init(
        &motor_status_filter,
        CAN_ID_MOTOR_STATUS);

    printf(
        "[CAN FILTER] Accepted ID = 0x%03X\n",
        (unsigned int)motor_status_filter.accepted_id);

    /*
     * 测试一：
     * 电机节点发送 ID=0x101 的电机状态帧。
     */
    printf("\n");
    printf("----- Test 1: Motor Status Frame -----\n");

    if(can_pack_motor_status(
           &motor_tx_frame,
           1500U,
           65,
           0U))
    {
        can_print_frame("TX", &motor_tx_frame);

        process_received_frame(
            &motor_status_filter,
            &motor_tx_frame);
    }
    else
    {
        printf("[CAN TX] Motor frame pack failed\n");
    }

    /*
     * 测试二：
     * 电池节点发送 ID=0x201 的电池状态帧。
     *
     * 当前接收过滤器只接受 0x101，
     * 因此 0x201 应被拒绝。
     */
    printf("\n");
    printf("----- Test 2: Battery Status Frame -----\n");

    can_frame_init(
        &battery_tx_frame,
        CAN_ID_BATTERY_STATUS);

    if(can_frame_set_data(
           &battery_tx_frame,
           battery_payload,
           4U))
    {
        can_print_frame("TX", &battery_tx_frame);

        process_received_frame(
            &motor_status_filter,
            &battery_tx_frame);
    }
    else
    {
        printf("[CAN TX] Battery frame creation failed\n");
    }

    printf("\n");
    printf("===== CAN Test Finished =====\n");

    return 0;
}