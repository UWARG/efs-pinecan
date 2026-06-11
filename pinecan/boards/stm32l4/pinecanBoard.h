#pragma once

#include "stm32l4xx_hal.h"
#include "canard.h"
#include "pinecan.h"

typedef struct {
    CAN_HandleTypeDef *hcan;
    CanardInstance *canard;
    struct uavcan_protocol_NodeStatus *nodeStatus;
} PinecanInit;

/**
 * @brief  Initialize PineCAN.
 * @param  initParams pointer to PinecanInit structure with initialization parameters.
 * @retval None
 */
PineCAN_Status pinecanInit(PinecanInit *initParams);
