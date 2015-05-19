
#ifndef SENSOR_DATA_MANAGER_H
#define SENSOR_DATA_MANAGER_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_can.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"
#include "GM862.h"

/* Prototypes */

BOOL SensorDataManager_Init();
void SensorDataManager_PutCanData(CAN_MSG_Type *msg);
uint16_t SensorDataManager_GetTables(uint8_t *tableBuffer, uint16_t bufferSize);

#endif
