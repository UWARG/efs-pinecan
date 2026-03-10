#pragma once

// ------------------------------------------------------------
// Dummy Pinecan handler list for bootloader target
// ------------------------------------------------------------
// This allows pinecan.c to compile without defining
// any application-level RX handlers.
//
// Bootloader-specific message handling (if any)
// should be implemented directly in onTransferReceived()
// or via a proper handler list later.
// ------------------------------------------------------------
#include "can.h"
// No application RX handlers
#define RX_HANDLER_LIST \
    REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_FILE_BEGINFIRMWAREUPDATE, handleBeginFirmwareUpdate, REQUEST) \
    REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_FILE_READ, handleFileReadResponse, RESPONSE)


