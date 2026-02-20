#ifndef __MCP4142_H__
#define __MCP4142_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"
#include <stdint.h>

typedef enum MCP4241_addr_tt {
	V_WIPER_0 	= 0x0,
	V_WIPER_1 	= 0x1,
	NV_WIPER_0 	= 0x2,
	NV_WIPER_1	= 0x3,
	TCON_REG	= 0x4,
	STATUS_REG	= 0x5
} MCP4241_addr_t;

typedef enum MCP4241_cmd_tt {
	WRITE_CMD	= 0x0,
	INCR_CMD	= 0x1,
	DECR_CMD	= 0x2,
	READ_CMD	= 0x3
} MCP4241_cmd_t;

void incr_resistance(MCP4241_addr_t, SPI_HandleTypeDef *hspi);
void decr_resistance(MCP4241_addr_t, SPI_HandleTypeDef *hspi);

void set_resistance(MCP4241_addr_t, uint8_t, SPI_HandleTypeDef *hspi);
uint8_t read_register(MCP4241_addr_t, SPI_HandleTypeDef *hspi);
void write_register(MCP4241_addr_t, uint16_t, SPI_HandleTypeDef *hspi);

#ifdef __cplusplus
}
#endif

#endif /* __MCP4142_H__ */