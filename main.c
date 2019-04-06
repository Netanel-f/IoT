
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "gps.h"


//int main() {
//    GPSInit();
//    uint32_t iteration_counter = 0;
//    GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
//    if (last_location == NULL)
//    {
//        printf("Memory Allocation Error");
//        return EXIT_FAILURE;
//    }
//
//    while (GPS_INITIALIZED) {
//        iteration_counter++;
//        GPSGetFixInformation(last_location); // TODO handle return value
//
//        // TODO print it + Google Maps format + counter
//
//        Sleep(3);
//    }
//    // TODO disable + free malloc + handle error to free!
//    //
//    exit(0);
//}

int main()
{
    if (SerialInit(PORT, BAUD_RATE))
    {
        for (int i = 0; i < 50; i++)
        {
            unsigned char buf[84] = "";
            SerialRecv(buf, 84, 5000);
            printf("%s\n", buf);
        }
        SerialDisable();
    }
    exit(0);
}