
/* Includes */

#include "TelemetryTask.h"

/* Prototypes */

static BOOL TelemetryTask_SendSensorData();

/* Variables */

static uint8_t _packetBuffer[256];

/* Implementation */

void TelemetryTask_Run(void *pdata)
{
	uint8_t setGprsTries;
	uint8_t openSocketTries;

	Debug_Send(DM_INFO, "Telemetry task started.");

	/*for (;;)
	{
		CoTimeDelay(0, 0, 1, 0);
		uint16_t t = SensorDataManager_GetTables(_packetBuffer, sizeof(_packetBuffer));

		char b[26];
		sprintf(b, "%u\r\n", t);
		Debug_Send(DM_INFO, b);
	}

	while (1);*/

TelitInitialize:

	// Try to initialize Telit GM862
	if (!GM862_Init())
	{
		Debug_Send(DM_ERROR, "Modem not responsive.");
		goto TelitInitialize;
	}

	Debug_Send(DM_INFO, "Modem responsive and initialized.");

	setGprsTries = 15;

TelitSetGPRS:

	// Force the Telit862 to register to the network
	if (GM862_GetNetworkRegistrationReport() == GM862_REPORT_NOT_REGISTERED_NOT_SEARCHING)
	{
		GM862_SetNetworkRegistration(TRUE);
	}

	Debug_Send(DM_ERROR, "Activating GPRS context.");

	// Try to enable GPRS
	while (!GM862_SetGprs(TRUE))
	{
		if (setGprsTries-- == 0)
		{
			Debug_Send(DM_ERROR, "GPRS context activation failed after several attempts, resetting modem.");
			GM862_Shutdown();
			goto TelitInitialize;
		}

		Debug_Send(DM_ERROR, "GPRS context activation failed, retrying.");

		CoTimeDelay(0, 0, 4, 0);
	}

	Debug_Send(DM_INFO, "GPRS context activation successful.");

TelitOpenSocket:

	openSocketTries = 25;

	Debug_Send(DM_INFO, "Opening socket to command center (" COMMAND_CENTER_ADDRESS ").");

	// Try to connect to command center
	while (!GM862_OpenSocket(COMMAND_CENTER_ADDRESS, COMMAND_CENTER_PORT))
	{
		if (openSocketTries-- == 0)
		{
			Debug_Send(DM_ERROR, "Opening socket failed after several attempts, resetting modem.");
			GM862_Shutdown();
			goto TelitInitialize;
		}

		if (GM862_GetGprs() != 1)
		{
			Debug_Send(DM_ERROR, "GPRS context activation error occurred.");
			setGprsTries = 5;
			goto TelitSetGPRS;
		}

		Debug_Send(DM_ERROR, "Opening socket failed, retrying.");

		CoTimeDelay(0, 0, 4, 0);
	}

	for (;;)
	{
		if (!TelemetryTask_SendSensorData())
		{
			Debug_Send(DM_ERROR, "Closing socket.");
			GM862_CloseSocket();
			goto TelitOpenSocket;
		}

		CoTimeDelay(0, 0, 1, 500);


	}
}

BOOL TelemetryTask_SendSensorData()
{
	// TESTING: Very error prone code, please test carefully

	uint8_t *bufferPos = &_packetBuffer;

	// Insert sync/sof byte
	*bufferPos++ = '$';

	// Copy available data tables (reserve space for size (8 bit), id (32 bit), timestamp (32 bit))
	uint16_t tablesSize = SensorDataManager_GetTables(
			bufferPos + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t), 128);
	if (tablesSize == 0)
	{
		// No data tables are ready
		Debug_Send(DM_INFO, "No sensor data to send.");
		return TRUE;
	}

	// Insert size
	uint8_t totalSize = sizeof(uint32_t) + sizeof(uint32_t) + tablesSize + sizeof(uint16_t);
	*((uint8_t *)bufferPos) = totalSize;
	bufferPos += sizeof(uint8_t);

	// Insert and update packet id
	uint32_t packetId = RTC_ReadGPREG(LPC_RTC, 0); // read from RTC RAM
	RTC_WriteGPREG(LPC_RTC, 0, packetId + 1); // write back
	*((uint32_t *)bufferPos) = packetId;
	bufferPos += sizeof(uint32_t);

	// Insert timestamp
	RTC_TIME_Type time;
	RTC_GetFullTime(LPC_RTC, &time);
	*((uint32_t *)bufferPos) = ConvertRtcToUnixTime(&time);
	bufferPos += sizeof(uint32_t);

	// Skip data tables
	bufferPos += tablesSize;

	// Calculate checksum of packet excluding sync/sof byte
	uint16_t checksum = CalculateCrc16(_packetBuffer + 1, bufferPos - _packetBuffer - 1);

	// Insert checksum at end
	*((uint16_t *)bufferPos) = checksum;
	bufferPos += sizeof(uint16_t);

	/*int i;
	Debug_Send(DM_INFO, "Sending:");
	for (i = 0; i < bufferPos - _packetBuffer; i++)
	{
		char buffer[16];
		sprintf(buffer, "%X", _packetBuffer[i]);
		Debug_Send(DM_INFO, buffer);
	}*/

	// Sending packet with Telit GM862 through open socket
	if (GM862_SendThroughSocket(_packetBuffer, bufferPos - _packetBuffer))
	{
		Debug_Send(DM_INFO, "Sensor data successful sent.");
		return TRUE;
	}
	else
	{
		Debug_Send(DM_ERROR, "Error sending sensor data.");
		return FALSE;
	}
}
