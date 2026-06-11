#pragma once

#include "servo.h"

/**
  * @brief  Initialize HAL CAN and canard.
  * @retval None
  */
void initCAN(void);

/**
  * @brief  Send actuator status.
  * @param  servo The servo to send status for.
  * @retval None
  */
void sendActuatorStatus(Servo_t *servo);
