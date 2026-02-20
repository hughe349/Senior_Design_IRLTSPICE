// my libs
#include "main.h"
#include "init.h"
#include "mcp4241.h"
#include "mt8816.h"

// c libs
#include <stdint.h>
#include <stdbool.h>

// temp funcs
void setup_blinker(void);

// defines
#define START_STOP_CHAR 64
#define MAX_RES         127
#define MIN_RES         0

int main(void)
{
  SPI_HandleTypeDef hspi;

  // setup funcs
  HAL_Init();
  internal_clock();
  setup_uart();
  setup_spi(&hspi);
  setup_crossbar_gpios();

  // clearing odr
  GPIOB->ODR &= 0;

  // reset crossbar
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);

  enable_connection(0b00001100);


  while(1) {
    enable_connection(0b00000010);
    HAL_Delay(1000);
    disable_connection(0b00000010);
    enable_connection(0b00000001);
    HAL_Delay(1000);
    enable_connection(0b00000010);
    HAL_Delay(1000);
    disable_connection(0b00000010);
    disable_connection(0b00000001);
    HAL_Delay(1000);
  }

}



void SysTick_Handler(void)
{
  HAL_IncTick();
}


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    OLD UART CODE
   -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
  // var inits for loop
  // uint8_t arrays[9][128];
  // int current_array = -1;
  // bool in_packet = false;

  //    in while loop vvvvv
  //      	if ((USART5->ISR & USART_ISR_RXNE)) {
  //   			uint8_t rx = USART5->RDR;
    
  //   			if (!in_packet) {
  //   				send_string("not in packet");
  //   				if (rx >= 1 && rx <= 9) {
  //   					current_array = rx - 1;
  //   					for(int i=0; i<128; i++) arrays[current_array][i] = 0;
  //   				}
  //   				else if (rx == START_STOP_CHAR && current_array != -1) {
  //   					in_packet = true;
  //   				}
  //   			}
  //   			else {
  //   				if (rx == START_STOP_CHAR) {
  //   					in_packet = false;
  //   					send_string("array updated");
  //   					send_array(arrays[current_array], 128);
  //   					current_array = -1;
    
  //   				}
  //   				else if (rx <= 127 && current_array != -1) {
  //   					arrays[current_array][rx] = 1;
  //   				}
  //   			}
    
  //   			USART5->TDR = rx;
  //   			while(!(USART5->ISR & USART_ISR_TC));
  //      	}

  // void send_array(uint8_t *data, uint32_t size)
// {
//     for (uint32_t i = 0; i < size; i++) {
//         while (!(USART5->ISR & USART_ISR_TXE));
//         USART5->TDR = data[i];
//     }

//     while (!(USART5->ISR & USART_ISR_TC));
// }

// void send_string(char *str)
// {
//     while (*str) {
//         while (!(USART5->ISR & USART_ISR_TXE));
//         USART5->TDR = *str++;
//     }
// }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    OLD DIGITAL POT CODE
   -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */

//    setup_blinker();
//      // enabling all connections in tcon
//   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
//   write_register(TCON_REG, 0x41FF, &hspi);
//   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

//   // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0); // manually framing w/ CS is needed rn, will be done by shift registers on real board
//   // set_resistance(V_WIPER_0, MAX_RES, &hspi);
//   // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);


//   while (1)
//   {
//     HAL_Delay(3000);
//     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
//     set_resistance(V_WIPER_0, MAX_RES, &hspi);
//     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

//     HAL_Delay(3000);
//     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
//     set_resistance(V_WIPER_0, MIN_RES, &hspi);
//     HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
//     // HAL_Delay(2000);
//     // decr_resistance(V_WIPER_0, &hspi);

    
//   }
// }


// void setup_blinker(void) {
//   __HAL_RCC_GPIOC_CLK_ENABLE();
//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   GPIO_InitStruct.Pin = GPIO_PIN_6;
//   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//   GPIO_InitStruct.Pull = GPIO_NOPULL;
//   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
// }