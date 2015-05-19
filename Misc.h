
#ifndef MISC_H
#define MISC_H

/* Includes */

#include <stdio.h>
#include <time.h>

#include <lpc17xx_rtc.h>

/* Defines */

#define CRC16_POLYNOMIAL		0x8408

/* Prototypes */

unsigned short CalculateCrc16(char *data_p, unsigned short length);

time_t ConvertRtcToUnixTime(RTC_TIME_Type *rtcTime);

#endif
