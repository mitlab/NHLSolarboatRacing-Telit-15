
#ifndef DEBUG_H
#define DEBUG_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_pinsel.h>
#include <lpc17xx_rtc.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

/* Defines */

// Used UART for debug output: LPC_UART0 (bus D) or LPC_UART3 (RS232)
#define DEBUG_UART			LPC_UART3

/* Enums */

typedef enum {
	DM_INFO			= 0,	// Information about current task
	DM_ERROR		= 1,	// An error occurred but process continues
	DM_FATAL_ERROR	= 2		// A fatal error occurred and process halts
} DEBUG_MESSAGE_TYPE;

/* Prototypes */

void Debug_Init();
void Debug_Send(DEBUG_MESSAGE_TYPE type, const char *str);

#endif
