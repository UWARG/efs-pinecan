#include <string.h>
#include "canard.h"
#include "dronecan_msgs.h"
#include "stm32l4xx_hal.h"
#include "canard_stm32_driver.h"

#define CAN_NODE_NAME          "6SSERVO"
#define CAN_NODE_ID            69U
#define COMMIT_HASH            0U
#define SOFTWARE_MAJOR_VERSION 1U
#define SOFTWARE_MINOR_VERSION 0U
#define HARDWARE_MAJOR_VERSION 2U
#define HARDWARE_MINOR_VERSION 0U

extern CAN_HandleTypeDef hcan1;

struct uavcan_protocol_NodeStatus nodeStatus;

static CanardInstance canard;
static uint8_t canardMemPool[1024];
static uint8_t hardwareID[16];


// === tx dronecan ===
// uavcan.protocol.nodestatus
static void sendNodeStatus(void)
{
  nodeStatus.uptime_sec = HAL_GetTick() / 1000U;

  uint8_t txBuffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE] = {0};
  uint32_t dataLength = uavcan_protocol_NodeStatus_encode(&nodeStatus, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
    CanardTransferTypeBroadcast,
    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
    UAVCAN_PROTOCOL_NODESTATUS_ID,
    &transferID,
    CANARD_TRANSFER_PRIORITY_LOW,
    txBuffer,
    dataLength
  };
  canardBroadcastObj(&canard, &txFrame);
}

static void handleFileReadResponse(CanardInstance* ins, CanardRxTransfer* transfer){ // This isn't being called
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

// uavcan.equipment.actuator.status
// TODO: QOL feature

// uavcan.equipment.power.CircuitStatus
// TODO: QOL feature

// === rx dronecan ===
// uavcan.protocol.getnodeinfo


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

static void handleBeginFirmwareUpdate(CanardInstance* ins, CanardRxTransfer *transfer)
{
	nodeStatus.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_SOFTWARE_UPDATE;
	//deconde message to get file path
	struct uavcan_protocol_file_BeginFirmwareUpdateRequest req;
	uavcan_protocol_file_BeginFirmwareUpdateRequest_decode((const)transfer, &req);

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



static void handleGetNodeInfo(CanardRxTransfer *transfer)
{
  nodeStatus.uptime_sec = HAL_GetTick() / 1000U;

  struct uavcan_protocol_GetNodeInfoResponse nodeInfoRes = {0};
  nodeInfoRes.status                                            = nodeStatus;
  nodeInfoRes.software_version.major                            = HARDWARE_MAJOR_VERSION;
  nodeInfoRes.software_version.minor                            = HARDWARE_MINOR_VERSION;
  nodeInfoRes.software_version.optional_field_flags             = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT;
  nodeInfoRes.software_version.vcs_commit                       = COMMIT_HASH;
  nodeInfoRes.hardware_version.major                            = HARDWARE_MAJOR_VERSION;
  nodeInfoRes.hardware_version.minor                            = HARDWARE_MINOR_VERSION;
  nodeInfoRes.hardware_version.certificate_of_authenticity.len  = 0;
  nodeInfoRes.name.len                                          = sizeof(CAN_NODE_NAME);
  memcpy(nodeInfoRes.hardware_version.unique_id, hardwareID, 16);
  strcpy((char*)nodeInfoRes.name.data, CAN_NODE_NAME);

  uint8_t txBuffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE] = {0};
  uint32_t dataLength = uavcan_protocol_GetNodeInfoResponse_encode(&nodeInfoRes, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
    CanardTransferTypeResponse,
    UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_SIGNATURE,
    UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_ID,
    &transferID,
    CANARD_TRANSFER_PRIORITY_LOW,
    txBuffer,
    dataLength
  };
  canardRequestOrRespondObj(&canard, transfer->source_node_id, &txFrame);
}

// uavcan.equipment.actuator.arraycommand
static void handleArrayCommand(CanardRxTransfer *transfer)
{
}

bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* crcSignature, uint16_t dataTypeID, CanardTransferType transferType, uint8_t srcNodeID)
{
  if(transferType == CanardTransferTypeResponse)
  {
    switch(dataTypeID)
    {
    case UAVCAN_PROTOCOL_FILE_READ_ID:
			*crcSignature = UAVCAN_PROTOCOL_FILE_READ_SIGNATURE;
			return true;   // accept File.Read responses
      default:
        return false;
    }
  }

  if(transferType == CanardTransferTypeRequest)
  {
    switch(dataTypeID)
    {
      case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        *crcSignature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
        return true;
      case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
		  *crcSignature = UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_SIGNATURE;
		  return true;
      default:
        return false;
    }
  }

  if(transferType == CanardTransferTypeBroadcast)
  {
    switch(dataTypeID)
    {
      case ARDUPILOT_INDICATION_NOTIFYSTATE_ID:
        *crcSignature = ARDUPILOT_INDICATION_NOTIFYSTATE_SIGNATURE;
        return true;
      case UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND_ID:
        *crcSignature = UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND_SIGNATURE;
        return true;
      case UAVCAN_PROTOCOL_NODESTATUS_ID:
        *crcSignature = UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE;
        return true;
      default:
        return false;
    }
  }

  return false;
}

void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
  if(transfer->transfer_type == CanardTransferTypeResponse)
  {
    switch(transfer->data_type_id)
    {
    case UAVCAN_PROTOCOL_FILE_READ_ID:
    	handleFileReadResponse(ins, transfer);
    	break;
      default:
        return;
    }
  }

  if(transfer->transfer_type == CanardTransferTypeRequest)
  {
    switch(transfer->data_type_id)
    {
      case UAVCAN_PROTOCOL_GETNODEINFO_ID:
        handleGetNodeInfo(transfer);
        return;
      case UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE_ID:
    	  handleBeginFirmwareUpdate(ins, transfer); //Maybe check to see if already updating
    	  return;
//      case UAVCAN_PROTOCOL_FILE_READ_ID:
//          	  handleFileReadResponse(ins, transfer);
      default:
        return;
    }
  }

  if(transfer->transfer_type == CanardTransferTypeBroadcast)
  {
    switch(transfer->data_type_id)
    {
      case UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND_ID:
        handleArrayCommand(transfer);
        return;
      case UAVCAN_PROTOCOL_FILE_READ_ID:
          	handleFileReadResponse(ins, transfer);
          	break;
      default:
        return;
    }
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  // receive HAL CAN packet
  CAN_RxHeaderTypeDef rxHeader = {0};
  uint8_t rxData[8] = {0};

  if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
  {
    if(rxHeader.IDE != CAN_ID_EXT || rxHeader.RTR != CAN_RTR_DATA)
    {
      return;
    }
  }

  // create canard packet
  CanardCANFrame rxFrame = {0};

  rxFrame.id = rxHeader.ExtId | CANARD_CAN_FRAME_EFF;
  rxFrame.data_len = rxHeader.DLC ;
  rxFrame.iface_id = 0;
  memcpy(rxFrame.data, rxData, rxHeader.DLC);

  canardHandleRxFrame(&canard, &rxFrame, HAL_GetTick() * 1000U);
}

void initCAN(void)
{
  // configure HAL CAN
  CAN_FilterTypeDef canfil;
  canfil.FilterBank = 0;
  canfil.FilterMode = CAN_FILTERMODE_IDMASK;
  canfil.FilterFIFOAssignment = CAN_RX_FIFO0;
  canfil.FilterIdHigh = 0;
  canfil.FilterIdLow = 0;
  canfil.FilterMaskIdHigh = 0;
  canfil.FilterMaskIdLow = 0;
  canfil.FilterScale = CAN_FILTERSCALE_32BIT;
  canfil.FilterActivation = ENABLE;
  canfil.SlaveStartFilterBank = 0;
  HAL_CAN_ConfigFilter(&hcan1, &canfil);

  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  // configure Canard
  canardInit(&canard,
    canardMemPool,
    sizeof(canardMemPool),
    onTransferReceived,
    shouldAcceptTransfer,
    NULL
  );

  canardSetLocalNodeID(&canard, CAN_NODE_ID);

  // set hardware id
  uint32_t tmpHID[] = {HAL_GetUIDw2(), HAL_GetUIDw1(), HAL_GetUIDw0()};
  memcpy(hardwareID + 4, tmpHID, 12);

  // start HAL driver
  HAL_CAN_Start(&hcan1);
}

void sendCANTx(void)
{
  while(1)
  {
    CanardCANFrame* frame = canardPeekTxQueue(&canard);
    if(frame == NULL)
    {
      return;
    }

    if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0)
    {
      CAN_TxHeaderTypeDef txHeader = {0};
      txHeader.ExtId = frame->id & 0x1FFFFFFF;
      txHeader.IDE = CAN_ID_EXT;
      txHeader.RTR = CAN_RTR_DATA;
      txHeader.DLC = frame->data_len;

      uint32_t txMailboxUsed = 0;
      if(HAL_CAN_AddTxMessage(&hcan1, &txHeader, frame->data, &txMailboxUsed) == HAL_OK)
      {
        canardPopTxQueue(&canard);
      }
    }
    else
    {
      return;
    }
  }
}

void periodicCANTasks(void)
{
  static uint32_t nextRunTime = 0;

  if(HAL_GetTick() >= nextRunTime)
  {
    nextRunTime += 1000U;

    canardCleanupStaleTransfers(&canard, HAL_GetTick() * 1000U);
    sendNodeStatus();
  }
}
