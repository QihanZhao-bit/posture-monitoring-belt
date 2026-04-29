#ifndef __TCA9548A_H__
#define __TCA9548A_H__

#include "stm32l4xx_hal.h"

// A0=A1=A2=0 时，TCA9548A 7bit 地址是 0x70
#define TCA9548A_ADDR   (0x70 << 1)   // HAL 用 8bit 地址

HAL_StatusTypeDef TCA9548A_Select(uint8_t channel);

#endif
