#include "mt8816.h"
#include "stm32f0xx_hal.h"

void enable_connection(uint8_t address) {
  // turning off all address bits, strobe, and data
  GPIOB->ODR &= ~(0xFF80);

  // turning on correct address bits and data
  GPIOB->ODR |= ((address << 9) | 0x0100);

  // STROBE
  GPIOB->ODR |= 0x0080;
  HAL_Delay(1);
  GPIOB->ODR &= ~(0x0080);
}

void disable_connection(uint8_t address) {
    // turning off all address bits, strobe, and data
    GPIOB->ODR &= ~(0xFF80);

    // turning on correct address bits
    GPIOB->ODR |= (address << 9);
  
    // STROBE
    GPIOB->ODR |= 0x0080;
    HAL_Delay(1);
    GPIOB->ODR &= ~(0x0080);
}
