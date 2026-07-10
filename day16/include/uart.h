#ifndef UART_H
#define UART_H

#include "system_type.h"

void uart_process_command(SystemStatus *sys, const char *command);

#endif