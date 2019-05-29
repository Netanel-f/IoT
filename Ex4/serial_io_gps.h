//
// Created by Sapir on 06-Apr-19.
//

#ifndef IOT_SERIAL_IO_H
#define IOT_SERIAL_IO_H

#define SERIAL_TIMEOUT -1

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

volatile uint32_t msTicks;

/**************************************************************************//**
 * @brief Initates the serial connection.
 * @param port - input port.
 * @param baud - baud rate
 * @return true if successful.
 *****************************************************************************/
bool SerialInitGPS(char* port, unsigned int baud);

/**************************************************************************//**
 * @brief Receive data from serial connection.
 * @param buf - buffer to be filled.
 * @param maxlen - maximum length of a line of data.
 * @param timeout_ms - length of the timeout in ms.
 *****************************************************************************/
unsigned int SerialRecvGPS(unsigned char* buf, unsigned int maxlen, unsigned int timeout_ms);


/**************************************************************************//**
 * @brief Empties the input buffer.
 *****************************************************************************/
void SerialFlushInputBuffGPS(void);

/**************************************************************************//**
 * @brief Disable the serial connection.
 *****************************************************************************/
void SerialDisableGPS();

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void DelayGPS(uint32_t ms);


#endif //IOT_SERIAL_IO_H
