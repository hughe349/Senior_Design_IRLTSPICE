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

// helpers
// static inline void to_instruction(instruction_t *instruction, uint8_t value);
// static void mem_dump(uint8_t *cb_array, uint8_t *pot_array);
static void send_num(uint8_t num);

// temp funcs
void setup_blinker(void);
void send_array(uint8_t *data, uint32_t size);
void send_string(char *str);

#define WAIT_FOR_UART_TX while (!(USART5->ISR & USART_ISR_TXE))
#define NUM_OF_CROSSBARS      9
#define NUM_OF_CROSSBAR_CONS  128
#define NUM_OF_POTS           24
#define MAX_POT_RES           128
// #define CROSSBAR_ARRAY_SIZE   NUM_OF_CROSSBARS * NUM_OF_CROSSBAR_CONS * sizeof(uint8_t)
// #define POT_ARRAY_SIZE        NUM_OF_POTS * sizeof(uint8_t)
#define ERROR_DELAY           2000

// defines
// #define START_STOP_CHAR 64

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

  setup_blinker();

  // var inits for loop
  uint8_t crossbar_cons[NUM_OF_CROSSBARS][NUM_OF_CROSSBAR_CONS] = {0};
  uint8_t pot_resistances[NUM_OF_POTS] = {0};
  uint8_t rx;
  // instruction_t instruction;

  uint8_t cur_cb;
  uint8_t cur_pot;
  bool error_flag_sent = 0;

  while (true) {  

    if ((state == ERROR_STATE) & (!error_flag_sent)) {
      WAIT_FOR_UART_TX;
      USART5->TDR = UART_ERROR;
      error_flag_sent = 1;
      // HAL_Delay(ERROR_DELAY);
    }


    if ((USART5->ISR & USART_ISR_RXNE)) {
      error_flag_sent = 0;
      rx = USART5->RDR;
      // to_instruction(&instruction, rx);

      if (rx == RESET_CONFIG) {
        state = IDLE;
        WAIT_FOR_UART_TX;
        USART5->TDR = RESET_SUCCESS;

        memset(crossbar_cons,   0, sizeof(crossbar_cons));
        memset(pot_resistances, 0, sizeof(pot_resistances));

        reset_crossbars();
        reset_pots();
        reset_bruz_sr();
        reset_cd22m_sr();
        continue;
      }

      switch (state) {
        case IDLE:
          if (rx == START_CONFIG) { state = STARTING; }
          else                    { state = ERROR_STATE; }
          break;
        case STARTING:
          if (rx == START_CONFIG) { 
            WAIT_FOR_UART_TX;
            USART5->TDR = READY_TO_START;
            state = UART_CONFIG;
          } else { state = ERROR_STATE; }
          break;
        case UART_CONFIG:
          if (get_prefix(rx) == START_CB) { 
            cur_cb = get_message(rx);
            state = CHOOSE_CB_CONNS;
          } else if (get_prefix(rx) == START_POT) {
            cur_pot = get_message(rx);
            state = CHOOSE_POT_RES;
          } else if (rx == END_CONFIG) {
            WAIT_FOR_UART_TX;
            USART5->TDR = CONFIG_SUCCESS;
            state = POST_BOARD_CONFIG;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);

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
            state = IDLE;
          } else { state = ERROR_STATE; }
          break;
        case CHOOSE_CB_CONNS:
          if (rx < NUM_OF_CROSSBAR_CONS)  { crossbar_cons[cur_cb][rx] = 1; }
          else if (rx == END_CB)          { 
            state = UART_CONFIG;
            WAIT_FOR_UART_TX;
            USART5->TDR = RETURN_TO_CONFIG;
          }
          else                            { state = ERROR; }
          break;
        case CHOOSE_POT_RES: 
          if (rx < MAX_POT_RES) {
            pot_resistances[cur_pot] = rx;
            USART5->TDR = RETURN_TO_CONFIG;
            state = UART_CONFIG;
          } else { state = ERROR_STATE; }
        break;
        case POST_BOARD_CONFIG:
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
          
        // TODO: send config back
        // mem_dump(crossbar_cons, pot_resistances);

          // for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
          //   WAIT_FOR_UART_TX;
          //   USART5->TDR = i + '0';
          //   for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
          //     if (crossbar_cons[i][j]) send_num(j);
          //   }
          //   WAIT_FOR_UART_TX;
          //   USART5->TDR = '\n';
          // }
        
          // for (int i = 0; i < NUM_OF_POTS; i++) {
          //   if (pot_resistances[i]) {
          //     send_num(i);
          //     send_num(pot_resistances[i]);
          //     WAIT_FOR_UART_TX;
          //     USART5->TDR = '\n';
          //   }
          // }
          state = IDLE;
        

        break;
        // case ANALOG_CONFIG: break;
        default: break;
      }
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

void setup_blinker(void) {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);
}

// static inline void to_instruction(instruction_t *instruction, uint8_t value) {
//     instruction->prefix  = value >> 5 & 0x7;
//     instruction->message = value & 0x1F;
// }

// static void mem_dump(uint8_t *cb_array, uint8_t *pot_array) {
//   for (int i = 0; i < NUM_OF_CROSSBARS; i++) {
//     USART5->TDR = i + '0';
//     for (int j = 0; j < NUM_OF_CROSSBAR_CONS; j++) {
//       if (cb_array[i][j]) send_num(j);
//     }
//     USART5->TDR = '\n';
//   }

//   for (int i = 0; i < NUM_OF_POTS; i++) {
//     if (pot_array[i]) {
//       send_num(i);
//       send_num(pot_array[i]);
//       USART5->TDR = '\n';
//     }
//   }
// }

static void send_num(uint8_t num) {
  while (num > 0) {
    int digit = num % 10;
    USART5->TDR = digit + '0';
    num = num / 10;
  }
  USART5->TDR = ' ';
}
