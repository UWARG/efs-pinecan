#include <string.h>
#include <stdio.h>
#include "pinecanBoard.h"
#include "pinecan_internals.h"

/* ============ PRIVATE DATA ============ */
typedef struct {
    CAN_HandleTypeDef *hcan;
    uint8_t hardwareID[16];
} PinecanBoardData;

//check if this is the best way to define, also check warg best practices

static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
static volatile uint8_t count = 0;
static CanardCANFrame rxQueue[RX_QUEUE_SIZE];

static PinecanBoardData boardData;

/* ============ PRIVATE FUNCTION DECLARATIONS ============ */

static void setUniqueHardwareID(uint8_t id[16]);

/* ============ PRIVATE FUNCTION DEFINITIONS ============ */

// get a 16 byte unique ID for this node, this should be based on the CPU unique ID or other unique ID
static void setUniqueHardwareID(uint8_t id[16]){
    uint32_t HALUniqueIDs[4];
    // Create Unique ID out of the 96-bit STM32 UID
    HALUniqueIDs[0] = HAL_GetUIDw0();
    HALUniqueIDs[1] = HAL_GetUIDw1();
    HALUniqueIDs[2] = HAL_GetUIDw2();
    HALUniqueIDs[3] = HAL_GetUIDw1(); // repeating UIDw1 for remaining bytes
    memcpy(id, HALUniqueIDs, 16);
}

/* ============ INTERNAL PUBLIC FUNCTION DEFINITIONS ============ */

uint8_t *getUniqueHardwareID(void) {
    return boardData.hardwareID;
}

uint32_t getUptimeMs(void) {
    return HAL_GetTick();
}

bool processTxMessage(const CanardCANFrame *txFrame) {
    if (HAL_CAN_GetTxMailboxesFreeLevel(boardData.hcan) > 0) {
        CAN_TxHeaderTypeDef txHeader = {0};

        if (txFrame->id & CANARD_CAN_FRAME_EFF) {
            txHeader.IDE = CAN_ID_EXT;
            txHeader.ExtId = txFrame->id & CANARD_CAN_EXT_ID_MASK;
        } else {
            txHeader.IDE = CAN_ID_STD;
            txHeader.StdId = txFrame->id & CANARD_CAN_STD_ID_MASK;
        }

        txHeader.DLC = txFrame->data_len;

        if (txFrame->id & CANARD_CAN_FRAME_RTR) {
            txHeader.RTR = CAN_RTR_REMOTE;
        } else {
            txHeader.RTR = CAN_RTR_DATA;
        }

        txHeader.TransmitGlobalTime = DISABLE;

        uint32_t txMailboxUsed = 0;
        if(HAL_CAN_AddTxMessage(boardData.hcan, &txHeader, txFrame->data, &txMailboxUsed) == HAL_OK)
        {
            return true;
        } else {
            // Failed to add message
            return false;
        }
    } else {
        // No free mailbox
        return false;
    }
}

/* ============ EXTERNAL PUBLIC FUNCTION DEFINITIONS ============ */

// TODO: compare to const int16_t rx_res = canardSTM32Recieve(hcan, CAN_RX_FIFO0, &rx_frame); again
// Can think abt support for multiple rx fifos (this would come with some smart stuff with setting up the filters correctly for that as well)
// Also even though we can use our own rx fifo queue here, we can also just use the fact that stm32 already has an rx fifo of length 3 (and its in hardware hooray)

//create another queue (holds more than 3 frames) which takes frames from RXFIFO0, holds frames until they can be processed
//figure out how to make queue global
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
//end goal: create frame, put in buffer and return
{
    CAN_RxHeaderTypeDef rxHeader = {0};
    uint8_t rxData[8] = {0};
    CanardCANFrame rxFrame = {0};

    // receive HAL CAN packet
    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
    {
        // Process ID to canard format
        rxFrame.id = rxHeader.ExtId;

        if (rxHeader.IDE == CAN_ID_EXT) { // canard will only process the message if it is extended ID
            rxFrame.id |= CANARD_CAN_FRAME_EFF;
        }

        if (rxHeader.RTR == CAN_RTR_REMOTE) { // canard won't process the message if it is a remote frame
            rxFrame.id |= CANARD_CAN_FRAME_RTR;
        }

        rxFrame.data_len = rxHeader.DLC;
        memcpy(rxFrame.data, rxData, rxHeader.DLC);

        // assume a single interface
        rxFrame.iface_id = 0;

        
        //Delete handleRxFrame, rxFrame is the frame from fifo0
        enqueueRxQueue(&rxFrame);
    }
}

void pinecanInit(PinecanInit *initParams) {
    boardData.hcan = initParams->hcan;

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
    HAL_CAN_ConfigFilter(initParams->hcan, &canfil);

    HAL_CAN_ActivateNotification(initParams->hcan, CAN_IT_RX_FIFO0_MSG_PENDING);

    // start HAL driver
    HAL_CAN_Start(initParams->hcan);

    // set hardware id
    setUniqueHardwareID(boardData.hardwareID);

    // initialize non board specific
    CommonInitParams commonInitParams = {
        .canard = initParams->canard,
        .canardMemPool = initParams->canardMemPool,
        .canardMemPoolSize = initParams->canardMemPoolSize,
        .nodeStatus = initParams->nodeStatus
    };
    init(&commonInitParams);
}

//insert function (put this in .h) that takes pointer to canardCANframe (thing in my queue), if queue empty return false
//if queue has data make input can frame = top frame in queue (pop input can frame and put into my own queue)
//keep track of how full queue is where you are in queue
//maybe use a circular buffer

bool enqueueRxQueue(const CanardCANFrame *frame)
{
    if (count >= RX_QUEUE_SIZE)
        return false; // queue full

    rxQueue[head] = *frame;     // copy entire frame
    head = (head + 1) % RX_QUEUE_SIZE;
    count++;

    return true;
}

bool dequeueRxQueue(CanardCANFrame *frame)
{
    if (count == 0)
        return false; // queue empty

    *frame = rxQueue[tail];
    tail = (tail + 1) % RX_QUEUE_SIZE;
    count--;

    return true;
}

CanardCANFrame* peekRxQueue()
{
    if (count == 0)
        return NULL; // queue empty
    return &rxQueue[tail];
}

void processCanardRxQueue()
{
    CanardCANFrame *nextRxframe = peekRxQueue();
    if (nextRxframe != NULL)
        handleRxFrame(nextRxframe);
    return;
}