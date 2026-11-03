// Compile-time RX handler registration for PineCAN; see Lighting/Lighting.md (PineCAN).
#pragma once
#include "canard.h"

#define RX_HANDLER_LIST \
    REGISTER_RX_HANDLER(ARDUPILOT_INDICATION_NOTIFYSTATE, Handle_Notify_State, BROADCAST)

#ifdef __cplusplus
extern "C" {
#endif

// Defined in Core/Src/can.c.
void Handle_Notify_State(CanardInstance *ins, CanardRxTransfer *transfer);

#ifdef __cplusplus
}
#endif
