
/* Name: GM862 modem driver
 * Description: Minimalistic send-only driver to interface with a GM862 modem
 * Author: Casper Kloppenburg
 */

/* Includes */

#include <stdarg.h>

#include "GM862.h"

/* Defines */

// GPIO pin/port definitions
#define GM862_SHUTDOWN_PORT			(0)
#define GM862_SHUTDOWN_PIN			(6)
#define GM862_RESET_PORT			(2)
#define GM862_RESET_PIN				(11)
#define GM862_ENABLE_PORT			(2)
#define GM862_ENABLE_PIN			(12)
#define GM862_PWRMON_PORT			(2)
#define GM862_PWRMON_PIN			(8)
#define GM862_DCD_PORT				(2)
#define GM862_DCD_PIN				(3)
#define GM862_DTR_PORT				(2)
#define GM862_DTR_PIN				(5)

#define UART_RING_BUFFER_SIZE		(256)
#define UART_TIMEOUT_OCCURRED		(0xFF00)

/* Enums */

typedef enum {
	GM862_RESULT_OK					= 0,
	GM862_RESULT_CONNECT			= 1,
	GM862_RESULT_NO_CARRIER			= 3,
	GM862_RESULT_ERROR				= 4,
	GM862_RESULT_UNKNOWN			= 0xFF
} GM862_RESULT;

/* Structs */

typedef struct {

	uint8_t buffer[UART_RING_BUFFER_SIZE];
	BOOL rxBufferIsFull;
	uint32_t rxBufferHead;
	uint32_t rxBufferTail;

} UART_RING_BUFFER_T;

/* Prototypes */

static void GM862_SendAtFormat(const char *format, ...);
static void GM862_SendAt(const char *command);
static GM862_RESULT GM862_GetResult();
static int GM862_GetResponseFormat(const char *format, ...);
static BOOL GM862_GetResponse(char *respBuffer, uint16_t buffSize);
static BOOL GM862_UART_SendByte(uint8_t b);
static uint16_t GM862_UART_ReceiveByte();
static uint32_t GM862_UART_Send(uint8_t *txbuf, uint32_t buflen);
static uint32_t GM862_UART_Receive(uint8_t *rxbuf, uint32_t buflen);

/* Variables */

static UART_RING_BUFFER_T _ringBuffer; // UART RX ring buffer
static uint16_t _timeout; // UART RX timeout in CoOS ticks
static char _globalBuffer[128]; // global command/response buffer

/* Implementation */

/*
 * @brief		Initialization of this driver, used peripherals, and Telit GM862 modem
 * @return		If modem is responsive and ready to operate TRUE is returned, else FALSE
 */
BOOL GM862_Init()
{
	PINSEL_CFG_Type pinConfig;

	// Set ring buffer to default state
	_ringBuffer.rxBufferIsFull = FALSE;
	_ringBuffer.rxBufferHead = 0;
	_ringBuffer.rxBufferTail = 0;

	// Set timeout to default state
	_timeout = UART_TIMEOUT_DEFAULT;

	// UART1 pin configuration
	pinConfig.Funcnum = PINSEL_FUNC_2;
	pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
	pinConfig.Pinnum = PINSEL_PIN_0;
	pinConfig.Portnum = PINSEL_PORT_2;
	PINSEL_ConfigPin(&pinConfig); // TX
	pinConfig.Pinnum = PINSEL_PIN_1;
	PINSEL_ConfigPin(&pinConfig); // RX

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
	UART_Init((LPC_UART_TypeDef *)LPC_UART1, &uartConfigStruct);

	// Initialize FIFO configuration structure to default settings:
	// - FIFO_DMAMode = DISABLE
	// - FIFO_Level = UART_FIFO_TRGLEV0
	// - FIFO_ResetRxBuf = ENABLE
	// - FIFO_ResetTxBuf = ENABLE
	// - FIFO_State = ENABLE
	UART_FIFOConfigStructInit(&uartFifoConfigStruct);

	// Initialize FIFO for UART
	UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART1, &uartFifoConfigStruct);

	// Enable UART transmitter
	UART_TxCmd((LPC_UART_TypeDef *)LPC_UART1, ENABLE);

	// Enable UART RX interrupt
	UART_IntConfig((LPC_UART_TypeDef *)LPC_UART1, UART_INTCFG_RBR, ENABLE);

    // NVIC preemption = 1, sub-priority = 1
    // NVIC_SetPriority(UART1_IRQn, ((0x01 << 3) | 0x01));

    // Enable Interrupt for UART1 channel
    NVIC_EnableIRQ(UART1_IRQn);

	// Configure SHUTDOWN, RESET, ENABLE, DTR pins (output)
	GPIO_SetDir(GM862_SHUTDOWN_PORT, _BIT(GM862_SHUTDOWN_PIN), 1);
	GPIO_SetDir(GM862_RESET_PORT, _BIT(GM862_RESET_PIN), 1);
	GPIO_SetDir(GM862_ENABLE_PORT, _BIT(GM862_ENABLE_PIN), 1);
	GPIO_SetDir(GM862_DTR_PORT, _BIT(GM862_DTR_PIN), 1);

	// Configure PWRMON pin (input, pull-down)
	pinConfig.Funcnum = PINSEL_FUNC_0;
	pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinConfig.Pinmode = PINSEL_PINMODE_PULLDOWN;
	pinConfig.Pinnum = GM862_PWRMON_PIN;
	pinConfig.Portnum = GM862_PWRMON_PORT;
	PINSEL_ConfigPin(&pinConfig);
	GPIO_SetDir(GM862_PWRMON_PORT, _BIT(GM862_PWRMON_PIN), 0);

	// Configure DCD pin (input, pull-down)
	pinConfig.Funcnum = PINSEL_FUNC_0;
	pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinConfig.Pinmode = PINSEL_PINMODE_PULLDOWN;
	pinConfig.Pinnum = GM862_DCD_PIN;
	pinConfig.Portnum = GM862_DCD_PORT;
	PINSEL_ConfigPin(&pinConfig);
	GPIO_SetDir(GM862_DCD_PORT, _BIT(GM862_DCD_PIN), 0);

	/**** Start of Telit862 initialization sequence ****/

	// Pulse SHUTDOWN pin
	GPIO_SetValue(GM862_SHUTDOWN_PORT, _BIT(GM862_SHUTDOWN_PIN));
	CoTimeDelay(0, 0, 5, 0); // 5s
	GPIO_ClearValue(GM862_SHUTDOWN_PORT, _BIT(GM862_SHUTDOWN_PIN));

	// Pulse RESET pin
	GPIO_SetValue(GM862_RESET_PORT, _BIT(GM862_RESET_PIN));
	CoTimeDelay(0, 0, 0, 250); // 250ms
	GPIO_ClearValue(GM862_RESET_PORT, _BIT(GM862_RESET_PIN));

	// Wait for PWRMON signal to go low (max. 5s)
	int timeout = 50;
	for (;;)
	{
		if (!(GPIO_ReadValue(GM862_PWRMON_PORT) & _BIT(GM862_PWRMON_PIN)))
			break;

		if (timeout-- == 0)
			return FALSE; // timeout occurred

		CoTimeDelay(0, 0, 0, 100); // 100ms
	}

	// Pulse ENABLE pin
	GPIO_SetValue(GM862_ENABLE_PORT, _BIT(GM862_ENABLE_PIN));
	CoTimeDelay(0, 0, 1, 0); // 1s
	GPIO_ClearValue(GM862_ENABLE_PORT, _BIT(GM862_ENABLE_PIN));

	// If PWRMON signal did not go high, modem is not responsive
	if (!(GPIO_ReadValue(GM862_PWRMON_PORT) & _BIT(GM862_PWRMON_PIN)))
		return FALSE;

	// Buy modem a little more time
	CoTimeDelay(0, 0, 1, 0); // 1s

	/**** End of initialization sequence ****/

	// Try to have successful communication with modem
	uint8_t triesLeft = 5;
	for (;;)
	{
		GM862_SendAt("AT\r");
		if (GM862_GetResult() == GM862_RESULT_OK)
			break; // modem responded with OK
		if (triesLeft-- == 0)
			return FALSE; // after 5 unsuccessful tries, give up
	}

	return TRUE; // all good
}

/*
 * @brief		TODO: NOT TESTED
 */
BOOL GM862_Shutdown()
{
	GM862_SendAt("AT#SHDN\r");

	return (GM862_GetResult() == GM862_RESULT_OK);
}

/*
 * @brief		Set global timeout for AT responses
 * @param[in]	Timeout in CoOS ticks
 * @return		None
 */
void GM862_SetTimeout(uint16_t timeout)
{
	_timeout = timeout;
}

/*
 * @brief		Get status of network registration (+CREG)
 * @return		Network registration report
 */
GM862_NETREG_REPORT GM862_GetNetworkRegistrationReport()
{
	int report;

	GM862_SendAt("AT+CREG?\r");

	if (GM862_GetResponseFormat("+CREG: %*d,%d", &report) != 1)
		return GM862_REPORT_UNKNOWN;

	return (GM862_NETREG_REPORT)report;
}

/*
 * @brief		Configure if modem must register to a network
 * @param[in]	TRUE to force an attempt to register, FALSE to leave network
 * @return		TRUE if modem responded with OK, FALSE if timeout occurred
 */
BOOL GM862_SetNetworkRegistration(BOOL doRegister)
{
	int mode = doRegister ? 0 : 2;

	GM862_SendAtFormat("AT+COPS=%d\r", mode);

	return (GM862_GetResult() == GM862_RESULT_OK);
}

/*
 * @brief		Get signal quality of the GSM network connection
 * @return		Signal quality (0..99), or -1 if timeout occurred
 */
int8_t GM862_GetSignalQuality()
{
	int rssi;

	GM862_SendAt("AT+CSQ\r");

	if (GM862_GetResponseFormat("+CSQ %d,%*d", &rssi) != 1)
		return -1;

	return rssi;
}

/*
 * @brief		Send message (SMS) to a destination address
 * @param[in]	da Numeric destination address
 * @param[in]	message Null-terminated message to send
 * @return		TRUE if successful, otherwise FALSE
 */
BOOL GM862_SendMessage(const char *da, const char *message)
{
	// TODO: not implemented
	return FALSE;
}

/*
 * @brief		Enable or disable GPRS
 * @param[in]	enabled TRUE to enable, FALSE to disable
 * @return		TRUE if modem responded with OK, FALSE if timeout occurred
 */
BOOL GM862_SetGprs(BOOL enabled)
{
	// Feature: grab IP address (+IP: xxx.xxx.xxx.xxx)

	GM862_SendAtFormat("AT#GPRS=%d\r", enabled ? 1 : 0);

	return (GM862_GetResult() == GM862_RESULT_OK);
}

/*
 * @brief		TODO: NOT TESTED
 */
int8_t GM862_GetGprs()
{
	GM862_SendAt("AT#GPRS?\r");

	int status;
	if (GM862_GetResponseFormat("#GPRS: %d", &status) == 0)
		return -1;

	return (int8_t)status;
}

/*
 * @brief		Open TCP socket (note that GPRS must be enabled)
 * @param[in]	address IP address or DNS name of host
 * @param[in]	port Port number of host
 * @return		TRUE if successful, FALSE if connecting failed or timeout occurred
 */
BOOL GM862_OpenSocket(const char *address, uint16_t port)
{
	// Construct AT command to dial socket
	GM862_SendAtFormat("AT#SD=%d,0,%d,\"", 1, port);
	GM862_SendAt(address);
	GM862_SendAt("\",0,0,0\r");

	// GM862_SetTimeout(1000); // Temporary set timeout to 10 sec

	if (GM862_GetResult() != GM862_RESULT_CONNECT)
	{
		// GM862_SetTimeout(UART_TIMEOUT_DEFAULT); // Reset timeout
		return FALSE;
	}

	// GM862_SetTimeout(UART_TIMEOUT_DEFAULT); // Reset timeout

	// We are now in transparent mode, pulse DTR pin to re-enter command mode
	GPIO_SetValue(GM862_DTR_PORT, _BIT(GM862_DTR_PIN));
	CoTimeDelay(0, 0, 0, 500); // TODO: tweak delay
	GPIO_ClearValue(GM862_DTR_PORT, _BIT(GM862_DTR_PIN));
	CoTimeDelay(0, 0, 0, 500);

	return (GM862_GetResult() == GM862_RESULT_OK);
}

/*
 * @brief		Send a packet of data to host
 * @param[in]	packet Pointer to packet of data to send
 * @param[in]	packetLength Length of packet of data
 * @return		TRUE if all data is successfully sent, FALSE if connection failed or timeout occurred
 */
BOOL GM862_SendThroughSocket(uint8_t *packet, uint16_t packetLength)
{
	// Try to restore socket
	GM862_SendAtFormat("AT#SO=%d\r", 1);

	if (GM862_GetResult() != GM862_RESULT_CONNECT)
		return FALSE;

	// We are now in transparent mode, send all bytes
	while (packetLength--)
	{
		// If DCD pin goes high, socket is closed and we are back in command mode, stop sending
		if (GPIO_ReadValue(GM862_DCD_PORT) & _BIT(GM862_DCD_PIN))
			return FALSE;

		GM862_UART_SendByte(*packet++);
	}

	// Pulse DTR pin to re-enter command mode (this can be tweaked for sure)
	GPIO_SetValue(GM862_DTR_PORT, _BIT(GM862_DTR_PIN));
	CoTimeDelay(0, 0, 0, 250);
	GPIO_ClearValue(GM862_DTR_PORT, _BIT(GM862_DTR_PIN));
	CoTimeDelay(0, 0, 0, 250);

	return TRUE;
}

/*
 * @brief		Close TCP socket
 * @return		TRUE if modem responded with OK, FALSE if timeout occurred
 */
BOOL GM862_CloseSocket()
{
	// Close socket, closing an already closed socket won't result in an error
	GM862_SendAtFormat("AT#SH=%d\r", 1);

	return (GM862_GetResult() == GM862_RESULT_OK);
}

// TODO: doc
BOOL GM862_GetSocketStatus()
{
	int state;

	GM862_SendAtFormat("AT#SS=%d\r", 1);

	int result = GM862_GetResponseFormat("#SS: %u", &state);

	if (GM862_GetResult() != GM862_RESULT_OK)
		return FALSE;

	return (result == 1);
}

/*
 * @brief		Get GPS position
 * @param[out]	gpsData GPS positioning data if function returns TRUE
 * @return		TRUE if data is successfully acquired AND GPS has fix, otherwise FALSE
 */
BOOL GM862_GpsGetPosition(GM862_GPS_DATA *gpsData)
{
	double latitude, longitude, spkm;
	char latitudePos, longitudePos;
	int fix, nsat;

	// Request GPS position
	GM862_SendAt("AT$GPSACP\r");

	// Parse GPS data (example: "$GPSACP: 161514.000,5312.7499N,00547.9893E,31.6,70.8,2,258.70,0.82,0.44,070612,03")
	int result = GM862_GetResponseFormat("$GPSACP: %*lf,%lf%c,%lf%c,%*lf,%*lf,%u,%*lf,%lf,%*lf,%*u,%u",
			&latitude, &latitudePos, &longitude, &longitudePos, &fix, &spkm, &nsat);

	// This returns always OK with correct settings (GPS enabled)
	if (GM862_GetResult() != GM862_RESULT_OK)
		return FALSE;

	// Check for valid GPS data (invalid if response format is invalid or if there's no GPS fix)
	if (result != 7)
		return FALSE;

	// Convert to 32 bit
	gpsData->latitude = (uint32_t)(latitude * 10000);
	if (latitudePos == 'N')
		gpsData->latitude |= _BIT(28);

	// Convert to 32 bit
	gpsData->longitude = (uint32_t)(longitude * 10000);
	if (longitudePos = 'E')
		gpsData->longitude |= _BIT(28);

	gpsData->sog = (uint16_t)(spkm * 100);
	gpsData->nsat = (uint8_t)nsat;
	gpsData->fix = (uint8_t)fix;

	return TRUE;
}

/*
 * @brief		Send AT command in specified format and arguments
 * @param[in]	format Format string, use printf() as reference
 * @param[in]	... Variable count of arguments
 * @return		None
 */
void GM862_SendAtFormat(const char *format, ...)
{
	va_list args;

	// Prepare variable argument list
	va_start(args, format);

	// Construct string in specified format and arguments
	vsprintf(_globalBuffer, format, args);

	// Send formatted string
	GM862_SendAt(_globalBuffer);

	// Dispose variable argument list
	va_end(args);
}

/*
 * @brief		Send AT command, clears response buffer to get rid of unsolicited messages
 * @param[in]	command AT command including \r
 * @return		None
 */
void GM862_SendAt(const char *command)
{
	// Destroy all unsolicited messages spat out by GM862
	uint8_t recv;
	while (GM862_UART_Receive(&recv, 1) == 1);

	while (*command)
		GM862_UART_SendByte(*command++);
}

/*
 * @brief		Get AT command result
 * @return		AT result, whereby GM862_RESULT_UNKNOWN if timeout occurred
 */
GM862_RESULT GM862_GetResult()
{
	for (;;)
	{
		// Get response to parse
		if (!GM862_GetResponse(_globalBuffer, sizeof(_globalBuffer)))
			return GM862_RESULT_UNKNOWN;

		// Result codes are one digit long
		if (strlen(_globalBuffer) == 1)
			break;
	}

	// Return associating enumerator
	switch (_globalBuffer[0])
	{
		case '0':
			return GM862_RESULT_OK;
		case '1':
			return GM862_RESULT_CONNECT;
		case '3':
			return GM862_RESULT_NO_CARRIER;
		case '4':
			return GM862_RESULT_ERROR;
		default:
			return GM862_RESULT_UNKNOWN;
	}
}

/*
 * @brief		Same as GM862_GetResponse(), but parses as specified format and arguments
 * @param[in]	format Format string, use printf() as reference
 * @param[in]	... Variable count of arguments
 * @return		Number of parsed arguments
 */
int GM862_GetResponseFormat(const char *format, ...)
{
	va_list args;
	int result;

	// Get response
	if (!GM862_GetResponse(_globalBuffer, sizeof(_globalBuffer)))
		return 0;

	// Prepare variable argument list
	va_start(args, format);

	// Parse response as specified format and arguments
	result = vsscanf(_globalBuffer, format, args);

	// Dispose variable argument list
	va_end(args);

	return result;
}

/*
 * @brief		Pop one complete line (ending with \r) from RX ring buffer
 * @param[out]	respBuffer Response buffer to copy line to, always null-terminated
 * @param[in]	buffSize Size of response buffer
 * @return		TRUE if successful, FALSE if line size exceeds response buffer size or timeout occurred
 */
BOOL GM862_GetResponse(char *respBuffer, uint16_t buffSize)
{
	if (buffSize == 0)
		return FALSE; // shame on you!

	uint16_t count = 0;
	for (;;)
	{
		// Reserve space for the null-terminator
		if (buffSize - 1 == 0)
		{
			respBuffer[count] = '\0';
			return FALSE;
		}

		// Read one byte from UART ring buffer
		uint16_t recv = GM862_UART_ReceiveByte();
		if (recv & UART_TIMEOUT_OCCURRED)
		{
			Debug_Send(DM_ERROR, "Timeout occurred.");

			// Timeout occurred, exit now
			respBuffer[count] = '\0';
			return FALSE;
		}

		// Ignore line feed characters
		if (recv == '\n')
			continue;

		if (recv != '\r')
		{
			// Collect received byte
			respBuffer[count] = recv;
		}
		else
		{
			// Carriage return character found, exit now
			break;
		}

		buffSize--;
		count++;
	}

	respBuffer[count] = '\0';
	return TRUE;
}

/*
 * @brief		Send one byte over UART to modem
 * @param[in]	b Byte to send
 * @return		TRUE if successful, otherwise FALSE
 */
BOOL GM862_UART_SendByte(uint8_t b)
{
	return (GM862_UART_Send(&b, 1) == 1);
}

/*
 * @brief		Pop one byte from RX ring buffer with timeout
 * @return		UART_TIMEOUT_OCCURRED or one popped byte
 */
uint16_t GM862_UART_ReceiveByte()
{
	uint16_t timeout = _timeout;

	uint8_t recv;
	for (;;)
	{
		if (GM862_UART_Receive(&recv, 1) == 1)
			return recv;

		// Suspend task for one tick to wait for a byte
		if (timeout != UART_TIMEOUT_INFINITE)
		{
			if (timeout-- == 0)
				return UART_TIMEOUT_OCCURRED; // timeout occurred
			CoTickDelay(1);
		}
	}
}

/*
 * @brief		Send data over UART to modem
 * @param[in]	txbuf Buffer with data to send
 * @param[in]	buflen Count of data in txbuf to send
 * @return		Number of bytes send
 */
uint32_t GM862_UART_Send(uint8_t *txbuf, uint32_t buflen)
{
	// FEATURE: use a TX ring buffer
	return UART_Send((LPC_UART_TypeDef *)LPC_UART1, txbuf, buflen, BLOCKING);
}

/*
 * @brief		Pop data from RX ring buffer
 * @param[out]	rxbuf Buffer to copy popped data to
 * @param[in]	buflen Maximum number of bytes to pop
 * @return		Number of bytes popped
 */
uint32_t GM862_UART_Receive(uint8_t *rxbuf, uint32_t buflen)
{
	// Disable UART RX interrupt service routine (prevent race conditions)
	UART_IntConfig((LPC_UART_TypeDef *)LPC_UART1, UART_INTCFG_RBR, DISABLE);

	uint32_t count = 0;
	while (buflen--)
	{
		// Check if ring buffer is not empty
		if ((_ringBuffer.rxBufferTail == _ringBuffer.rxBufferHead) && !_ringBuffer.rxBufferIsFull)
			break; // it is, exit now

		// Get byte from ring buffer
		rxbuf[count++] = _ringBuffer.buffer[_ringBuffer.rxBufferTail];
		_ringBuffer.rxBufferTail = (_ringBuffer.rxBufferTail + 1) % UART_RING_BUFFER_SIZE;
		_ringBuffer.rxBufferIsFull = FALSE;
	}

	// Re-enable UART RX interrupt service routine
	UART_IntConfig((LPC_UART_TypeDef *)LPC_UART1, UART_INTCFG_RBR, ENABLE);

	return count;
}

/*
 * @brief		UART1 interrupt service routine
 * @return		None
 */
void UART1_IRQHandler()
{
	uint32_t intId = UART_GetIntId((LPC_UART_TypeDef *)LPC_UART1);

	// Receive data available interrupt has been requested
	if ((intId & UART_IIR_INTID_MASK) == UART_IIR_INTID_RDA)
	{
		uint8_t recv;
		for (;;)
		{
			// Get byte from UART RX FIFO
			if (UART_Receive((LPC_UART_TypeDef *)LPC_UART1, &recv, 1, NONE_BLOCKING) == 0)
				break; // UART RX FIFO is empty

			// Ring buffer is full (i.e. data wasn't retrieved fast enough from ring buffer)
			if (_ringBuffer.rxBufferIsFull)
				break;

			// Put byte into ring buffer
			_ringBuffer.buffer[_ringBuffer.rxBufferHead] = recv;
			_ringBuffer.rxBufferHead = (_ringBuffer.rxBufferHead + 1) % UART_RING_BUFFER_SIZE;
			if (_ringBuffer.rxBufferHead == _ringBuffer.rxBufferTail)
				_ringBuffer.rxBufferIsFull = TRUE;
		}
	}
}
