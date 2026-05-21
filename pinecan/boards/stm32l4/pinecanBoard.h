#pragma once

#include "stm32l4xx_hal.h"
#include "canard.h"
#define RX_QUEUE_SIZE 32

typedef struct {
    CAN_HandleTypeDef *hcan;
    CanardInstance *canard;
    void* canardMemPool;
    size_t canardMemPoolSize;
    struct uavcan_protocol_NodeStatus *nodeStatus;
} PinecanInit;

/**
 * @brief  Initialize PineCAN.
 * @param  initParams pointer to PinecanInit structure with initialization parameters.
 * @retval None
 */
void pinecanInit(PinecanInit *initParams);
