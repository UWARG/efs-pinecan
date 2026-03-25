#include "canard.h"
#include "dynamic_node_allocatee.h"
#include "pinecanBoard_internals.h"
#include "uavcan.protocol.dynamic_node_id.Allocation.h"
#include <string.h>



#define MAX_REQUEST_PERIOD UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_REQUEST_PERIOD_MS
#define MIN_REQUEST_PERIOD UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS
#define MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST
#define MAX_FOLLOWUP_DELAY_MS UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS
#define MIN_FOLLOWUP_DELAY_MS UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_FOLLOWUP_DELAY_MS

typedef enum {
    ALLOC_STATE_IDLE,
	ALLOC_STATE_WAITING_FOR_ACK_0, /* sent chunk 1, waiting for ack */
	ALLOC_STATE_WAITING_FOR_ACK_1, /* sent chunk 2, waiting for ack */
	ALLOC_STATE_WAITING_FOR_ALLOC, /* sent chunk 3, wait for final node ID */
	ALLOC_STATE_RANDOM_DELAY, /* random delay before every chunk to avoid collision */
    ALLOC_STATE_ALLOCATED
} alloc_state_t;


static alloc_state_t _state = ALLOC_STATE_IDLE;
static uint32_t _timer_ms = 0;
static uint32_t _random_delay_ms = 0;
static uint8_t _unique_id[16] = {0};
static uint8_t _next_chunk = 0;
static CanardInstance* _ins = NULL;
static uint8_t transferID = 0;
static void sendChunk(uint8_t chunk_index);

static uint32_t getRandomRange(uint32_t min, uint32_t max){
	if (max <= min) return min;
	uint32_t uid_seed;
	memcpy(&uid_seed, _unique_id, 4);
	uint32_t span = max - min + 1;
	uint32_t noise = getUptimeMs() ^ uid_seed;
	return min + (noise % span);
}

void dnIdAllocateeInit(CanardInstance *ins){
    canardSetLocalNodeID(ins, 0);
    _state = ALLOC_STATE_IDLE;
    if (ins == NULL){
    	return;
    }
    _ins = ins;
    const uint8_t* unique_id = getUniqueHardwareID();
    memcpy(_unique_id, unique_id, 16);
    _random_delay_ms = getRandomRange(MIN_REQUEST_PERIOD, MAX_REQUEST_PERIOD );

}


void dnIdAllocatee1ms(void){
	if (_state == ALLOC_STATE_ALLOCATED){
		return;
	}
	_timer_ms++;
	switch (_state){
	case ALLOC_STATE_IDLE:
		if(_timer_ms >= _random_delay_ms){
			_timer_ms = 0;
			sendChunk(_next_chunk);
			_state = ALLOC_STATE_WAITING_FOR_ACK_0;
		}
		break;
	case ALLOC_STATE_RANDOM_DELAY:
		if (_timer_ms >= _random_delay_ms){
			_timer_ms = 0;
			sendChunk(_next_chunk);
			if (_next_chunk == 1) _state = ALLOC_STATE_WAITING_FOR_ACK_1;
			else if (_next_chunk == 2) _state = ALLOC_STATE_WAITING_FOR_ALLOC;

		}
		break;
	case ALLOC_STATE_WAITING_FOR_ACK_0:
	case ALLOC_STATE_WAITING_FOR_ACK_1:
	case ALLOC_STATE_WAITING_FOR_ALLOC:
		if (_timer_ms >= UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_FOLLOWUP_TIMEOUT_MS) {
			//Abort current attempt and restart
			_state = ALLOC_STATE_IDLE;
			_random_delay_ms = getRandomRange(MIN_REQUEST_PERIOD, MAX_REQUEST_PERIOD);
			_timer_ms = 0;
			_next_chunk = 0;
		}
		break;
	default:
		break;

	}
}


void handleDnIDAllocationRes(CanardInstance* ins, CanardRxTransfer* transfer){
	if (_state == ALLOC_STATE_ALLOCATED){
		return;
	}
    struct uavcan_protocol_dynamic_node_id_Allocation msg;

    //Decode Failed
    if (uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg)){
    	return;
    }

	if (_state == ALLOC_STATE_WAITING_FOR_ALLOC && msg.node_id != 0) {
		if (msg.unique_id.len == 16 && !memcmp(msg.unique_id.data, _unique_id, 16)) {
			canardSetLocalNodeID(_ins, msg.node_id);
			_state = ALLOC_STATE_ALLOCATED;
		}
		return;
	}

	if (!memcmp(msg.unique_id.data, _unique_id, msg.unique_id.len)) {
		if (_state == ALLOC_STATE_WAITING_FOR_ACK_0 && msg.first_part_of_unique_id){
			_next_chunk = 1;
			_timer_ms = 0;
			_state = ALLOC_STATE_RANDOM_DELAY;
			_random_delay_ms = getRandomRange(MIN_FOLLOWUP_DELAY_MS, MAX_FOLLOWUP_DELAY_MS );
		} else if (_state == ALLOC_STATE_WAITING_FOR_ACK_1 && !msg.first_part_of_unique_id && msg.unique_id.len == 12){
			_next_chunk = 2;
			_timer_ms = 0;
			_state = ALLOC_STATE_RANDOM_DELAY;
			_random_delay_ms = getRandomRange(MIN_FOLLOWUP_DELAY_MS, MAX_FOLLOWUP_DELAY_MS );
		}

	}

   
}


uint8_t dnIdAllocateeGetAssignedNodeID(void) {
    if (_state == ALLOC_STATE_ALLOCATED){
        return canardGetLocalNodeID(_ins);
    } else {
        return 0;
    }
}

static void sendChunk(uint8_t chunk_index){
	struct uavcan_protocol_dynamic_node_id_Allocation msg = {0};

	if (chunk_index == 0){
		msg.first_part_of_unique_id = true;
		msg.unique_id.len = MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST;
		memcpy(msg.unique_id.data, _unique_id, msg.unique_id.len);
	}else if (chunk_index == 1){
		msg.first_part_of_unique_id = false;
		msg.unique_id.len = MAX_LENGTH_OF_UNIQUE_ID_IN_REQUEST;
		memcpy(msg.unique_id.data, &_unique_id[6], msg.unique_id.len);
	}else if (chunk_index == 2){
		msg.first_part_of_unique_id = false;
		msg.unique_id.len = 4;
		memcpy(msg.unique_id.data, &_unique_id[12], msg.unique_id.len);
	}else{
		return;
	}

	uint8_t txBuffer[UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_SIZE] = {0};
	uint32_t len = uavcan_protocol_dynamic_node_id_Allocation_encode(&msg, txBuffer);

	CanardTxTransfer txFrame = {
		.transfer_type = CanardTransferTypeBroadcast,
		.data_type_signature =  UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE,
        .data_type_id = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
		.inout_transfer_id = &transferID,
		.priority = CANARD_TRANSFER_PRIORITY_LOW,
		.payload = txBuffer,
		.payload_len = len
	};

	canardBroadcastObj(_ins, &txFrame);

}
