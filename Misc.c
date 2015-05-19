
/* Includes */

#include "Misc.h"

/* Implementation */

// TODO: http://www8.cs.umu.se/~isak/snippets/crc-16.c
unsigned short CalculateCrc16(char *data_p, unsigned short length)
{
    unsigned char i;
    unsigned int data;
    unsigned int crc = 0xffff;

    if (length == 0)
    	return (~crc);

    do
    {
    	for (i = 0, data = (unsigned int)0xff & *data_p++;
    		i < 8;
    		i++, data >>= 1)
    	{
    		if ((crc & 0x0001) ^ (data & 0x0001))
    			crc = (crc >> 1) ^ CRC16_POLYNOMIAL;
    		else  crc >>= 1;
    	}
    } while (--length);

    crc = ~crc;
    data = crc;
    crc = (crc << 8) | (data >> 8 & 0xff);

    return (crc);
}

time_t ConvertRtcToUnixTime(RTC_TIME_Type *rtcTime)
{
	struct tm tmTime;

	RTC_GetFullTime(LPC_RTC, rtcTime);

	tmTime.tm_sec = rtcTime->SEC;
	tmTime.tm_min = rtcTime->MIN;
	tmTime.tm_hour = rtcTime->HOUR;
	tmTime.tm_mday = rtcTime->DOM;
	tmTime.tm_mon = rtcTime->MONTH - 1;
	tmTime.tm_year = rtcTime->YEAR - 1900;
	tmTime.tm_wday = rtcTime->DOW;
	tmTime.tm_yday = rtcTime->DOY;
	tmTime.tm_isdst = -1;

	return mktime(&tmTime);
}
