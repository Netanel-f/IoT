
#include <stdio.h>
#include <stdlib.h>
#include "serial_io.h"
#define PORT "COM5"
#define BAUD_RATE 9600

int main()
{
    if (SerialInit(PORT, BAUD_RATE))
    {
        for (int i = 0; i < 10000; i++)
        {
            unsigned char buf[84] = "";
            SerialRecv(buf, 84, 5000);
            printf("%s", buf);
        }
        SerialDisable();
    }
    exit(0);
}