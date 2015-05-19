
/* Includes */

#include "SensorDataManager.h"

/* Defines */

// CAN bus message identifiers
#define MESSAGE_PGN_BMS						0x302
#define MESSAGE_PGN_BMS_TEMP				0x402
#define MESSAGE_PGN_MPPT1A					0x185
#define MESSAGE_PGN_MPPT1B					0x285
#define MESSAGE_PGN_MPPT2A					0x186
#define MESSAGE_PGN_MPPT2B					0x286
#define MESSAGE_PGN_MPPT3A					0x187
#define MESSAGE_PGN_MPPT3B					0x287
#define MESSAGE_PGN_MPPT4A					0x188
#define MESSAGE_PGN_MPPT4B					0x288
#define MESSAGE_PGN_TEMPERATURE				0x18FD	// not correct, needs to be 0xFD12 (Tim's fault)

// Table full mask definitions
#define TABLE_BMS_FULL_MASK					0b0000000000011111
#define TABLE_MPPT_FULL_MASK				0b0000000011111111
#define TABLE_TEMPERATURE_FULL_MASK			0b0000000001111111

/* Enumerators */

typedef enum {

	TABLE_ID_BMS							= 0x01,
	TABLE_ID_TRACKING						= 0x02,
	TABLE_ID_MPPT 							= 0x03,
	TABLE_ID_TEMPERATURE					= 0x04

} TABLE_ID;

typedef enum {

	BMS_VOLTAGE								= 0,
	BMS_CURRENT_IN							= 1,
	BMS_CURRENT_OUT							= 2,
	BMS_STATE_OF_CHARGE						= 3,
	BMS_TEMPERATURE							= 4,

	MPPT1A									= 0,
	MPPT1B									= 1,
	MPPT2A									= 2,
	MPPT2B									= 3,
	MPPT3A									= 4,
	MPPT3B									= 5,
	MPPT4A									= 6,
	MPPT4B									= 7,

	TEMPERATURE_SENSOR1						= 0,
	TEMPERATURE_SENSOR2						= 1,
	TEMPERATURE_SENSOR3						= 2,
	TEMPERATURE_SENSOR4						= 3,
	TEMPERATURE_SENSOR5						= 4,
	TEMPERATURE_SENSOR6						= 5,
	TEMPERATURE_REFERENCE					= 6

} DATA_TYPE;

typedef enum {

	SI_BMS_VOLTAGE 							= 1,
	SI_BMS_CURRENT							= 2,
	SI_BMS_CURRENT_CHARGE					= 3,
	SI_BMS_CURRENT_DISCHARGE				= 4,
	SI_BMS_STATE_OF_CHARGE					= 5,
	SI_BMS_TIME_TO_GO						= 7,

	SI_BMS_CELL_TEMP_HIGH					= 9,
	SI_BMS_CELL_TEMP_LOW					= 11,
	SI_BMS_CELL_VOLTAGE_HIGH				= 12,
	SI_BMS_CELL_VOLTAGE_LOW					= 13,
	SI_BMS_STATE							= 14,
	SI_BMS_TEMP_COLLECTION					= 15
} SUB_INDEX;

/* Structs */

typedef struct {

	uint16_t voltage __attribute__ ((__packed__));
	int16_t currentIn __attribute__ ((__packed__));
	int16_t currentOut __attribute__ ((__packed__));
	uint8_t stateOfCharge __attribute__ ((__packed__));
	int8_t temperature __attribute__ ((__packed__));

} TableBms_t;

typedef struct {

	float currentIn1 __attribute__ ((__packed__));
	float currentIn2 __attribute__ ((__packed__));
	float currentIn3 __attribute__ ((__packed__));
	float currentIn4 __attribute__ ((__packed__));
	float voltageIn1 __attribute__ ((__packed__));
	float voltageIn2 __attribute__ ((__packed__));
	float voltageIn3 __attribute__ ((__packed__));
	float voltageIn4 __attribute__ ((__packed__));
	float voltageOut1 __attribute__ ((__packed__));
	float voltageOut2 __attribute__ ((__packed__));
	float voltageOut3 __attribute__ ((__packed__));
	float voltageOut4 __attribute__ ((__packed__));
	float powerIn1 __attribute__ ((__packed));
	float powerIn2 __attribute__ ((__packed));
	float powerIn3 __attribute__ ((__packed));
	float powerIn4 __attribute__ ((__packed));

} TableMppt_t;

typedef struct {

	uint8_t sensor1 __attribute__ ((__packed__));
	uint8_t sensor2 __attribute__ ((__packed__));
	uint8_t sensor3 __attribute__ ((__packed__));
	uint8_t sensor4 __attribute__ ((__packed__));
	uint8_t sensor5 __attribute__ ((__packed__));
	uint8_t sensor6 __attribute__ ((__packed__));
	uint8_t reference __attribute__ ((__packed__));

} TableTemperature_t;

/* Variables */

static OS_MutexID _dataMutexId;

static TableBms_t _tableBms;
static TableMppt_t _tableMppt;
static TableTemperature_t _tableTemperature;

static uint16_t _bmsDataReady;
static uint16_t _mpptDataReady;
static uint16_t _temperatureDataReady;

/* Implementation */

BOOL SensorDataManager_Init()
{
	// Create mutex
	_dataMutexId = CoCreateMutex();
	if (_dataMutexId == E_CREATE_FAIL)
	{
		return FALSE;
	}

	_bmsDataReady = 0;
	_mpptDataReady = 0;

	return TRUE;
}

void SensorDataManager_PutCanData(CAN_MSG_Type *msg)
{
	if (CoEnterMutexSection(_dataMutexId) != E_OK)
		return;

	if(msg->format == STD_ID_FORMAT)
	{
		switch (msg->id)
		{
			case MESSAGE_PGN_BMS://CAN data with COB-ID 0x302
			case MESSAGE_PGN_BMS_TEMP://CAN data with COB-ID 0x402
				switch(msg->dataA[3]) //SUB_INDEX
				{
					case SI_BMS_VOLTAGE:
						_tableBms.voltage = ((uint16_t)msg->dataB[1] << 8) | msg->dataB[0]; // battery voltage
						_bmsDataReady |= _BIT(BMS_VOLTAGE);
						break;
					case SI_BMS_CURRENT_CHARGE:
						_tableBms.currentIn = ((int16_t)msg->dataB[1] << 8) | msg->dataB[0]; // charge current
						_bmsDataReady |= _BIT(BMS_CURRENT_IN);
						break;
					case SI_BMS_CURRENT_DISCHARGE:
						_tableBms.currentOut = ((int16_t)msg->dataB[1] << 8) | msg->dataB[0]; // discharge current
						_bmsDataReady |= _BIT(BMS_CURRENT_OUT);
						break;
					case SI_BMS_STATE_OF_CHARGE:
						_tableBms.stateOfCharge = msg->dataB[0]; // SOC (%)
						_bmsDataReady |= _BIT(BMS_STATE_OF_CHARGE);
						break;
					case SI_BMS_CELL_TEMP_HIGH:
						_tableBms.temperature = msg->dataB[0]; // highest cell temperature
						_bmsDataReady |= _BIT(BMS_TEMPERATURE);
						break;
				}

				break;

			case MESSAGE_PGN_MPPT1A:
				_tableMppt.currentIn1 = *( (float*)(msg->dataA));
				_tableMppt.voltageIn1 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT1A);
				break;
			case MESSAGE_PGN_MPPT1B:
				_tableMppt.voltageOut1 = *( (float*)(msg->dataA));
				_tableMppt.powerIn1 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT1B);
				break;
			case MESSAGE_PGN_MPPT2A:
				_tableMppt.currentIn2 = *( (float*)(msg->dataA));
				_tableMppt.voltageIn2 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT2A);
				break;
			case MESSAGE_PGN_MPPT2B:
				_tableMppt.voltageOut2 = *( (float*)(msg->dataA));
				_tableMppt.powerIn2 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT2B);
				break;
			case MESSAGE_PGN_MPPT3A:
				_tableMppt.currentIn3 = *( (float*)(msg->dataA));
				_tableMppt.voltageIn3 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT3A);
				break;
			case MESSAGE_PGN_MPPT3B:
				_tableMppt.voltageOut3 = *( (float*)(msg->dataA));
				_tableMppt.powerIn3 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT3B);
				break;
			case MESSAGE_PGN_MPPT4A:
				_tableMppt.currentIn4 = *( (float*)(msg->dataA));
				_tableMppt.voltageIn4 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT4A);
				break;
			case MESSAGE_PGN_MPPT4B:
				_tableMppt.voltageOut4 = *( (float*)(msg->dataA));
				_tableMppt.powerIn4 = *( (float*)(msg->dataB));
				_mpptDataReady |= _BIT(MPPT4B);
				break;
		}
	}
	else if(msg->format == EXT_ID_FORMAT)
	{
		switch ((msg->id & 0x00FFFFFF) >> 8)
		{
			case MESSAGE_PGN_TEMPERATURE:

				_tableTemperature.sensor1 = msg->dataA[0];
				_tableTemperature.sensor2 = msg->dataA[1];
				_tableTemperature.sensor3 = msg->dataA[2];
				_tableTemperature.sensor4 = msg->dataA[3];
				_tableTemperature.sensor5 = msg->dataB[0];
				_tableTemperature.sensor6 = msg->dataB[1];
				_tableTemperature.reference = msg->dataB[2];

				_temperatureDataReady |= _BIT(TEMPERATURE_SENSOR1) | _BIT(TEMPERATURE_SENSOR2) | _BIT(TEMPERATURE_SENSOR3) |
					_BIT(TEMPERATURE_SENSOR4) | _BIT(TEMPERATURE_SENSOR5) | _BIT(TEMPERATURE_REFERENCE);

				break;
		}
	}


	CoLeaveMutexSection(_dataMutexId);
}

uint16_t SensorDataManager_GetTables(uint8_t *tableBuffer, uint16_t bufferSize)
{
	uint16_t bufferUsed = 0;

	CoEnterMutexSection(_dataMutexId);

	GM862_GPS_DATA gpsData;
	if (GM862_GpsGetPosition(&gpsData))
	{
		if (bufferSize - bufferUsed >= sizeof(uint8_t) + sizeof(GM862_GPS_DATA))
		{
			Debug_Send(DM_INFO, "GPS data collected.");

			tableBuffer[bufferUsed++] = TABLE_ID_TRACKING;

			memcpy(tableBuffer + bufferUsed, &gpsData, sizeof(GM862_GPS_DATA));
			bufferUsed += sizeof(GM862_GPS_DATA);
		}
	}

	if (_bmsDataReady & TABLE_BMS_FULL_MASK)
	{
		if (bufferSize - bufferUsed >= sizeof(uint8_t) + sizeof(TableBms_t))
		{
			Debug_Send(DM_INFO, "BMS data collected.");

			tableBuffer[bufferUsed++] = TABLE_ID_BMS;

			memcpy(tableBuffer + bufferUsed, &_tableBms, sizeof(TableBms_t));
			bufferUsed += sizeof(TableBms_t);

			_bmsDataReady = 0;
		}
	}

	if (_mpptDataReady & TABLE_MPPT_FULL_MASK)
	{
		if (bufferSize - bufferUsed >= sizeof(uint8_t) + sizeof(TableMppt_t))
		{
			Debug_Send(DM_INFO, "MPPT data collected.");

			tableBuffer[bufferUsed++] = TABLE_ID_MPPT;

			memcpy(tableBuffer + bufferUsed, &_tableMppt, sizeof(TableMppt_t));
			bufferUsed += sizeof(TableMppt_t);

			_bmsDataReady = 0;
		}
	}

	if (_temperatureDataReady & TABLE_TEMPERATURE_FULL_MASK)
	{
		if (bufferSize - bufferUsed >= sizeof(uint8_t) + sizeof(TableTemperature_t))
		{
			Debug_Send(DM_INFO, "Temperature data collected.");

			tableBuffer[bufferUsed++] = TABLE_ID_TEMPERATURE;

			memcpy(tableBuffer + bufferUsed, &_tableTemperature, sizeof(TableTemperature_t));
			bufferUsed += sizeof(TableTemperature_t);

			_bmsDataReady = 0;
		}
	}

	CoLeaveMutexSection(_dataMutexId);

	return bufferUsed;
}
