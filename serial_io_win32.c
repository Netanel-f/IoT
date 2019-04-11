
#include "serial_io.h"
#include <windows.h>

static HANDLE hComm;
static COMMTIMEOUTS original_timeouts;
static COMMTIMEOUTS timeouts;

/*
 * Initialize serial port.
 */
bool SerialInit(char* port, unsigned int baud)
{
    char fullPort[20];
    DCB state;

    /* Open port */
    sprintf(fullPort, "\\\\.\\%s", port);
    hComm = CreateFile(fullPort, // Port Name
            GENERIC_READ, //Read/Write
            0,                            // No Sharing
            NULL,                         // No Security
            OPEN_EXISTING,// Open existing port only
            0,            // Non Overlapped I/O
            NULL);

    /* Error handling */
    if (hComm == INVALID_HANDLE_VALUE)
    {
        printf("Error in opening serial port\n");
        return FALSE;
    }


    /* Set the Comm state */
    GetCommState(hComm, &state);
    if (baud == 9600)
    {
        state.BaudRate = CBR_9600;
    } else {
        printf("Wrong baud rate\n");
    }
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    state.EvtChar = '\n';
    SetCommState(hComm, &state);

    GetCommTimeouts(hComm, &original_timeouts);
    timeouts = original_timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;

    /* setup device buffers */
    SetupComm(hComm, 10000, 10000);

    /* purge any information in the buffer */
    PurgeComm(hComm, PURGE_RXABORT | PURGE_RXCLEAR);

    return TRUE;
}

unsigned int SerialRecv(unsigned char* buf, unsigned int maxlen, unsigned int timeout_ms){
    DWORD dwBytes = 0;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    SetCommTimeouts(hComm, &timeouts);

    SetCommMask(hComm, EV_RXFLAG);
    DWORD  dwEventMask;
    WaitCommEvent(hComm, &dwEventMask, NULL);

    if(!ReadFile(hComm, buf, maxlen, &dwBytes, NULL)){
        printf("Error reading port\n");
        return 0;
    }
    return dwBytes > 0 ? dwBytes : SERIAL_TIMEOUT;
}

void SerialFlushInputBuff(void){
    FlushFileBuffers(hComm);
}

void SerialDisable(){
    SerialFlushInputBuff();
    CloseHandle(hComm); //Closing the Serial Port
}
