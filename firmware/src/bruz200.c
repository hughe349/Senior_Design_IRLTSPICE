#include "bruz200.h"

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"

#define TRANSMIT_DELAY  10
#define ONE_BYTE_CMD    1
#define ADDR_MASK       0b11
#define RES_OFFSET      1

void set_resistance(uint8_t addr, uint8_t res, SPI_HandleTypeDef *hspi) {
  uint8_t tx[2];

  // {wiper_addr[3:0], 4'b0}
  // {write_data[7:0]}
  tx[0] = (uint8_t)(res << RES_OFFSET);
  tx[1] = (uint8_t)(addr & ADDR_MASK);
  HAL_SPI_Transmit(hspi, tx, 1, TRANSMIT_DELAY);
}

void reset_pots(void) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
}
