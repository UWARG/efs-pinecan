/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "can.h"

/* USER CODE BEGIN 0 */
#include <stdbool.h>
#include <stdint.h>
#include "pinecan.h"
#include "dronecan_msgs.h"
#include "ardupilot.indication.NotifyState.h"

// can.c is the sole owner of PineCAN state; see Lighting/Lighting.md (PineCAN).
static CanardInstance canard;
static struct uavcan_protocol_NodeStatus node_status;
static bool pinecan_initialized = false;

// Async cache: written in IRQ (Handle_Notify_State), read in main loop; IRQ-bracketed (non-atomic 64-bit).
static volatile uint64_t latest_vehicle_state = 0;

/* USER CODE END 0 */

CAN_HandleTypeDef hcan1;

/* CAN1 init function */
void Mx_Can1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 12;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* can_handle)
{

  GPIO_InitTypeDef gpio_init_struct = {0};
  if(can_handle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    gpio_init_struct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* can_handle)
{

  if(can_handle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN1 GPIO Configuration
    PA11     ------> CAN1_RX
    PA12     ------> CAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

// Sets NodeStatus defaults and calls pinecanInit once; failure leaves pinecan_initialized false.
PineCAN_Status Init_Can(void)
{
    node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status.mode   = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status.sub_mode = 0;
    node_status.vendor_specific_status_code = 0;

    PinecanInit init_params = {
        .hcan       = &hcan1,
        .canard     = &canard,
        .nodeStatus = &node_status
    };

    PineCAN_Status result = pinecanInit(&init_params);
    pinecan_initialized = (result == PINECAN_OK);
    return result;
}

// Gates pinecan1ms() to once per millisecond; no-op until Init_Can() succeeds.
void Can_Service(void)
{
    static uint32_t last_tick = 0;

    if (!pinecan_initialized) {
        return;
    }

    uint32_t now = HAL_GetTick();
    if (now != last_tick) {
        last_tick = now;
        pinecan1ms();
    }
}

// RX handler (IRQ context): decode NotifyState and cache vehicle_state; discard on decode failure.
void Handle_Notify_State(CanardInstance *ins, CanardRxTransfer *transfer)
{
    (void)ins;

    struct ardupilot_indication_NotifyState decoded = {0};

    const int decode_result = (ardupilot_indication_NotifyState_decode(transfer, &decoded) != 0);

    if (decode_result) {
        return;
    }

    __disable_irq();
    latest_vehicle_state = decoded.vehicle_state;
    __enable_irq();
}

// Getter polled by the main loop; returns 0 before any message. No mapping.
uint64_t Can_Get_Latest_Vehicle_State(void)
{
    uint64_t v;
    __disable_irq();
    v = latest_vehicle_state;
    __enable_irq();
    return v;
}

/* USER CODE END 1 */
