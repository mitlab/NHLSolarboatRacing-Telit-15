
/* Name: Storage task
 * Description: Manages logbook on storage (microSD card)
 * Author: Casper Kloppenburg
 */

#ifndef STORAGE_TASK_H
#define STORAGE_TASK_H

/* Includes */

#include <stdio.h>

#include <lpc_types.h>
#include <lpc17xx_can.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_rtc.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"

/* Defines */

// Storage task priority and stack size definition
#define STORAGE_TASK_PRIORITY					0
#define STORAGE_TASK_STACK_SIZE					2048

#define LOG_FILE_NAME							"BOATLOG.TXT"
#define CAN_FILE_NAME							"CANDATA.CAN"

/* Variables */

// Storage task stack and unique identifier administration
OS_STK storageTaskStack[STORAGE_TASK_STACK_SIZE];
OS_TID storageTaskId;

/* Prototypes */

// Storage task prototype
void StorageTask_Run(void *pdata);

void StorageTask_WriteLogFile(char *str);
void StorageTask_WriteCanFile(uint8_t *dat, uint32_t len);

#endif
