
/* Includes */

#include <stdio.h>

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

#include "TelemetryTask.h"
#include "CanTask.h"
#include "StorageTask.h"
#include "NotificationTask.h"

/* Defines */

#define FIRMWARE_VERSION				110
#define FIRMWARE_VERSION_VERBOSE		"ver. R1.1_T"

/* Entry point */
int main()
{
	// Enable GPIO clock
	CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCGPIO, ENABLE);

	// Initialize RTC
	RTC_Init(LPC_RTC);
	RTC_Cmd(LPC_RTC, ENABLE);

	// Initialize CooCox CoOS
	CoInitOS();

	// Initialize debug
	Debug_Init();

	Debug_Send(DM_INFO, "/********* NHL Solarboat Mainboard 2012 (" FIRMWARE_VERSION_VERBOSE ") *********/");

	// Initialize telemetry task
	telemetryTaskId = CoCreateTask(
			TelemetryTask_Run, (void *)0,
			TELEMETRY_TASK_PRIORITY,
			&telemetryTaskStack[TELEMETRY_TASK_STACK_SIZE - 1],
			TELEMETRY_TASK_STACK_SIZE);
	if (telemetryTaskId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Initialization of telemetry task failed.");
		while (1); // Enter panic state
	}

	// Initialize CAN task
	canTaskId = CoCreateTask(
			CanTask_Run, (void *)0,
			CAN_TASK_PRIORITY,
			&canTaskStack[CAN_TASK_STACK_SIZE - 1],
			CAN_TASK_STACK_SIZE);
	if (canTaskId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Initialization of CAN task failed.");
		while (1); // Enter panic state
	}

	// Initialize storage task
	/*storageTaskId = CoCreateTask(
			StorageTask_Run, (void *)0,
			STORAGE_TASK_PRIORITY,
			&storageTaskStack[STORAGE_TASK_STACK_SIZE - 1],
			STORAGE_TASK_STACK_SIZE);
	if (storageTaskId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Initialization of storage task failed.");
		while (1); // Enter panic state
	}*/

	// Initialize notification task
	/*notificationTaskId = CoCreateTask(
			NotificationTask_Run, (void *)0,
			NOTIFICATION_TASK_PRIORITY,
			&notificationTaskStack[NOTIFICATION_TASK_STACK_SIZE - 1],
			NOTIFICATION_TASK_STACK_SIZE);
	if (notificationTaskId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Initialization of notification task failed.");
		while (1); // Enter panic state
	}*/

	Debug_Send(DM_INFO, "All tasks successful initialized.");

	// Startup OS, this function never returns
	CoStartOS();
	
	return 0;
}
