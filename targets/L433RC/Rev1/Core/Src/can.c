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
#include <string.h>
#include "canard.h"
#include "dronecan_msgs.h"
#include "stm32l4xx_hal.h"
#include "pinecan.h"

/* USER CODE BEGIN 0 */

CAN_HandleTypeDef hcan1;

struct uavcan_protocol_NodeStatus nodeStatus;

static CanardInstance canard;
static uint8_t canardMemPool[1024];
static uint8_t hardwareID[16];

/* USER CODE END 0 */

/* CAN1 init function */
void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 20;
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

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
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
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
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

void initCAN(void)
{
  PinecanInit initParams = {
    .hcan = &hcan1,
    .canard = &canard,
    .canardMemPool = canardMemPool,
    .canardMemPoolSize = sizeof(canardMemPool),
    .nodeStatus = &nodeStatus
  };
  pinecanInit(&initParams);
}

void handleFileReadResponse(CanardInstance* ins, CanardRxTransfer* transfer){
	if ((transfer->transfer_id+1)%32 != fwupdate.transfer_id ||
		transfer->source_node_id != fwupdate.node_id) {
		/* not for us */
		return;
	}

	struct uavcan_protocol_file_ReadResponse pkt;
	if (uavcan_protocol_file_ReadResponse_decode(transfer, &pkt)) {
		/* bad packet */
		return;
	}

	if (pkt.error.value != UAVCAN_PROTOCOL_FILE_ERROR_OK) {
		/* read failed */
		fwupdate.in_progress = false;
		fwupdate.node_id = 0;
		return;
	}

	if (pkt.data.len == 0) {
	    // No more data – update complete --> need to add flags for this to stop the update now
	    fwupdate.in_progress = false;
	    fwupdate.node_id = 0;
	    return;
	}

	HAL_FLASH_Unlock();

	uint32_t bytes_written = 0;
	while(bytes_written < pkt.data.len){
		uint8_t chunk[8] = {0xFF}; //Default erased value
		uint32_t remain = pkt.data.len - bytes_written;
		uint32_t to_copy = (remain >= 8U) ? 8U : remain;

		memcpy(chunk, &pkt.data.data[bytes_written], to_copy);

		uint64_t double_word;

		memcpy(&double_word, chunk, 8); // Makes a 64 bit object for the HAL_FLASH

		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, fwupdate.flash_address, double_word) != HAL_OK){
			HAL_FLASH_Lock();
			fwupdate.in_progress = false;
			fwupdate.node_id = 0;
			return;
		}
		//Double word means it writes 64 bits at a time (a word is 32 bits)
		//Might have to change around the values
		fwupdate.flash_address += 8U; //8 bytes long = 64 bits
		bytes_written += to_copy;
	}
	HAL_FLASH_Lock();

	fwupdate.offset += pkt.data.len;
	fwupdate.last_read_ms = 0; // makes it so fwupdate check in sendFirmwareRead will go through right away
}

void eraseStagingFlash(void){

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase = {0};
	uint32_t page_error = 0;
	// This assumes a 32kb bootloader size. (From 0x08000000 - 0x08008000)
	uint32_t page_num = (0x08020000-0x08000000)/0x800; //0x800 is 2kb which is 1 page length according to data sheet
	// Line above shows starting page for memory wipe
	fwupdate.flash_address = 0x08020000; // Need to change this maybe to make sure that bootloader is not affected
	erase.Page = page_num;
	erase.NbPages = 16; //Assuming 32kb for staging slot
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	//erase.Banks     = FLASH_BANK_1; // Always specify on STM32L4/F4
	if(HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK){
		//TODO: handle error
	}

	HAL_FLASH_Lock();

}

void handleBeginFirmwareUpdate(CanardInstance* ins, CanardRxTransfer *transfer)
{
	nodeStatus.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE;
	//deconde message to get file path
	struct uavcan_protocol_file_BeginFirmwareUpdateRequest req;
	uavcan_protocol_file_BeginFirmwareUpdateRequest_decode(transfer, &req);

	uint8_t source_node_id = 		transfer->source_node_id;
	uint8_t *file_path_bytes = 		req.image_file_remote_path.path.data;
	uint8_t file_len = 				req.image_file_remote_path.path.len;

	fwupdate.offset = 0;
	fwupdate.node_id = source_node_id;
	strncpy(fwupdate.path, (char*)file_path_bytes, file_len);

	uint8_t buffer[UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_MAX_SIZE];
	struct uavcan_protocol_file_BeginFirmwareUpdateResponse reply;
	memset(&reply, 0, sizeof(reply));
	reply.error = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_RESPONSE_ERROR_OK;

	uint32_t total_size = uavcan_protocol_file_BeginFirmwareUpdateResponse_encode(&reply, buffer);

	canardRequestOrRespond(ins, // Need to pass in CanardInstance ins ex --> static void handle_param_ExecuteOpcode(CanardInstance* ins, CanardRxTransfer* transfer)
						   transfer->source_node_id,
						   UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_SIGNATURE,
						   UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID,
						   &transfer->transfer_id,
						   transfer->priority,
						   CanardResponse,
						   &buffer[0],
						   total_size);

	//erase the flash staging area
	eraseStagingFlash();

//	return true;
}


void sendFirmwareRead(void){
	uint32_t now = HAL_GetTick();
	if (now - fwupdate.last_read_ms < 750) {
		// the server may still be responding
		return;
	}
	fwupdate.last_read_ms = now;

	uint8_t buffer[UAVCAN_PROTOCOL_FILE_READ_REQUEST_MAX_SIZE];

	struct uavcan_protocol_file_ReadRequest pkt;
	memset(&pkt, 0, sizeof(pkt));

	pkt.path.path.len = strlen((const char *)fwupdate.path);
	pkt.offset = fwupdate.offset;
	memcpy(pkt.path.path.data, fwupdate.path, pkt.path.path.len);

	uint16_t total_size = uavcan_protocol_file_ReadRequest_encode(&pkt, buffer);

	canardRequestOrRespond(&canard,
			   fwupdate.node_id,
						   UAVCAN_PROTOCOL_FILE_READ_SIGNATURE,
						   UAVCAN_PROTOCOL_FILE_READ_ID,
			   &fwupdate.transfer_id,
						   CANARD_TRANSFER_PRIORITY_HIGH,
						   CanardRequest,
						   &buffer[0],
						   total_size);
}


/* USER CODE END 1 */
