#include "shift_registers.h"
#include "stm32f0xx_hal.h"


void reset_cd22m_sr(void) {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
}

void reset_bruz_sr(void) {

  // bruz is ~cs
  set_bruz_sr_data(1);

  for (int i = 0; i < 8; i++) clock_bruz_sr();
}

void set_bruz_sr_data(int data) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, data);
}

void set_cd22m_sr_data(int data) {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, data);
}

void clock_bruz_sr(void) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
}

void clock_cd22m_sr(void) {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
}

