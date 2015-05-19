
#ifndef CAN_TASK_H
#define CAN_TASK_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_can.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"
#include "SensorDataManager.h"

/* Defines */

// CAN task priority and stack size definition
#define CAN_TASK_PRIORITY						0
#define CAN_TASK_STACK_SIZE						2048

/* Variables */

// CAN task stack and unique identifier administration
OS_STK canTaskStack[CAN_TASK_STACK_SIZE];
OS_TID canTaskId;

/* Prototypes */

// CAN task prototype
void CanTask_Run(void *pdata);

#endif
