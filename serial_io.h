//
// Created by Sapir on 06-Apr-19.
//

#ifndef IOT_SERIAL_IO_H
#define IOT_SERIAL_IO_H

#define SERIAL_TIMEOUT -1

#include <stdbool.h>
#include <stdio.h>


bool SerialInit(char* port, unsigned int baud);
unsigned int SerialRecv(unsigned char* buf, unsigned int maxlen, unsigned int timeout_ms);
void SerialFlushInputBuff(void);
void SerialDisable();


#endif //IOT_SERIAL_IO_H
