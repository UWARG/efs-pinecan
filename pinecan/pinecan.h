#pragma once

#include "canard.h"
#include <stdint.h>

typedef enum PineCAN_Status_e {
    PINECAN_OK = 0,
    PINECAN_ERROR = 1
} PineCAN_Status;

/**
 * @brief  Process PineCAN tasks every 1 ms
 * @retval None
 */
PineCAN_Status pinecan1ms(void);
