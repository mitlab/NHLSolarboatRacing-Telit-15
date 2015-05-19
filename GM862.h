
/* Name: GM862 modem driver
 * Description: Minimalistic send-only driver to interface with a GM862 modem
 * Author: Casper Kloppenburg
 */

#ifndef GM862_H
#define GM862_H

/* Includes */

#include <lpc_types.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_nvic.h>
#include <lpc17xx_pinsel.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

#include "Debug.h"

/* Defines */

#define UART_TIMEOUT_INFINITE	(0xFFFFFFFF)
#define UART_TIMEOUT_DEFAULT	(300)

/* Enums */

typedef enum {
	GM862_REPORT_NOT_REGISTERED_NOT_SEARCHING	= 0,
	GM862_REPORT_REGISTERED_HOME_NETWORK		= 1,
	GM862_REPORT_NOT_REGISTERED_SEARCHING		= 2,
	GM862_REPORT_REGISTRATION_DENIED			= 3,
	GM862_REPORT_UNKNOWN						= 4,
	GM862_REPORT_REGISTERED_ROAMING				= 5
} GM862_NETREG_REPORT;

/* Structs */

typedef struct {

	uint32_t latitude;
	uint32_t longitude;
	uint16_t sog;
	uint8_t nsat;
	uint8_t fix;

} GM862_GPS_DATA;

/* Prototypes */

BOOL GM862_Init();
BOOL GM862_Shutdown();
void GM862_SetTimeout(uint16_t timeout);
GM862_NETREG_REPORT GM862_GetNetworkRegistrationReport();
BOOL GM862_SetNetworkRegistration(BOOL doRegister);
int8_t GM862_GetSignalQuality();
BOOL GM862_SendMessage(const char *da, const char *message); // (+CMGS)
BOOL GM862_SetGprs(BOOL enabled);
int8_t GM862_GetGprs();
BOOL GM862_OpenSocket(const char *address, uint16_t port);
BOOL GM862_SendThroughSocket(uint8_t *packet, uint16_t packetLength);
BOOL GM862_CloseSocket();
BOOL GM862_GetSocketStatus();
BOOL GM862_GpsGetPosition(GM862_GPS_DATA *gpsData);

#endif
