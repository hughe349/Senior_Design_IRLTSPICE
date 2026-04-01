#ifndef __UART_BYTES_H__
#define __UART_BYTES_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

// irlc -> micro
#define START_CONFIG    0b10000000
#define END_CONFIG      0b10000001
#define START_CB        0b10000010
#define START_CB        0b110  // last 5 bits are 0-8 to decide which crossbar to config
#define END_CB          0b10000011
#define START_POT       0b101  // last 5 bits are 0-23 to decide which pot to config
#define END_POT         0b10000101
#define RESET_CONFIG    0b10011111

// micro -> irlc
#define READY_TO_START  0b10000110
#define SUCCESS         0b10001000
#define ERROR           0b11111111

typedef struct _instruction {
  uint8_t message : 5;
  uint8_t prefix  : 3;
} instruction_t;

#ifdef __cplusplus
}
#endif

#endif /* __UART_BYTES_H__ */
