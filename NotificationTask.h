
#ifndef NOTIFICATION_TASK_H
#define NOTIFICATION_TASK_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"

/* Defines */

#define LED_GREEN_PORT		0
#define LED_GREEN_PIN		_BIT(9)
#define LED_ORANGE_PORT		0
#define LED_ORANGE_PIN		_BIT(7)
#define LED_RED_PORT		0
#define LED_RED_PIN			_BIT(1)

// Notification task priority and stack size definition
#define NOTIFICATION_TASK_PRIORITY				0
#define NOTIFICATION_TASK_STACK_SIZE			512

/* Variables */

// Notification task stack and unique identifier administration
OS_STK notificationTaskStack[NOTIFICATION_TASK_STACK_SIZE];
OS_TID notificationTaskId;

/* Prototypes */

// Notification task prototype
void NotificationTask_Run(void *pdata);

void NotificationTask_DoBlink();
void NotificationTask_ReportError();
void NotificationTask_ReportFatalError();

#endif
