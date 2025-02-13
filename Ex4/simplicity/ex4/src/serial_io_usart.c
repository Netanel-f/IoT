/**************************************************************************//**
 * @serial_io.c
 * @brief Interface for reading from serial port.
 * @version 0.0.1
 *  ***************************************************************************/
#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_chip.h"

#include "serial_io_usart.h"

/**************************************************************************//**
 * 							GLOBAL VARIABLES
*****************************************************************************/
static uint32_t rxReadIndex = 0;
static uint32_t rxWriteIndex = 0;
static char rxBuffer[RX_BUFFER_SIZE]; // Software receive buffer


void initUSART(void)
{
	// clear buffer
	memset(rxBuffer, '\0', RX_BUFFER_SIZE);

	// Enable LE (low energy) clocks
	CMU_ClockEnable(cmuClock_HFLE, true); // Necessary for accessing LE modules
	CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO); // Set a reference clock


	// Initialize the USART2 module
	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
	init.enable = usartDisable;
	USART_InitAsync(USART2, &init);

	// Enable TX/RX
	USART2->CMD = USART_CMD_RXEN | USART_CMD_TXEN;

	//// Enable USART RX pins on PA[7]
	USART2->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_TXPEN;
	USART2->ROUTELOC0 = USART_ROUTELOC0_RXLOC_LOC1 | USART_ROUTELOC0_TXLOC_LOC1;

	/* Clear previous RX interrupts */
	USART_IntClear(USART2 ,USART_IEN_RXDATAV);
	NVIC_ClearPendingIRQ(USART2_RX_IRQn);

	/* Enable RX interrupts */
	USART_IntEnable(USART2, USART_IEN_RXDATAV);
	NVIC_EnableIRQ(USART2_RX_IRQn);

	USART_Enable(USART2, usartEnable);
}

void USART2_RX_IRQHandler(void) {
	// Note: These are static because the handler will exit/enter
	//       multiple times to fully transmit a message.

	// Acknowledge the interrupt
	uint32_t flags = USART_IntGet(USART2);
	USART_IntClear(USART2, flags);


	// RX portion of the interrupt handler
	if (flags & USART_IF_RXDATAV) {
		char data = USART2->RXDATA;
		if (data != '\0') {
			rxBuffer[rxWriteIndex++] = data;
			rxWriteIndex = rxWriteIndex % RX_BUFFER_SIZE;
		}
	}
}


/**************************************************************************//**
 * @brief
 * @param port - the serial port of the cellular modem module
 * @param baud - the baud rate of the cellular modem module.
 *****************************************************************************/
bool SerialInitCellular(char* port, unsigned int baud) {
	// Initialize USART2 RX pin
	int num_port = atoi(port);

    if (num_port == gpioPortA) {
		// GPS PA7
		GPIO_PinModeSet(gpioPortA, 7, gpioModeInput, 0);
		// RX
		// GPS PA6
		GPIO_PinModeSet(gpioPortA, 6, gpioModePushPull, 1);    // TX
		initUSART();

	} else {
		return false;
	}

	return true;
}

/**************************************************************************//**
 * @brief
 * @param buf - to store result.
 * @param maxlen - the maximum length of result.
 * @param timeout_ms - timeout to receive.
 *****************************************************************************/
unsigned int SerialRecvCellular(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms){
	uint32_t i = 0;
	uint32_t curTicks;

	curTicks = msTicks;

	while ((msTicks - curTicks) < timeout_ms) {
		if (rxReadIndex != rxWriteIndex) {
			buf[i++] = rxBuffer[rxReadIndex];
			rxReadIndex = (++rxReadIndex) % RX_BUFFER_SIZE;
		}
	}
	return i;
}

/**
 * writing buf string to serial port
 * @param buf
 * @param size
 * @return true on success
 */
bool SerialSendCellular(unsigned char *buf, unsigned int size) {
	for (unsigned int index = 0; index < size; index++) {
		USART_Tx(USART2, buf[index]);
	}
	return true;
}


/**************************************************************************//**
 * @brief
 *****************************************************************************/
void SerialFlushInputBuffCellular(void){
	rxReadIndex = 0;
	rxWriteIndex = 0;
	memset(rxBuffer, '\0', RX_BUFFER_SIZE);
}

/**************************************************************************//**
 * @brief
 *****************************************************************************/
void SerialDisableCellular(){
	SerialFlushInputBuffCellular();
	NVIC_DisableIRQ(USART2_RX_IRQn);
	USART_IntDisable(USART2, USART_IEN_RXDATAV);
	CMU_ClockEnable(cmuClock_USART2, false);
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void DelayCellular(uint32_t ms) {
	uint32_t curTicks;

	curTicks = msTicks;
	while ((msTicks - curTicks) < ms) ;
}
