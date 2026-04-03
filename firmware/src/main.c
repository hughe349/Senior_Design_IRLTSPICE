// my libs
#include "main.h"
#include "init.h"
#include "bruz200.h"
#include "cd22m.h"

// c libs
#include <stdint.h>
#include <stdbool.h>

// temp funcs
void setup_blinker(void);
void send_array(uint8_t *data, uint32_t size);
void send_string(char *str);

// defines
// #define START_STOP_CHAR 64
#define MAX_RES         127
#define MIN_RES         0
#define WAIT_FOR_UART_TX while (!(USART5->ISR & USART_ISR_TXE))

// demo
#define UART_TEST
// #define MIDTERM_DEMO
// #define DEMO_DIGITAL_POT
// #define DEMO_CROSSBAR
// #define DEMO_HIGH_PASS
// #define DEMO_LOW_PASS
// #define DEMO_INVERTING_AMP
// #define DEMO_SINE_SHAPER
// #define DEMO_HALF_WAVE

#ifdef UART_TEST

int main(void)
{
  SPI_HandleTypeDef hspi;
  board_state_t state;

  HAL_Init();
  internal_clock();
  setup_uart();
  setup_spi(&hspi);
  setup_gpios();

  // var inits for loop
  uint8_t arrays[9][128];
  int current_array = -1;

  // in while loop vvvvv
  while (true) {  
    if ((USART5->ISR & USART_ISR_RXNE)) {
      uint8_t rx = USART5->RDR;

      // USART5->TDR = rx;
      // // send_string("blah");

      if (rx == RESET_CONFIG) {
        state = IDLE;
        
        WAIT_FOR_UART_TX;
        // USART5->TDR = RESET_SUCCESS;
        send_string("reseting"); // TEMP
      }

      switch (state) {
        case IDLE:
          if (rx == START_CONFIG) { 
            state = STARTING;
            send_string("starting"); // TEMP
          }
          break;
        case ERROR_STATE:

        case STARTING:
          if (rx == START_CONFIG) { 
            WAIT_FOR_UART_TX;
            USART5->TDR = READY_TO_START;
            send_string("UART_CONFIG"); // TEMP
            state = UART_CONFIG;
          }
          break;
        case UART_CONFIG:
          if (rx == START_CB) { }
          else if (rx == START_POT) { }
          else if (rx == END_CONFIG) { }
          break;
        case CHOOSE_CB_CONNS: break;
        case CHOOSE_POS_RES: break;
        case POST_POT_CONFIG: break;
        case POST_BOARD_CONFIG: break;
        // case ANALOG_CONFIG: break;
        default: break;
      }

      // if (!in_packet) {
      //   send_string("not in packet");
      //   if (rx >= 1 && rx <= 9) {
      //     current_array = rx - 1;
      //     for(int i=0; i<128; i++) arrays[current_array][i] = 0;
      //   }
      //   else if (rx == START_STOP_CHAR && current_array != -1) {
      //     in_packet = true;
      //   }
      // }
      // else {
      //   if (rx == START_STOP_CHAR) {
      //     in_packet = false;
      //     send_string("array updated");
      //     send_array(arrays[current_array], 128);
      //     current_array = -1;

      //   }
      //   else if (rx <= 127 && current_array != -1) {
      //     arrays[current_array][rx] = 1;
      //   }
      // }

      // USART5->TDR = rx;
      // while(!(USART5->ISR & USART_ISR_TC));
    }
  }

}



#endif


#ifdef MIDTERM_DEMO
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
  reset_crossbars();

  // #ifdef DEMO_DIGITAL_POT
    setup_blinker();
    // enabling all connections in tcon
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
    // write_register(1, 0x41FF, &hspi);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0); // manually framing w/ CS is needed rn, will be done by shift registers on real board
  // set_resistance(V_WIPER_0, MAX_RES, &hspi);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);


  while (1)
  {
    // for (uint8_t i = 0; i < 128; i++) {
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
    set_resistance(0, MIN_RES + 1, &hspi);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

    // HAL_Delay(3000);
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
    // set_resistance(0, MIN_RES, &hspi);
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
    // }
  }

  // #endif

  #ifdef DEMO_LOW_PASS
    enable_connection(0, 0);
    enable_connection(1, 3);
    enable_connection(4, 3);
    enable_connection(5, 5);
  #endif

  #ifdef DEMO_HIGH_PASS
    enable_connection(4, 0);
    enable_connection(5, 3);
    enable_connection(0, 3);
    enable_connection(1, 5);
  #endif

  #ifdef DEMO_INVERTING_AMP
    enable_connection(0, 0);

    enable_connection(1, 1);
    enable_connection(2, 1);
    enable_connection(6, 1);

    enable_connection(8, 3);
    enable_connection(3, 3);

    enable_connection(7, 5);
  #endif

  #ifdef DEMO_SINE_SHAPER
    enable_connection(0, 0);

    enable_connection(1, 3);
    enable_connection(2, 3);
    enable_connection(9, 3);
    enable_connection(13, 3);

    enable_connection(3, 5);
    enable_connection(10, 5);
    enable_connection(12, 5);
  #endif

  #ifdef DEMO_HALF_WAVE
    enable_connection(12, 0);
    enable_connection(13, 3);
    enable_connection(0, 3);
    enable_connection(1, 5);
  #endif

}

#endif



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

void send_array(uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        while (!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = data[i];
    }

    while (!(USART5->ISR & USART_ISR_TC));
}

void send_string(char *str)
{
    while (*str) {
        while (!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = *str++;
    }
}

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


void setup_blinker(void) {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
}