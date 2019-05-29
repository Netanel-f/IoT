
#include "serial_io_cellular.h"
#include <windows.h>
#include <time.h>

static HANDLE hComm;
static COMMTIMEOUTS original_timeouts;
static COMMTIMEOUTS timeouts;

/**************************************************************************//**
 * @brief Initiates the serial connection.
 * @param port - input port
 * @param baud - baud rate
 * @return true if successful.
 *****************************************************************************/
bool SerialInitCellular(char* port, unsigned int baud)
{
    char fullPort[20];
    DCB state;

    /* Open port */
    sprintf(fullPort, "\\\\.\\%s", port);
    hComm = CreateFile(fullPort, // Port Name
            GENERIC_READ | GENERIC_WRITE,        //Read/Write
            0,                   // No Sharing
            NULL,                // No Security
            OPEN_EXISTING,       // Open existing port only
            0,                   // Non Overlapped I/O
            NULL);

    /* Error handling */
    if (hComm == INVALID_HANDLE_VALUE) {
        printf("Error in opening serial port\n");
        return FALSE;
    }


    /* Set the Comm state */
    if (!GetCommState(hComm, &state)) {
        printf("GetCommState failed!\n");
        return FALSE;
    }

    if (baud == 9600) {
        // Quectel GPS rate
        state.BaudRate = CBR_9600;
    } else if (baud == 115200){
        // Gemalto Cellular modem rate
        state.BaudRate = CBR_115200;
    } else {
        printf("Wrong baud rate\n");
        return FALSE;
    }

    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    state.EvtChar = '\n';
    if (!SetCommState(hComm, &state)) {
        printf("SetCommState failed!\n");
        return FALSE;
    }

    if (!GetCommTimeouts(hComm, &original_timeouts)) {
        printf("GetCommTimeouts failed!\n");
        return FALSE;
    }
    timeouts = original_timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;

    /* set comm mask */
    if (!SetCommMask(hComm, EV_RXFLAG)) {
        printf("SetCommMask failed!\n");
        return FALSE;
    }

    /* setup device buffers */
    if (!SetupComm(hComm, 10000, 10000)) {
        printf("SetupComm failed!\n");
        return FALSE;
    }

    /* purge any information in the buffer */
    if (!PurgeComm(hComm, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
        printf("PurgeComm failed!\n");
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************//**
 * @brief Receive data from serial connection.
 * @param buf - buffer to be filled.
 * @param maxlen - maximum length of a line of data.
 * @param timeout_ms - length of the timeout in ms.
 *****************************************************************************/
unsigned int SerialRecvCellular(unsigned char* buf, unsigned int maxlen, unsigned int timeout_ms){
    DWORD dwBytes = 0;
    timeouts.ReadTotalTimeoutConstant = timeout_ms;
    SetCommTimeouts(hComm, &timeouts);

    DWORD  dwEventMask;
    WaitCommEvent(hComm, &dwEventMask, NULL);

    if(!ReadFile(hComm, buf, maxlen, &dwBytes, NULL)){
        printf("Error reading from serial port.\n");
        return 0;
    }
    return dwBytes > 0 ? dwBytes : SERIAL_TIMEOUT;
}

/**
 * will send first size bytes of buf to serial socket
 * @param buf
 * @param size
 * @return
 */
bool SerialSendCellular(unsigned char *buf, unsigned int size) {
    DWORD numOfBytesWritten = 0;
//    SerialFlushInputBuff();
    return WriteFile(hComm, buf, size, &numOfBytesWritten, NULL);
}

/**************************************************************************//**
 * @brief Empties the input buffer.
 *****************************************************************************/
void SerialFlushInputBuff(void){
    FlushFileBuffers(hComm);
}

/**************************************************************************//**
 * @brief Disable the serial connection.
 *****************************************************************************/
void SerialDisableCellular(){
    SerialFlushInputBuff();
    CloseHandle(hComm); //Closing the Serial Port
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void DelayCellular(uint32_t ms){
    time_t init_time = time(NULL);
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, init_time);
    while (diff_seconds < ms) {
        current_time = time(NULL);
        diff_seconds = difftime(current_time, init_time);
    }
}
