/*
 * stm32l47xx_hts221.h
 *
 *  Created on: Jun 2, 2025
 *      Author: admin
 */

#ifndef INC_STM32L47XX_HTS221_H_
#define INC_STM32L47XX_HTS221_H_

#include"stm32l47xx.h"
#include "stm32l47xx_i2c_driver.h"
#include <stdint.h>

#define HTS221_ADDR             0x5F
#define HTS221_WHO_AM_I_REG     0x0F
#define HTS221_CTRL_REG1        0x20
#define HTS221_TEMP_OUT_L       0x2A
#define HTS221_TEMP_OUT_H       0x2B
#define HTS221_HUMIDITY_OUT_L   0x28
#define HTS221_HUMIDITY_OUT_H   0x29

// Funciones
uint8_t HTS221_ReadWHOAMI(I2C_Handle_t *I2C_Handle);
void HTS221_PowerOn(I2C_Handle_t *I2C_Handle);
int16_t HTS221_ReadTemperatureRaw(I2C_Handle_t *I2C_Handle);
int16_t HTS221_ReadHumidityRaw(I2C_Handle_t *I2C_Handle);





#endif /* INC_STM32L47XX_HTS221_H_ */
