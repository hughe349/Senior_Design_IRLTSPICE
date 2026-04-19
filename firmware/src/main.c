// my libs
#include "main.h"
#include "init.h"
#include "bruz200.h"
#include "cd22m.h"
#include "shift_registers.h"

// c libs
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


// temp funcs
void setup_blinker(void);
void send_array(uint8_t *data, uint32_t size);
// void send_string(char *str);

#define WAIT_FOR_UART_TX while (!(USART5->ISR & USART_ISR_TXE))
#define NUM_OF_CROSSBARS      9
#define NUM_OF_CROSSBAR_CONS  128
#define MAX_POT_RES           128
#define ERROR_DELAY           2000

uint8_t crossbar_order[9] = {
  0, 3, 6,
  1, 4, 7,
  2, 5, 8
};

int main(void)
{
  SPI_HandleTypeDef hspi;
  board_state_t state;

  HAL_Init();
  internal_clock();
  setup_uart();
  setup_spi(&hspi);
  setup_gpios();
  setup_blinker();

  // var inits for loop
  uint8_t crossbar_cons[NUM_OF_CROSSBARS][NUM_OF_CROSSBAR_CONS] = {0};
  uint8_t pot_resistances[NUM_OF_POTS] = {0};
  uint8_t rx;
  // instruction_t instruction;

  uint8_t cur_cb;
  uint8_t cur_pot;

  reset_crossbars();
  reset_pots();

  // while (true) {
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  //   HAL_Delay(500);

  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  //   HAL_Delay(500);

  // }

  sr_set(CD22M_SR, 0);
  sr_reset(CD22M_SR);
  sr_start(CD22M_SR);
  // HAL_Delay(5000);
  sr_set(CD22M_SR, 1);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR); // top level
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  // sr_set(CD22M_SR, 0);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR); // cell #1 cb 0
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  // HAL_Delay(5000);
  // sr_clock(CD22M_SR);
  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  // sr_start(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // sr_shift_en(CD22M_SR);
  // while (true) {  
  //   HAL_Delay(4000);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  //   enable_connection(0, 0);
  //   HAL_Delay(4000);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  //   disable_connection(0, 0);
  // }
  // enable_connection(0, 0);

  // set_resistance(0, 100, &hspi);

  // sr_shift_en(BRUZ_SR);
  // sr_shift_en(BRUZ_SR);



  while (true);

  // sr_shift_en(BRUZ_SR);

  // set_resistance

  while (true) {  

    // if ((state == ERROR_STATE) & (!error_flag_sent)) {
    //   WAIT_FOR_UART_TX;
    //   USART5->TDR = UART_ERROR;
    //   error_flag_sent = 1;
    // }

    // reset_crossbars();
    // while (true) { 
    //   if (USART5->ISR & USART_ISR_RXNE) {
    //     WAIT_FOR_UART_TX;
    //     USART5->TDR = USART5->RDR;
    //   }

    // }


    if ((USART5->ISR & USART_ISR_RXNE)) {
      // error_flag_sent = 0;
      rx = USART5->RDR;
      // to_instruction(&instruction, rx);

      if (rx == RESET_CONFIG) {
        state = IDLE;

        memset(crossbar_cons,   0, sizeof(crossbar_cons));
        memset(pot_resistances, 0, sizeof(pot_resistances));

        reset_crossbars();
        reset_pots();
        sr_reset(BRUZ_SR);
        sr_reset(CD22M_SR);

        WAIT_FOR_UART_TX;
        USART5->TDR = RESET_SUCCESS;
        continue;
      }

      switch (state) {
        case IDLE:
          if (rx == START_CONFIG) { state = STARTING; }
          else                    { state = ERROR_STATE; }
          break;
        
        case ERROR_STATE:
          WAIT_FOR_UART_TX;
          USART5->TDR = UART_ERROR;
          break;
        
        case STARTING:
          if (rx == START_CONFIG) { 
            WAIT_FOR_UART_TX;
            USART5->TDR = READY_TO_START;
            state = UART_CONFIG;
          } else {
            state = ERROR_STATE;
          }
          break;

        case UART_CONFIG:
          if (get_prefix(rx) == START_CB) { 
            cur_cb = get_message(rx);
            state = CHOOSE_CB_CONNS;
          } else if (get_prefix(rx) == START_POT) {
            cur_pot = get_message(rx);
            state = CHOOSE_POT_RES;
          } else if (rx == END_CONFIG) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

            reset_crossbars();
            reset_pots();

            sr_reset(BRUZ_SR);
            sr_reset(CD22M_SR);

            WAIT_FOR_UART_TX;
            USART5->TDR = CONFIG_SUCCESS;

            for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
              WAIT_FOR_UART_TX;
              USART5->TDR = i;
              for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
                if (crossbar_cons[i][j]) {
                  WAIT_FOR_UART_TX;
                  USART5->TDR = j;
                }
              }
              WAIT_FOR_UART_TX;
              USART5->TDR = '\n';
            }

            for (int i = 0; i < NUM_OF_POTS; i++) {
              if (pot_resistances[i]) {
                // send_num(i);
                WAIT_FOR_UART_TX;
                USART5->TDR = i;
                // send_num(pot_resistances[i]);
                WAIT_FOR_UART_TX;
                USART5->TDR = pot_resistances[i];
                WAIT_FOR_UART_TX;
                USART5->TDR = '\n';
              }
            }

            // config_pots(pot_resistances, &hspi);

            // // crossbar config
            // sr_start(CD22M_SR);
            // for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
            //   for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
            //     if (crossbar_cons[crossbar_order[i]][j]) {
            //       enable_connection(get_x(j), get_y(j));
            //     }
            //   }
            //   sr_shift_en(CD22M_SR);
            // }
            // state = IDLE;

            // WAIT_FOR_UART_TX;
            // USART5->TDR = CONFIG_SUCCESS;

          } else {
            state = ERROR_STATE;
          }
          break;

        case CHOOSE_CB_CONNS:
          if (rx < NUM_OF_CROSSBAR_CONS) {
            crossbar_cons[cur_cb][rx] = 1;
          }
          else if (rx == END_CB) { 
            state = UART_CONFIG;
            WAIT_FOR_UART_TX;
            USART5->TDR = RETURN_TO_CONFIG;
          }
          else { 
            state = ERROR; 
          }
          break;

        case CHOOSE_POT_RES: 
          if (rx < MAX_POT_RES) {
            pot_resistances[cur_pot] = rx;
            USART5->TDR = RETURN_TO_CONFIG;
            state = UART_CONFIG;
          } else { 
            state = ERROR_STATE;
          }
          break;
        default: break;
      }
    }
  }

}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

void setup_blinker(void) {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
}

// for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
//   WAIT_FOR_UART_TX;
//   USART5->TDR = i;
//   for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
//     if (crossbar_cons[i][j]) {
//       WAIT_FOR_UART_TX;
//       USART5->TDR = j;
//     }
//   }
//   WAIT_FOR_UART_TX;
//   USART5->TDR = '\n';
// }

// for (int i = 0; i < NUM_OF_POTS; i++) {
//   if (pot_resistances[i]) {
//     // send_num(i);
//     WAIT_FOR_UART_TX;
//     USART5->TDR = i;
//     // send_num(pot_resistances[i]);
//     WAIT_FOR_UART_TX;
//     USART5->TDR = pot_resistances[i];
//     WAIT_FOR_UART_TX;
//     USART5->TDR = '\n';
//   }
// }

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
