/*
 * stm32l47xx_hts221.c
 *
 *  Created on: Jun 2, 2025
 *      Author: admin
 */


#include "stm32l47xx.h"
#include "stm32l47xx_hts221.h"
#include "stm32l47xx_i2c_driver.h"

#include "stm32l47xx_hts221.h"

uint8_t HTS221_ReadWHOAMI(I2C_Handle_t *I2C_Handle)
{
    uint8_t reg = HTS221_WHO_AM_I_REG;
    uint8_t value;

    I2C_MasterSendData(I2C_Handle->pI2Cx, &reg, 1, HTS221_ADDR, 0);
    I2C_MasterReceiveData(I2C_Handle->pI2Cx, &value, 1, HTS221_ADDR, 0);

    return value;
}

void HTS221_PowerOn(I2C_Handle_t *I2C_Handle)
{
    uint8_t data[2];
    data[0] = HTS221_CTRL_REG1;
    data[1] = 0x84;  // PD=1 (bit 7), BDU=1 (bit 2), ODR=1Hz (01)

    I2C_MasterSendData(I2C_Handle->pI2Cx, data, 2, HTS221_ADDR, 0);
}

int16_t HTS221_ReadTemperatureRaw(I2C_Handle_t *I2C_Handle)
{
    uint8_t reg = HTS221_TEMP_OUT_L;
    uint8_t temp_l, temp_h;

    I2C_MasterSendData(I2C_Handle->pI2Cx, &reg, 1, HTS221_ADDR, 0);
    I2C_MasterReceiveData(I2C_Handle->pI2Cx, &temp_l, 1, HTS221_ADDR, 0);

    reg = HTS221_TEMP_OUT_H;
    I2C_MasterSendData(I2C_Handle->pI2Cx, &reg, 1, HTS221_ADDR, 0);
    I2C_MasterReceiveData(I2C_Handle->pI2Cx, &temp_h, 1, HTS221_ADDR, 0);

    return (int16_t)((temp_h << 8) | temp_l);
}

int16_t HTS221_ReadHumidityRaw(I2C_Handle_t *I2C_Handle)
{
    uint8_t reg = HTS221_HUMIDITY_OUT_L;
    uint8_t hum_l, hum_h;

    I2C_MasterSendData(I2C_Handle->pI2Cx, &reg, 1, HTS221_ADDR, 0);
    I2C_MasterReceiveData(I2C_Handle->pI2Cx, &hum_l, 1, HTS221_ADDR, 0);

    reg = HTS221_HUMIDITY_OUT_H;
    I2C_MasterSendData(I2C_Handle->pI2Cx, &reg, 1, HTS221_ADDR, 0);
    I2C_MasterReceiveData(I2C_Handle->pI2Cx, &hum_h, 1, HTS221_ADDR, 0);

    return (int16_t)((hum_h << 8) | hum_l);
}

