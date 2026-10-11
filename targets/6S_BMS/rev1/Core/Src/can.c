#include "canard.h"
#include "dronecan_msgs.h"
#include "pinecan.h"

extern CAN_HandleTypeDef hcan1;
struct uavcan_protocol_NodeStatus nodeStatus;
static CanardInstance canard;

// === tx dronecan ===
// ardupilot.equipment.power.BatteryCells
void sendCellVoltages(uint16_t *cell_voltages)
{
  struct ardupilot_equipment_power_BatteryCells batteryCells = {0};
  for(int i = 0; i < 6; i++) {
    batteryCells.voltages.data[i] = cell_voltages[i];
  }
  batteryCells.voltages.len = 6; 

  uint8_t txBuffer[ARDUPILOT_EQUIPMENT_POWER_BATTERYCELLS_MAX_SIZE] = {0};
  const uint32_t dataLength = ardupilot_equipment_power_BatteryCells_encode(&batteryCells, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
      CanardTransferTypeBroadcast,
      ARDUPILOT_EQUIPMENT_POWER_BATTERYCELLS_SIGNATURE,
      ARDUPILOT_EQUIPMENT_POWER_BATTERYCELLS_ID,
      &transferID,
      CANARD_TRANSFER_PRIORITY_LOW,
      txBuffer,
      dataLength
  };
  
  canardBroadcastObj(&canard, &txFrame);
}

// === rx dronecan ===
// 
void handleArrayCommand(CanardInstance* ins, CanardRxTransfer *transfer)
{ 
    // To be implemented later
/** From Single Servo Driver
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
  } **/
}

void CAN_init() {
    static PinecanInit pinecanInitParams = {
        .hcan = &hcan1,
        .canard = &canard,
        .nodeStatus = &nodeStatus,
    };
    pinecanInit(&pinecanInitParams);
}

