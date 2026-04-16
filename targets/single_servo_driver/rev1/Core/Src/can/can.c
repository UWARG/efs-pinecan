#include <string.h>
#include <math.h>
#include "canard.h"
#include "dronecan_msgs.h"
#include "servo.h"
#include "stm32l4xx_hal.h"
#include "pinecan.h"

extern CAN_HandleTypeDef hcan1;

struct uavcan_protocol_NodeStatus nodeStatus;

static CanardInstance canard;

// === tx dronecan ===
// uavcan.equipment.actuator.status
void sendActuatorStatus(Servo_t servo)
{
  // This is kind of garbage data right now, but including for proof of concept of tx
  struct uavcan_equipment_actuator_Status status = {0};
  status.actuator_id = servo.actuatorID;
  status.position = NAN; // Unknown fields should be set to NAN.
  status.force = NAN;
  status.speed = NAN;
  status.power_rating_pct = UAVCAN_EQUIPMENT_ACTUATOR_STATUS_POWER_RATING_PCT_UNKNOWN;

  uint8_t txBuffer[UAVCAN_EQUIPMENT_ACTUATOR_STATUS_MAX_SIZE] = {0};
  const uint32_t dataLength = uavcan_equipment_actuator_Status_encode(&status, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
      CanardTransferTypeBroadcast,
      UAVCAN_EQUIPMENT_ACTUATOR_STATUS_SIGNATURE,
      UAVCAN_EQUIPMENT_ACTUATOR_STATUS_ID,
      &transferID,
      CANARD_TRANSFER_PRIORITY_LOW,
      txBuffer,
      dataLength
  };
  
  canardBroadcastObj(&canard, &txFrame);
}

// uavcan.equipment.power.CircuitStatus
// TODO: QOL feature

// === rx dronecan ===
// uavcan.equipment.actuator.arraycommand
void handleArrayCommand(CanardInstance* ins, CanardRxTransfer *transfer)
{
  UNUSED(ins);

  struct uavcan_equipment_actuator_ArrayCommand cmdArray = {0};
  if(uavcan_equipment_actuator_ArrayCommand_decode(transfer, &cmdArray))
  {
    return;
  }

  for(uint8_t i = 0; i < cmdArray.commands.len; ++i)
  {
    struct uavcan_equipment_actuator_Command cmd = cmdArray.commands.data[i];

    switch(cmd.command_type)
    {
      case UAVCAN_EQUIPMENT_ACTUATOR_COMMAND_COMMAND_TYPE_UNITLESS:
        percentageActuation(cmd.command_value, cmd.actuator_id);
        break;
      case UAVCAN_EQUIPMENT_ACTUATOR_COMMAND_COMMAND_TYPE_PWM:
        dutyCycleActuation(cmd.command_value, cmd.actuator_id);
        break;
    }
  }
}

void initCAN(void)
{
  PinecanInit initParams = {
    .hcan = &hcan1,
    .canard = &canard,
    .nodeStatus = &nodeStatus
  };
  pinecanInit(&initParams);
}
