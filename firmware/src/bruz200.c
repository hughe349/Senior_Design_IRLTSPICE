#include "bruz200.h"

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"

#define TRANSMIT_DELAY  10
#define ONE_BYTE_CMD    1
#define ADDR_MASK       0b11
#define RES_OFFSET      1


// void incr_resistance(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {
//   uint8_t tx[1];

//   // {wiper_addr[3:0], 2'b01, 2'b0}
//   tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | INCR_CMD;
//   HAL_SPI_Transmit(hspi, tx, ONE_BYTE_CMD, TRANSMIT_DELAY);
// }

// void decr_resistance(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {
//   uint8_t tx[1];

//   // {wiper_addr[3:0], 2'b10, 2'b0}
//   tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | DECR_CMD;
//   HAL_SPI_Transmit(hspi, tx, ONE_BYTE_CMD, TRANSMIT_DELAY);
// }

void set_resistance(uint8_t addr, uint8_t res, SPI_HandleTypeDef *hspi) {
  uint8_t tx[2];

  // {wiper_addr[3:0], 4'b0}
  // {write_data[7:0]}
  tx[0] = (uint8_t)(res << RES_OFFSET);
  tx[1] = (uint8_t)(addr & ADDR_MASK);
  HAL_SPI_Transmit(hspi, tx, 1, TRANSMIT_DELAY);
}

//uint16_t read_register(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {}

// void write_register(MCP4241_addr_t wiper_addr, uint16_t write_data, SPI_HandleTypeDef *hspi) {

//   // {wiper_addr[3:0], cmd[1:0], write_data[9:8]}
//   // {write_data[7:0]}
//   uint8_t tx[2];
//   tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | (uint8_t)((write_data >> 8) & 0x3);
//   tx[1] = (uint8_t)(write_data & 0xFF);

//   HAL_SPI_Transmit(hspi, tx, TWO_BYTE_CMD, TRANSMIT_DELAY);
// }
