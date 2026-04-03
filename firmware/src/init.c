#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_spi.h"
#include "init.h"

#define STALL while (1) {}

void setup_spi(SPI_HandleTypeDef *hspi)
{
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // gpio config
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin           = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 |
                                  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode          = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull          = GPIO_NOPULL;
  GPIO_InitStruct.Speed         = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate     = GPIO_AF0_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // spi config
  hspi->Instance                = SPI1;
  hspi->Init.Mode               = SPI_MODE_MASTER;
  hspi->Init.Direction          = SPI_DIRECTION_2LINES;
  hspi->Init.DataSize           = SPI_DATASIZE_10BIT;
  hspi->Init.CLKPolarity        = SPI_POLARITY_LOW;
  hspi->Init.CLKPhase           = SPI_PHASE_1EDGE;
  hspi->Init.NSS                = SPI_NSS_SOFT;
  hspi->Init.BaudRatePrescaler  = SPI_BAUDRATEPRESCALER_256;
  hspi->Init.FirstBit           = SPI_FIRSTBIT_MSB;
  hspi->Init.TIMode             = SPI_TIMODE_DISABLE;
  hspi->Init.CRCCalculation     = SPI_CRCCALCULATION_DISABLE;

  if (HAL_SPI_Init(hspi) != HAL_OK) { STALL }

  SET_BIT(SPI1->CR2, SPI_CR2_FRXTH);
}

void setup_gpios(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIOB_InitStruct = {0};
  GPIOB_InitStruct.Pin =  GPIO_PIN_3  |
                          GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  |
                          GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  |
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  GPIOB_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOB_InitStruct.Pull = GPIO_NOPULL;
  GPIOB_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIOB_InitStruct);

  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIOC_InitStruct = {0};
  GPIOC_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIOC_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOC_InitStruct.Pull = GPIO_NOPULL;
  GPIOC_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIOC_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);  // active low resets
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
}

/*
 * BELOW INIT FUNCTIONS BORROWED FROM NIRAJ'S 362 SPRING 25 REPO
 */
void internal_clock()
{
  /* Disable HSE to allow use of the GPIOs */
  RCC->CR &= ~RCC_CR_HSEON;
  /* Enable Prefetch Buffer and set Flash Latency */
  FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY;
  /* HCLK = SYSCLK */
  RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
  /* PCLK = HCLK */
  RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE_DIV1;
  /* PLL configuration = (HSI/2) * 12 = ~48 MHz */
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL));
  RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLXTPRE_HSE_PREDIV_DIV1 | RCC_CFGR_PLLMUL12);
  /* Enable PLL */
  RCC->CR |= RCC_CR_PLLON;
  /* Wait till PLL is ready */
  while ((RCC->CR & RCC_CR_PLLRDY) == 0)
    ;
  /* Select PLL as system clock source */
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;
  /* Wait till PLL is used as system clock source */
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
    ;

  // fix hal timing
  SystemCoreClock = 48000000;
  HAL_InitTick(TICK_INT_PRIORITY);
}

void setup_uart(void)
{
  RCC->AHBENR |= 0x00180000;
  GPIOC->MODER |= 0x02000000;
  GPIOC->AFR[1] |= 0x00020000;
  GPIOD->MODER |= 0x00000020;
  GPIOD->AFR[0] |= 0x00000200;
  RCC->APB1ENR |= 0x00100000;
  USART5->CR1 &= ~0x00000001;
  USART5->CR1 |= 0x00008000;
  USART5->BRR = 0x340;
  USART5->CR1 |= 0x0000000c;
  USART5->CR1 |= 0x00000001;
}
