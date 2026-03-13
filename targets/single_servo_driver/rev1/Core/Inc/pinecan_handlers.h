#pragma once

#include "canard.h"
#include "dynamic_node_allocatee.h"

// REGISTER_RX_HANDLER(TYPE, HANDLER, TRANSFER_KIND)
// TYPE is the base name of the DSDL type:
//   expects TYPE_ID, TYPE_REQUEST_SIGNATURE, TYPE_RESPONSE_SIGNATURE, or TYPE_SIGNATURE to exist
// HANDLER is the function that handles the received transfer
// TRANSFER_KIND is one of: REQUEST, RESPONSE, BROADCAST
#define RX_HANDLER_LIST \
    REGISTER_RX_HANDLER(UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND, handleArrayCommand, BROADCAST) \
    REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION, handleDnIDAllocationRes, BROADCAST)

void handleArrayCommand(CanardInstance* ins, CanardRxTransfer *transfer);
