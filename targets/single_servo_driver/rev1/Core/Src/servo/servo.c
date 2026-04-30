#include "servo.h"
#include "can.h"

#define SERVO_COUNT 1

extern TIM_HandleTypeDef htim1;

static Servo_t servos[SERVO_COUNT] = {
  {
    .actuatorID = 1U,
    .timer = &htim1,
    .channel = TIM_CHANNEL_1,
    .minDutyCycle = 0.05,
    .maxDutyCycle = 0.10,
    .minCCR = 0,
    .maxCCR = 0,
    .period = 0
  }
};

void initServos(void)
{
  for(int i = 0; i < SERVO_COUNT; ++i)
  {
    uint32_t prescaler = servos[i].timer->Instance->PSC + 1;
    uint32_t period = servos[i].timer->Instance->ARR + 1;

    servos[i].minCCR = servos[i].minDutyCycle * period;
    servos[i].maxCCR = servos[i].maxDutyCycle * period;
    servos[i].period = HAL_RCC_GetPCLK1Freq() / (prescaler * period) * 1e6;

    uint32_t halfDutyCycle = (servos[i].minCCR + servos[i].maxCCR) / 2.0;
    __HAL_TIM_SET_COMPARE(servos[i].timer, servos[i].channel, halfDutyCycle);
    HAL_TIMEx_PWMN_Start (servos[i].timer, servos[i].channel);
  }
}

void dutyCycleActuation(float dutyCycle, uint8_t actuatorID)
{
  for(int i = 0; i < SERVO_COUNT; ++i)
  {
    if (servos[i].actuatorID != actuatorID)
    {
      continue;
    }

    uint32_t ccrValue = servos[i].timer->Instance->ARR * dutyCycle / servos[i].period;
    __HAL_TIM_SET_COMPARE(servos[i].timer, servos[i].channel, ccrValue);
    return;
  }
}

void percentageActuation(float percentage, uint8_t actuatorID)
{
  for(int i = 0; i < SERVO_COUNT; ++i)
  {
    if (servos[i].actuatorID != actuatorID)
    {
      continue;
    }

    percentage = (percentage + 1) / 2;
    uint32_t ccrValue = servos[i].minCCR + percentage * (servos[i].maxCCR - servos[i].minCCR);
    __HAL_TIM_SET_COMPARE(servos[i].timer, servos[i].channel, ccrValue);
    return;
  }
}

void sendActuatorStatuses(void)
{
  for(int i = 0; i < SERVO_COUNT; ++i)
  {
    sendActuatorStatus(&servos[i]);
  }
}
