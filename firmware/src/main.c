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

// #define SINGLE_CELL
#define REAL_BOARD

#define WAIT_FOR_UART_TX while (!(USART5->ISR & USART_ISR_TXE))
#ifdef REAL_BOARD
  #define NUM_OF_CROSSBARS      9
  #define NUM_OF_POTS           24
#endif
#ifdef SINGLE_CELL
  uint8_t x_addr_single_cell[16] = {0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 6, 7, 14, 15};

  static void program_single_cell_pot(int bruz_num, int pot_num, uint8_t res, SPI_HandleTypeDef *hspi);
  static void enable_single_cell_connection(int cd22m_num, uint8_t x_pin, uint8_t y_pin);
  static void setup_single_cell_gpios(void);
  static void reset_single_cell_cd22m(void);
  static void reset_single_cell_bruz(void);

  #define NUM_OF_CROSSBARDS     2
  #define NUM_OF_POTS           6
#endif
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

  HAL_Init();
  internal_clock();
  setup_uart();
  setup_spi(&hspi);
  setup_blinker();
  setup_gpios();
  // var inits for loop

  #ifdef REAL_BOARD
  board_state_t state = IDLE;
  uint8_t crossbar_cons[NUM_OF_CROSSBARS][NUM_OF_CROSSBAR_CONS] = {0};
  uint8_t pot_resistances[NUM_OF_POTS] = {0};
  uint8_t rx;
  // instruction_t instruction;

  uint8_t cur_cb;
  uint8_t cur_pot;

  reset_crossbars();
  reset_pots();

  sr_start(CD22M_SR);
  enable_connection(14,4);
  // for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
  //   for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
  //     if (crossbar_cons[crossbar_order[i]][j]) {
  //       enable_connection(get_x(j), get_y(j));
  //     }
  //   }
  // Shift twice = cell 2 bar 1
  // Y4 is input
  sr_shift_en(CD22M_SR);
  sr_shift_en(CD22M_SR);
  // enable_connection(14,0);
  // Divider
  // enable_connection(6,4);
  // enable_connection(7,7);
  // enable_connection(8,7);
  // enable_connection(9,0);

  // IDK
  // enable_connection(1,4);
  // enable_connection(0,7);
  // enable_connection(2,7);

  // Inverting amp
  // enable_connection(6,4);
  // enable_connection(7,3);
  // enable_connection(2,3);
  // enable_connection(8,3);
  // enable_connection(1,0);
  // enable_connection(9,7);
  // enable_connection(0,7);

  sr_shift_en(CD22M_SR);
  sr_shift_en(CD22M_SR);
  sr_shift_en(CD22M_SR);
  sr_shift_en(CD22M_SR);
  sr_shift_en(CD22M_SR);
  // At 7 now
  enable_connection(0,0);


  while (true) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
    HAL_Delay(500);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
    HAL_Delay(500);

  }

  // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

  // sr_set(CD22M_SR, 0);
  // sr_reset(CD22M_SR);
  // sr_start(CD22M_SR);

  // while (true) {

  //   HAL_Delay(5000);
  //   enable_connection(1, 1);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);

  //   HAL_Delay(5000);
  //   disable_connection(1, 1);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);

  // }
  // for (int i = 0; i < 5; i++) { 
  //   sr_clock(CD22M_SR);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
  //   HAL_Delay(1000);
  //   sr_clock(CD22M_SR);
  //   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
  //   HAL_Delay(1000);
  // }
  // sr_set(CD22M_SR, 1);

  while (true);

  while (true) {  

    if (state == ERROR_STATE) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
    }

    if (state == IDLE) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
    }

    // reset_crossbars();
    // while (true) { 
    //   if (USART5->ISR & USART_ISR_RXNE) {
    //     WAIT_FOR_UART_TX;
    //     USART5->TDR = USART5->RDR;
    //   }

    // }


    if ((USART5->ISR & USART_ISR_RXNE)) {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);
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
  #endif

  #ifdef SINGLE_CELL
  reset_crossbars();
  // reset_pots();
  // enable_connection(15, 1);
  

  /* low pass */
  // enable_connection(8, 1);
  // enable_connection(9, 2);

  // enable_connection(10, 2);
  // enable_connection(11, 4);

  /* high pass */

  enable_single_cell_connection(0, 14, 0);
  enable_single_cell_connection(0, 15, 1);
  enable_single_cell_connection(0, 10, 1);
  enable_single_cell_connection(0, 11, 4);

  program_single_cell_pot(0, 0, 96, &hspi);
  program_single_cell_pot(0, 1, 96, &hspi);
  program_single_cell_pot(0, 2, 96, &hspi);
  program_single_cell_pot(0, 3, 96, &hspi);

  // enable_connection(6, 1);

  // enable_connection(7, 3);
  // enable_connection(1, 3);
  // enable_connection(10, 3);

  // enable_connection(2, 4);

  // enable_connection(0, 2);
  // enable_connection(11, 2);

  // program_single_cell_pot(0, 1, 96, &hspi);
  
  // void set_resistance(uint8_t addr, uint8_t res, SPI_HandleTypeDef *hspi)
  // addr 0 -> wiper 1
  // addr 3 -> wiper 4
  // 128 == 200k, 0 = 1k
  // &hspi

  // ncs = 0
  // set_resistance(uint8_t addr, uint8_t res, SPI_HandleTypeDef *hspi);
  // ncs = 1
  


  // enable_connection(8, 0);
  // enable_connection(9, 4);
  // enable_connection(10, 1);
  // enable_connection(11, 4);

  // HAL_Delay(5000);
  // enable_connection(8, 0);


  // enable_connection(2, 0);


  while(true) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);
    HAL_Delay(500);
  }

  #endif

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

#ifdef SINGLE_CELL
static void program_single_cell_pot (int bruz_num, int pot_num, uint8_t res, SPI_HandleTypeDef *hspi) {
  if (bruz_num == 0) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 0);
  else               HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);

  set_resistance(pot_num, res, hspi);

  if (bruz_num == 0) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
  else               HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);
}

static void reset_single_cell_bruz(void) {
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 0);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1);
}

static void reset_single_cell_cd22m(void) {
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 1);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, 0);
}

static void enable_single_cell_connection(int cd22m_num, uint8_t x_pin, uint8_t y_pin) {
  // turning off all address bits, strobe, and data
  // GPIOB->ODR &= ~(0xFF80);

  if (cd22m_num == 0) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 1);
  else                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 1);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, 1); // DATA

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, (x_addr_single_cell[x_pin] >> 3) & 0b1); // AX3
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14 , (x_addr_single_cell[x_pin] >> 2) & 0b1); // AX2
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13 , (x_addr_single_cell[x_pin] >> 1) & 0b1); // AX1
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, (x_addr_single_cell[x_pin] >> 0) & 0b1); // AX0

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, (y_pin >> 2) & 0b1); // AY2
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, (y_pin >> 1) & 0b1); // AY1
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9 , (y_pin >> 0) & 0b1); // AY0

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,  1); // STROBE
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7,  0);

  if (cd22m_num == 0) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, 0);
  else                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, 0);
}

static void setup_single_cell_gpios(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIOB_InitStruct = {0};
  GPIOB_InitStruct.Pin =  GPIO_PIN_3  | GPIO_PIN_15 |
                          GPIO_PIN_6  | GPIO_PIN_7  | GPIO_PIN_8  |
                          GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_11 |
                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
  GPIOB_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOB_InitStruct.Pull = GPIO_NOPULL;
  GPIOB_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIOB_InitStruct);

  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIOA_InitStruct = {0};
  GPIOA_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                         GPIO_PIN_10;
  GPIOA_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIOA_InitStruct.Pull = GPIO_NOPULL;
  GPIOA_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIOA_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, 1); // active lows
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0); // active highs
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, 0);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, 0);

}
#endif

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
