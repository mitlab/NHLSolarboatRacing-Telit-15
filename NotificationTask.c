
/* Includes */

#include "NotificationTask.h"

/* Variables */

static OS_FlagID _blinkFlag = E_CREATE_FAIL;
static OS_FlagID _errorFlag = E_CREATE_FAIL;
static OS_FlagID _fatalErrorFlag = E_CREATE_FAIL;

static uint8_t _greenBlink = 0;
static uint8_t _orangeBlink = 0;
static uint8_t _redBlink = 0;

/* Prototypes */

static void NotificationTask_Init();

/* Implementation */

void NotificationTask_Run(void *pdata)
{
	Debug_Send(DM_INFO, "Notification task started.");

	NotificationTask_Init();

	for (;;)
	{
		// If a fatal error occurred, blink red continuously
		if (CoAcceptSingleFlag(_fatalErrorFlag) == E_OK)
		{
			for (;;)
			{
				GPIO_SetValue(LED_RED_PORT, LED_RED_PIN);
				CoTimeDelay(0, 0, 0, 200);
				GPIO_ClearValue(LED_RED_PORT, LED_RED_PIN);
				CoTimeDelay(0, 0, 0, 200);
			}
		}

		// Continuously blink green if system is running
		GPIO_SetValue(LED_GREEN_PORT, LED_GREEN_PIN);
		CoTimeDelay(0, 0, 0, 500);
		GPIO_ClearValue(LED_GREEN_PORT, LED_GREEN_PIN);

		// If an error occurred, blink orange
		if (CoAcceptSingleFlag(_errorFlag) == E_OK)
		{
			uint8_t i;
			for (i = 0; i < 5; i++)
			{
				GPIO_SetValue(LED_ORANGE_PORT, LED_ORANGE_PIN);
				CoTimeDelay(0, 0, 0, 200);
				GPIO_ClearValue(LED_ORANGE_PORT, LED_ORANGE_PIN);
				CoTimeDelay(0, 0, 0, 200);
			}
		}
		else
		{
			CoTimeDelay(0, 0, 2, 0);
		}
	}
}

void NotificationTask_DoBlink()
{
	CoSetFlag(_blinkFlag);
}

void NotificationTask_ReportError()
{
	CoSetFlag(_errorFlag);
}

void NotificationTask_ReportFatalError()
{
	CoSetFlag(_fatalErrorFlag);
}

void NotificationTask_Init()
{
	// Create notification flags
	_blinkFlag = CoCreateFlag(TRUE, FALSE);
	_errorFlag = CoCreateFlag(FALSE, FALSE);
	_fatalErrorFlag = CoCreateFlag(TRUE, FALSE);

	// Set LED pin directions
	GPIO_SetDir(LED_GREEN_PORT, LED_GREEN_PIN, 1);
	GPIO_SetDir(LED_ORANGE_PORT, LED_ORANGE_PIN, 1);
	GPIO_SetDir(LED_RED_PORT, LED_RED_PIN, 1);

	// Turn off all LEDS
	GPIO_ClearValue(LED_GREEN_PORT, LED_GREEN_PIN);
	GPIO_ClearValue(LED_ORANGE_PORT, LED_ORANGE_PIN);
	GPIO_ClearValue(LED_RED_PORT, LED_RED_PIN);
}
