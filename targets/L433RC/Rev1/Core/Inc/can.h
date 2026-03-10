/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "canard.h"
#include <stdbool.h>

/* USER CODE BEGIN Includes */
/**
  * @brief  Initialize HAL CAN and canard.
  * @retval None
  */
void initCAN(void);

/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan1;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_CAN1_Init(void);

/* USER CODE BEGIN Prototypes */

// FW update struct
typedef struct {
    char path[256];
    uint8_t node_id;
    uint8_t transfer_id;
    uint32_t last_read_ms;
    uint32_t offset;            // Byte offset in firmware file
	uint32_t flash_address;     // Current staging flash write address
	bool in_progress;           // Is update running?
} FirmwareUpdate;

extern FirmwareUpdate fwupdate;

void sendFirmwareRead(void);
void handleFileReadResponse(CanardInstance* ins, CanardRxTransfer* transfer);
void handleBeginFirmwareUpdate(CanardInstance* ins, CanardRxTransfer *transfer);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */

