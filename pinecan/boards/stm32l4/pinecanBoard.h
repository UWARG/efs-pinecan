#pragma once

#include "stm32l4xx_hal.h"
#include "canard.h"
#ifndef RX_QUEUE_SIZE 
#define RX_QUEUE_SIZE 32
#endif

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
bool enqueueRxQueue(const CanardCANFrame *frame);
bool dequeueRxQueue(CanardCANFrame *frame);
void processCanardRxQueue();
void handleRxFrame(CanardCANFrame *rxFrame);
CanardCANFrame* peekRxQueue();
