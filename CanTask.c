
/* Includes */

#include "CanTask.h"

/* Defines */

#define CAN_RING_BUFFER_SIZE				(32)

/* Struct */

typedef struct {

	CAN_MSG_Type rxBuffer[CAN_RING_BUFFER_SIZE];
	BOOL rxBufferIsFull;
	uint32_t rxBufferHead;
	uint32_t rxBufferTail;

} CAN_RING_BUFFER_T;

/* Prototypes */

void CanTask_CanInit();
BOOL CanTask_CanReceive(CAN_MSG_Type *msg);

/* Variables */

static CAN_RING_BUFFER_T _ringBuffer; // CAN RX ring buffer

/* Implementation */

void CanTask_Run(void *pdata)
{
	CAN_MSG_Type msg;

	Debug_Send(DM_INFO, "CAN task started.");

	// Set ring buffer to default state
	_ringBuffer.rxBufferIsFull = FALSE;
	_ringBuffer.rxBufferHead = 0;
	_ringBuffer.rxBufferTail = 0;

	// Initialize sensor data manager
	SensorDataManager_Init();

	// Initialize CAN
	CanTask_CanInit();

	for (;;)
	{
		// Get CAN message from ring buffer
		if (!CanTask_CanReceive(&msg))
		{
			// No message received, look again after 1 CoOS tick
			CoTickDelay(1);
			continue;
		}

		// Process message
		SensorDataManager_PutCanData(&msg);

		/*char buffer[256];
		sprintf(buffer, "Message received (id: %X, length: %X, format: %X, type: %X, data: %X %X %X %X %X %X %X %X)!\r\n",
				msg.id, msg.len, msg.format, msg.type,
				msg.dataA[0], msg.dataA[1], msg.dataA[2], msg.dataA[3],
				msg.dataB[0], msg.dataB[1], msg.dataB[2], msg.dataB[3]);

		Debug_Send(DM_INFO, buffer);*/
	}
}

void CanTask_CanInit()
{
	// Pin configuration CAN2
	PINSEL_CFG_Type pinConfig;
	pinConfig.Funcnum = PINSEL_FUNC_2;
	pinConfig.OpenDrain = PINSEL_PINMODE_NORMAL;
	pinConfig.Pinmode = PINSEL_PINMODE_PULLUP;
	pinConfig.Portnum = PINSEL_PORT_0;
	// P0.04 CAN Receiver
	pinConfig.Pinnum = PINSEL_PIN_4;
	PINSEL_ConfigPin(&pinConfig);
	// P0.05 CAN Transmitter
	pinConfig.Pinnum = PINSEL_PIN_5;
	PINSEL_ConfigPin(&pinConfig);

	// Initialize CAN
	CAN_Init(LPC_CAN2, 250000);
	CAN_ModeConfig(LPC_CAN2, CAN_OPERATING_MODE, ENABLE);

	// Enable receive interrupt
	CAN_IRQCmd(LPC_CAN2, CANINT_RIE, ENABLE);

	// Bypass acceptance filter
	CAN_SetAFMode(LPC_CANAF, CAN_AccBP);

	// Enable CAN interrupts
	NVIC_EnableIRQ(CAN_IRQn);
}

BOOL CanTask_CanReceive(CAN_MSG_Type *msg)
{
	// Disable CAN RX interrupt service routine (prevent race conditions)
	CAN_IRQCmd((LPC_UART_TypeDef *)LPC_CAN2, CANINT_RIE, DISABLE);

	// Check if ring buffer is not empty
	if ((_ringBuffer.rxBufferTail == _ringBuffer.rxBufferHead) && !_ringBuffer.rxBufferIsFull)
	{
		// Re-enable CAN RX interrupt service routine
		CAN_IRQCmd((LPC_UART_TypeDef *)LPC_CAN2, CANINT_RIE, ENABLE);

		return FALSE; // buffer is empty
	}
	else
	{
		// Get message from CAN ring buffer
		*msg = _ringBuffer.rxBuffer[_ringBuffer.rxBufferTail];
		_ringBuffer.rxBufferTail = (_ringBuffer.rxBufferTail + 1) % CAN_RING_BUFFER_SIZE;
		_ringBuffer.rxBufferIsFull = FALSE;

		// Re-enable CAN RX interrupt service routine
		CAN_IRQCmd((LPC_UART_TypeDef *)LPC_CAN2, CANINT_RIE, ENABLE);

		return TRUE; // successful
	}
}

void CAN_IRQHandler()
{
	Debug_Send(DM_INFO, "IRQ IS GEROEPEREERT, dat is goed");
	CAN_MSG_Type msg;

	uint32_t icrCAN2 = CAN_IntGetStatus(LPC_CAN2);
	if (icrCAN2 & (1 << 0)) // CAN message received
	{
		// Get CAN message
		CAN_ReceiveMsg(LPC_CAN2, &msg);

		// Ring buffer is full (i.e. data wasn't retrieved fast enough from ring buffer)
		if (_ringBuffer.rxBufferIsFull)
			return;

		// Put CAN message into ring buffer
		_ringBuffer.rxBuffer[_ringBuffer.rxBufferHead] = msg;
		_ringBuffer.rxBufferHead = (_ringBuffer.rxBufferHead + 1) % CAN_RING_BUFFER_SIZE;
		if (_ringBuffer.rxBufferHead == _ringBuffer.rxBufferTail)
			_ringBuffer.rxBufferIsFull = TRUE;
	}
}
