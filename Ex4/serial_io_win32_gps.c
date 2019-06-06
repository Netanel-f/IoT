
#include "serial_io_gps.h"
#include <windows.h>
#include <time.h>

static HANDLE hComm;
static COMMTIMEOUTS original_timeouts;
static COMMTIMEOUTS timeouts;

/**************************************************************************//**
 * @brief Initates the serial connection.
 * @param port - input port
 * @param baud - baud rate
 * @return true if succesful.
 *****************************************************************************/
bool SerialInitGPS(char* port, unsigned int baud)
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
    if (baud == 9600) {
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

/**************************************************************************//**
 * @brief Receive data from serial connection.
 * @param buf - buffer to be filled.
 * @param maxlen - maximum length of a line of data.
 * @param timeout_ms - length of the timeout in ms.
 *****************************************************************************/
unsigned int SerialRecvGPS(unsigned char* buf, unsigned int maxlen, unsigned int timeout_ms){
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
//    SerialFlushInputBuffGPS();
    return dwBytes > 0 ? dwBytes : SERIAL_TIMEOUT;
}

/**************************************************************************//**
 * @brief Empties the input buffer.
 *****************************************************************************/
void SerialFlushInputBuffGPS(void){
    FlushFileBuffers(hComm);
}

/**************************************************************************//**
 * @brief Disable the serial connection.
 *****************************************************************************/
void SerialDisableGPS(){
    SerialFlushInputBuffGPS();
    CloseHandle(hComm); //Closing the Serial Port
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void DelayGPS(uint32_t ms){
    time_t init_time = time(NULL);
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, init_time);
    while (diff_seconds < ms) {
        current_time = time(NULL);
        diff_seconds = difftime(current_time, init_time);
    }
}
