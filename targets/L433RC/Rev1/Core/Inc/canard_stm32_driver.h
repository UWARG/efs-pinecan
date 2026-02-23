#pragma once
#include "canard.h"


/**
  * @brief  Send all CAN Tx messages queued in canard.
  * @retval None
  */
void sendCANTx(void);

/**
  * @brief  Execute periodic CAN maintenance tasks.
  * @retval None
  */
void periodicCANTasks(void);

void sendFirmwareRead(void);


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
