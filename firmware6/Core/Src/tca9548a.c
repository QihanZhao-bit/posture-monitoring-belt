#include "tca9548a.h"

extern I2C_HandleTypeDef hi2c1;

// 选择 TCA9548A 的某个通道（0~7）
HAL_StatusTypeDef TCA9548A_Select(uint8_t channel)
{
    if (channel > 7) return HAL_ERROR;

    uint8_t data = 1 << channel;   // bit0~bit7 对应通道 0~7

    return HAL_I2C_Master_Transmit(&hi2c1,
                                   TCA9548A_ADDR,
                                   &data,
                                   1,
                                   100);
}
