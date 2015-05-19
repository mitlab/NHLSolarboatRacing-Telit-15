
/* Includes */

#include "Debug.h"

/* Variables */

static OS_MutexID _mutex;

/* Implementation */

void Debug_Init()
{
	PINSEL_CFG_Type pinConfig;

	if (DEBUG_UART == LPC_UART0)
	{
		// UART0 pin configuration (bus D)
		pinConfig.Funcnum = PINSEL_FUNC_1;
		pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
		pinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
		pinConfig.Pinnum = 2;
		pinConfig.Portnum = 0;
		PINSEL_ConfigPin(&pinConfig); // TX
		pinConfig.Pinnum = 3;
		PINSEL_ConfigPin(&pinConfig); // RX
	}
	else if (DEBUG_UART == LPC_UART3)
	{
		// UART3 pin configuration (RS232)
		PINSEL_CFG_Type pinConfig;
		pinConfig.Funcnum = PINSEL_FUNC_3;
		pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
		pinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
		pinConfig.Pinnum = 28;
		pinConfig.Portnum = 4;
		PINSEL_ConfigPin(&pinConfig); // TX
		pinConfig.Pinnum = 29;
		PINSEL_ConfigPin(&pinConfig); // RX
	}

	UART_CFG_Type uartConfigStruct;
	UART_FIFO_CFG_Type uartFifoConfigStruct;

	// Initialize UART configuration structure to default settings:
	// Baudrate = 9600bps
	// 8 data bit
	// 1 Stop bit
	// None parity
	UART_ConfigStructInit(&uartConfigStruct);

	// Set baudrate
	uartConfigStruct.Baud_rate = 115200;

	// Initialize UART
	UART_Init(DEBUG_UART, &uartConfigStruct);

	// Initialize FIFO configuration structure to default settings:
	// - FIFO_DMAMode = DISABLE
	// - FIFO_Level = UART_FIFO_TRGLEV0
	// - FIFO_ResetRxBuf = ENABLE
	// - FIFO_ResetTxBuf = ENABLE
	// - FIFO_State = ENABLE
	UART_FIFOConfigStructInit(&uartFifoConfigStruct);

	// Initialize FIFO for UART
	UART_FIFOConfig(DEBUG_UART, &uartFifoConfigStruct);

	// Enable UART transmitter
	UART_TxCmd(DEBUG_UART, ENABLE);

	_mutex = CoCreateMutex();
}

void Debug_Send(DEBUG_MESSAGE_TYPE type, const char *str)
{
	/* FEATURED

	switch (type)
	{
	case DM_ERROR:

		// A error occurred
		NotificationTask_ReportError();
		break;

	case DM_FATAL_ERROR:

		// A fatal error occurred
		NotificationTask_ReportFatalError();
		break;
	}

	*/

	CoEnterMutexSection(_mutex); // TODO: catch result

	// Get time from RTC
	RTC_TIME_Type time;
	RTC_GetFullTime(LPC_RTC, &time);

	// Create time prefix
	char prefix[20];
	sprintf(prefix, "%02u:%02u:%02u - ", time.HOUR, time.MIN, time.SEC);

	// Send prefix and debug message
	UART_Send(DEBUG_UART, (uint8_t *)prefix, strlen(prefix), BLOCKING);
	UART_Send(DEBUG_UART, (uint8_t *)str, strlen(str), BLOCKING);
	UART_Send(DEBUG_UART, (uint8_t *)"\r\n", sizeof("\r\n"), BLOCKING);

	CoLeaveMutexSection(_mutex);
}
