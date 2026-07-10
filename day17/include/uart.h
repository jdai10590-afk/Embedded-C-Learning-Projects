#ifndef UART_H
#define UART_H

#include "system_type.h"
#include "ring_buffer.h"

#define UART_COMMAND_MAX_LENGTH 32

void uart_process_command(SystemStatus *sys, const char *command);

int uart_receive_byte(RingBuffer *rx_buffer, char byte);

void uart_process_rx_buffer(SystemStatus *sys,
                            RingBuffer *rx_buffer);

#endif