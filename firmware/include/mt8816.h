#ifndef __MT8816_H__
#define __MT8816_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void enable_connection(uint8_t);
void disable_connection(uint8_t);

#ifdef __cplusplus
}
#endif

#endif /* __MT8816_H__ */