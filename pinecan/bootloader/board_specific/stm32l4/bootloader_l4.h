#include "can.h"
#include "dronecan_msgs.h"
#include "stm32l4xx_hal.h"
#include <string.h>

void programBootloader(struct uavcan_protocol_file_ReadResponse pkt);
void eraseStagingFlash(void);
uint32_t bootloaderGetTime(void);
