#include "mcp4241.h"

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"

#define TRANSMIT_DELAY  10
#define ONE_BYTE_CMD    1
#define TWO_BYTE_CMD    2
#define ADDR_SHIFT      4
#define INCR_CMD        0x4
#define DECR_CMD        0x8

void incr_resistance(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {
  uint8_t tx[1];

  // {wiper_addr[3:0], 2'b01, 2'b0}
  tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | INCR_CMD;
  HAL_SPI_Transmit(hspi, tx, ONE_BYTE_CMD, TRANSMIT_DELAY);
}

void decr_resistance(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {
  uint8_t tx[1];

  // {wiper_addr[3:0], 2'b10, 2'b0}
  tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | DECR_CMD;
  HAL_SPI_Transmit(hspi, tx, ONE_BYTE_CMD, TRANSMIT_DELAY);
}

void set_resistance(MCP4241_addr_t wiper_addr, uint8_t res, SPI_HandleTypeDef *hspi) {
  uint8_t tx[2];

  // {wiper_addr[3:0], 4'b0}
  // {write_data[7:0]}
  tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT);
  tx[1] = res;
  HAL_SPI_Transmit(hspi, tx, TWO_BYTE_CMD, TRANSMIT_DELAY);
}

//uint16_t read_register(MCP4241_addr_t wiper_addr, SPI_HandleTypeDef *hspi) {}

void write_register(MCP4241_addr_t wiper_addr, uint16_t write_data, SPI_HandleTypeDef *hspi) {

  // {wiper_addr[3:0], cmd[1:0], write_data[9:8]}
  // {write_data[7:0]}
  uint8_t tx[2];
  tx[0] = (uint8_t)(wiper_addr << ADDR_SHIFT) | (uint8_t)((write_data >> 8) & 0x3);
  tx[1] = (uint8_t)(write_data & 0xFF);

  HAL_SPI_Transmit(hspi, tx, TWO_BYTE_CMD, TRANSMIT_DELAY);
}
