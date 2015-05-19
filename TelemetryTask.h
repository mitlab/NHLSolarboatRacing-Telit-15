
#ifndef TELEMETRY_TASK_H
#define TELEMETRY_TASK_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_pinsel.h>
#include <lpc17xx_rtc.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"
#include "ThreadSafeQueue.h"
#include "GM862.h"

/* Defines */

// Telemetry task priority and stack size definition
#define TELEMETRY_TASK_PRIORITY					0
#define TELEMETRY_TASK_STACK_SIZE				2048

//IP 81.169.247.40 for A-boat and T-boat
//#define COMMAND_CENTER_ADDRESS					"81.169.247.40"
#define COMMAND_CENTER_ADDRESS					"77.250.231.177"
//Port 88 for A-boat and 90 for T-boat
#define COMMAND_CENTER_PORT						88

/* Variables */

// Telemetry task stack and unique identifier administration
OS_STK telemetryTaskStack[TELEMETRY_TASK_STACK_SIZE];
OS_TID telemetryTaskId;

QueueStruct_T _sensorDataQueue;

/* Prototypes */

// Telemetry task prototype
void TelemetryTask_Run(void *pdata);

#endif
