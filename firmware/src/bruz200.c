#include "bruz200.h"

#include "stm32f0xx_hal.h"
#include "shift_registers.h"
#include "stm32f0xx_hal_spi.h"

#define TRANSMIT_DELAY  10
#define ONE_BYTE_CMD    1
#define ADDR_MASK       0b11
#define RES_OFFSET      1

uint8_t pot_order[24] = {
  0, 1, 2, 3,
  12, 13, 14, 15,
  4, 5, 16, 17,
  6, 7, 8, 9,
  18, 19, 20, 21,
  10, 11, 22, 23
};

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

void config_pots(uint8_t *pot_resistances, SPI_HandleTypeDef *hspi) {

  sr_start(BRUZ_SR);
  for (int i = 0; i < NUM_OF_POTS; i++) {

    uint8_t pot_index = pot_order[i] % 6;
    if (pot_index < 3) {
      // 0 -> 0, 1 -> 1, 2 -> 2, 3 -> 3
      set_resistance(pot_index, pot_resistances[pot_order[i]], hspi);
    } else {
      // 4 -> 0, 5 -> 2
      set_resistance((pot_index - 4) * 2, pot_resistances[pot_order[i]], hspi);
    }

    // if we reach the end of an enable bit
    if (pot_index == 3 | pot_index == 5) {
      sr_shift_en(BRUZ_SR);
    }
  }
}
