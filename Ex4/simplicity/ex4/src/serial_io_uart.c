#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "em_leuart.h"
#include "em_chip.h"

#include "serial_io_uart.h"

/**************************************************************************//**
 * 							GLOBAL VARIABLES
*****************************************************************************/
static uint32_t rxDataReady = 0;      // Flag indicating receiver does not have data
static char rxBuffer[RX_BUFFER_SIZE]; // Software receive buffer

/**************************************************************************//**
 * @brief
 *    Initialize the LEUART module
 *****************************************************************************/
void initLeuart(void)
{
  // Enable LE (low energy) clocks
  CMU_ClockEnable(cmuClock_HFLE, true); // Necessary for accessing LE modules
  CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO); // Set a reference clock

  // Enable clocks for LEUART0
  CMU_ClockEnable(cmuClock_LEUART0, true);
  CMU_ClockDivSet(cmuClock_LEUART0, cmuClkDiv_1); // Don't prescale LEUART clock


  // Initialize the LEUART0 module
  LEUART_Init_TypeDef init = LEUART_INIT_DEFAULT;
  LEUART_Init(LEUART0, &init);

  // Enable LEUART0 RX pins on PD[11]
  LEUART0->ROUTEPEN  = LEUART_ROUTEPEN_RXPEN;
  LEUART0->ROUTELOC0 = LEUART_ROUTELOC0_RXLOC_LOC18;

  // Enable LEUART0 RX interrupts
  LEUART_IntEnable(LEUART0, LEUART_IEN_RXDATAV);
  NVIC_EnableIRQ(LEUART0_IRQn);
}


/**************************************************************************//**
 * @brief
 *    LEUART0 interrupt service routine
 *
 * @details
 *    Keep receiving data while there is still data left in the hardware RX buffer.
 *    Store incoming data into rxBuffer and set rxDataReady when a linefeed '\n' is
 *    sent or if there is no more room in the buffer.
 *****************************************************************************/
void LEUART0_IRQHandler(void)
{
  // Note: These are static because the handler will exit/enter
  //       multiple times to fully transmit a message.
  static uint32_t rxIndex = 0;

  // Acknowledge the interrupt
  uint32_t flags = LEUART_IntGet(LEUART0);
  LEUART_IntClear(LEUART0, flags);

  // RX portion of the interrupt handler
  if (flags & LEUART_IF_RXDATAV) {
    while (LEUART0->STATUS & LEUART_STATUS_RXDATAV) { // While there is still incoming data
      char data = LEUART_Rx(LEUART0);
      if ((rxIndex < RX_BUFFER_SIZE - 2) && (data != '\n')) { // Save two spots for '\n' and '\0'
        rxBuffer[rxIndex++] = data;
      } else { // Done receiving
        rxBuffer[rxIndex++] = '\n';
        rxBuffer[rxIndex] = '\0';
        rxDataReady = 1;
        rxIndex = 0;
        break;
      }
    }
  }
}


/**************************************************************************//**
 * @brief
 * @param port - the serial port of the gps module
 * @param baud - the baud rate of the gps module.
 *****************************************************************************/
bool SerialInitGPS(char* port, unsigned int baud){
	// Initialize LEUART0 RX pin
	int num_port = atoi(port);
	GPIO_PinModeSet(num_port, 11, gpioModeInput, 0);    // RX
	initLeuart();
	return true;
}

/**************************************************************************//**
 * @brief
 * @param buf - to store result.
 * @param maxlen - the maximum length of result.
 * @param timeout_ms - timeout to receive.
 *****************************************************************************/
unsigned int SerialRecvGPS(unsigned char *buf, unsigned int maxlen, unsigned int timeout_ms){
	uint32_t i;
	uint32_t curTicks;

	curTicks = msTicks;
	while ((msTicks - curTicks) < timeout_ms){

		if (rxDataReady) {
			  LEUART_IntDisable(LEUART0, LEUART_IEN_RXDATAV); // Disable interrupts

			  for (i = 0; rxBuffer[i] != 0; i++) {
				  buf[i] = rxBuffer[i]; // Copy rxBuffer into txBuffer
			  }
			  buf[i] = '\0';
			  rxDataReady = 0; // Indicate that we need new data
			  LEUART_IntEnable(LEUART0, LEUART_IEN_RXDATAV); // Re-enable interrupts
			  return i;
		}
	}
	return i;
}

/**************************************************************************//**
 * @brief
 *****************************************************************************/
void SerialFlushInputBuffGPS(void){
	rxBuffer[0] = '\0';
}

/**************************************************************************//**
 * @brief
 *****************************************************************************/
void SerialDisableGPS(){
	SerialFlushInputBuffGPS();
	NVIC_DisableIRQ(LEUART0_IRQn);
	LEUART_IntDisable(LEUART0, LEUART_IEN_RXDATAV);
	CMU_ClockEnable(cmuClock_LEUART0, false);

}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void DelayGPS(uint32_t ms)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < ms) ;
}
