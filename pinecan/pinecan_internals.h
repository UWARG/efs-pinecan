#pragma once

#include "pinecan.h"

#ifndef PINECAN_DEBUG
#define PINECAN_DEBUG 0
#endif

#ifndef PINECAN_DEBUG_ASSERT
# if PINECAN_DEBUG
#include <assert.h>
#define PINECAN_DEBUG_ASSERT(x) assert(x)
# else
#define PINECAN_DEBUG_ASSERT(x) (void)(x)
# endif
#endif

#ifndef PINECAN_UNUSED
#define PINECAN_UNUSED(x) (void)(x)
#endif

typedef struct {
    CanardInstance *canard;
    struct uavcan_protocol_NodeStatus *nodeStatus;
} CommonInitParams;

void init(CommonInitParams *initParams);
void handleRxFrame(CanardCANFrame *rxFrame);
