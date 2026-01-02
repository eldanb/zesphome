#pragma once

#include "freertos/FreeRTOS.h"

void aseer_set_output_queue(QueueHandle_t queue);
void aseer_handle_state_interrupt(int state);