#include "canard.h"
#include "pinecanBoard_internals.h"
#include "uavcan.protocol.dynamic_node_id.Allocation.h"


void dnIdAllocateeInit(CanardInstance *ins);
void dnIdAllocatee1ms(void);
void handleDnIDAllocationRes(CanardInstance* ins, CanardRxTransfer* transfer);
uint8_t dnIdAllocateeGetAssignedNodeID(void);





