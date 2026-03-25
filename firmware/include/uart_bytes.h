#ifndef __UART_BYTES_H__
#define __UART_BYTES_H__

#ifdef __cplusplus
extern "C"
{
#endif

// irlc -> micro
#define START_CONFIG    0b10000000
#define END_CONFIG      0b10000001
#define START_CB        0b10000010
#define END_CB          0b10000011
#define START_POT       0b10000100
#define END_POT         0b10000101

// micro -> irlc
#define READY_TO_START  0b10000110
#define SUCCESS         0b10001000
#define ERROR           0b11111111

#ifdef __cplusplus
}
#endif

#endif /* __UART_BYTES_H__ */
