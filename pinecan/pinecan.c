#include "pinecan.h"
#include "pinecan_internals.h"
#include "pinecanBoard.h"
#include "pinecanBoard_internals.h"
#include "dronecan_msgs.h"
#include "pinecan_handlers.h"
#include <string.h>

#define RX_HANDLER_LIST_PRIVATE RX_HANDLER_LIST \
        REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_GETNODEINFO,           handleGetNodeInfo,           REQUEST) \

/* ============ PRIVATE DATA ============ */

typedef struct {
    CanardInstance *canard;
    struct uavcan_protocol_NodeStatus *nodeStatus;
    uint32_t nextRunTime1Hz;
} PinecanData;

static PinecanData data;

/* ============ PRIVATE FUNCTION DECLARATIONS ============ */

static bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* crcSignature, uint16_t dataTypeID, CanardTransferType transferType, uint8_t sourceNodeID);
static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer);
static void handleGetNodeInfo(CanardInstance* ins, CanardRxTransfer *transfer);
static void sendNodeStatus(void);
static void processCanardTxQueue(void);

/* ============ PRIVATE FUNCTION DEFINITIONS ============ */

static bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* crcSignature, uint16_t dataTypeID, CanardTransferType transferType, uint8_t sourceNodeID) {
    UNUSED(ins);
    UNUSED(sourceNodeID);

    // --------- RESPONSE ----------
    if (transferType == CanardTransferTypeResponse)
    {
        switch (dataTypeID)
        {
            // For RESPONSE, we might have signature TYPE_RESPONSE_SIGNATURE (if you use it)
            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_RESPONSE_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE)   /* nothing */
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_RESPONSE
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_BROADCAST

            default:
                return false;
        }
    }

    // --------- REQUEST ----------
    if (transferType == CanardTransferTypeRequest)
    {
        switch (dataTypeID)
        {
            // For REQUEST, we use TYPE_REQUEST_SIGNATURE
            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_REQUEST_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE)  /* nothing */
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_RESPONSE
            #undef DISPATCH_ACCEPT_CASE_BROADCAST

            default:
                return false;
        }
    }

    // --------- BROADCAST ----------
    if (transferType == CanardTransferTypeBroadcast)
    {
        switch (dataTypeID)
        {
            // For BROADCAST we usually just have TYPE_SIGNATURE
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE)  /* nothing */
            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_BROADCAST
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_RESPONSE

            default:
                return false;
        }
    }

    return false;
}

static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
    // --------- RESPONSE ----------
    if (transfer->transfer_type == CanardTransferTypeResponse)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(ins, transfer); \
                    return;

            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_RESPONSE
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_BROADCAST

            default:
                return;
        }
    }

    // --------- REQUEST ----------
    if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(ins, transfer); \
                    return;

            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_RESPONSE
            #undef DISPATCH_HANDLER_BROADCAST

            default:
                return;
        }
    }

    // --------- BROADCAST ----------
    if (transfer->transfer_type == CanardTransferTypeBroadcast)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(ins, transfer); \
                    return;

            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST_PRIVATE

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_BROADCAST
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_RESPONSE

            default:
                return;
        }
    }
}

// === rx dronecan ===
// uavcan.protocol.getnodeinfo
static void handleGetNodeInfo(CanardInstance* ins, CanardRxTransfer *transfer) // TODO: really no reason that nodeInfoRes can't be static and not regenerated everytime
{
    UNUSED(ins);

    data.nodeStatus->uptime_sec = getUptimeMs() / 1000U;

    struct uavcan_protocol_GetNodeInfoResponse nodeInfoRes = {0};
    nodeInfoRes.status                                            = *(data.nodeStatus);
    nodeInfoRes.software_version.major                            = SOFTWARE_MAJOR_VERSION;
    nodeInfoRes.software_version.minor                            = SOFTWARE_MINOR_VERSION;
    nodeInfoRes.software_version.optional_field_flags             = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT;
    nodeInfoRes.software_version.vcs_commit                       = COMMIT_HASH;
    nodeInfoRes.hardware_version.major                            = HARDWARE_MAJOR_VERSION;
    nodeInfoRes.hardware_version.minor                            = HARDWARE_MINOR_VERSION;
    nodeInfoRes.hardware_version.certificate_of_authenticity.len  = 0;
    nodeInfoRes.name.len                                          = sizeof(NODE_NAME);
    memcpy(nodeInfoRes.hardware_version.unique_id, getUniqueHardwareID(), 16);
    strcpy((char*)nodeInfoRes.name.data, NODE_NAME);

    uint8_t txBuffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE] = {0};
    const uint32_t dataLength = uavcan_protocol_GetNodeInfoResponse_encode(&nodeInfoRes, txBuffer);

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
    canardRequestOrRespondObj(data.canard, transfer->source_node_id, &txFrame);
}

// uavcan.protocol.nodestatus
static void sendNodeStatus(void)
{
    data.nodeStatus->uptime_sec = getUptimeMs() / 1000U;

    uint8_t txBuffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE] = {0};
    const uint32_t dataLength = uavcan_protocol_NodeStatus_encode(data.nodeStatus, txBuffer);

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
    canardBroadcastObj(data.canard, &txFrame);
}

static void processCanardTxQueue(void) {
    while (1) {
        const CanardCANFrame *txFrame = canardPeekTxQueue(data.canard);
        if (txFrame == NULL) {
            break;
        }

        if (txFrame->id & CANARD_CAN_FRAME_ERR) {
            // error frame, pop and continue
            canardPopTxQueue(data.canard);
        } else {
            if (processTxMessage(txFrame)) {
                // successfully transmitted, pop and continue
                canardPopTxQueue(data.canard);
            } else {
                // failed to transmit, exit and try transmitting later
                break;
            }
        }
    }
}

/* ============ INTERNAL PUBLIC FUNCTION DEFINITIONS ============ */

void init(CommonInitParams *initParams) {
    data.canard = initParams->canard;
    data.nodeStatus = initParams->nodeStatus;
    data.nextRunTime1Hz = 0U;

    // configure Canard
    canardInit(initParams->canard,
        initParams->canardMemPool,
        initParams->canardMemPoolSize,
        onTransferReceived,
        shouldAcceptTransfer,
        NULL
    );

    canardSetLocalNodeID(initParams->canard, NODE_ID);
}

void handleRxFrame(CanardCANFrame *rxFrame) {
    // TODO: this rx frame is deleted after this function returns. If/when adding a queue, make sure to handle this correctly
    canardHandleRxFrame(data.canard, rxFrame, getUptimeMs() * 1000U);
    //make vrb of canardcanframe in pinecanboard.c make it equal to the frame im going to pop(make a peek), then pass into handlerxframe
    
    dequeueRxQueue(rxFrame); //how else can i delete it...
}

/* ============ EXTERNAL PUBLIC FUNCTION DEFINITIONS ============ */

void pinecan1ms(void){
    processCanardTxQueue();
    processCanardRxQueue();
    if(getUptimeMs() >= data.nextRunTime1Hz)
    {
        data.nextRunTime1Hz = getUptimeMs() + 1000U;

        canardCleanupStaleTransfers(data.canard, getUptimeMs() * 1000U);
        sendNodeStatus();
    }
}
