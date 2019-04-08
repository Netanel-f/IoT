
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "gps.h"


int main() {
    GPSInit();
    GPS_INITIALIZED = TRUE;
    uint32_t iteration_counter = 0;
    GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
    if (last_location == NULL)
    {
        printf("Memory Allocation Error");
        return EXIT_FAILURE;
    }

    bool result;
    while (GPS_INITIALIZED && iteration_counter < 50) {
        iteration_counter++;
        result = GPSGetFixInformation(last_location);

//        // TODO print it + Google Maps format + counter
        if (result == TRUE){
            printf("TYPE: %d\t", last_location->prefix);
            printf("latitude: %d\t", last_location->latitude);
            printf("longitude: %d\t", last_location->longitude);
            printf("altitude: %d\t", last_location->altitude);
            printf("time: %s\n", last_location->fixtime);

            Sleep(1);
        }
    }
    free(last_location);
    GPSDisable();
//    // TODO disable + free malloc + handle error to free!

    exit(0);
}

//int main()
//{
//    if (SerialInit(PORT, BAUD_RATE))
//    {
//        for (int i = 0; i < 50; i++)
//        {
//            unsigned char buf[84] = "";
//            SerialRecv(buf, 84, 5000);
//            printf("%s", buf);
//        }
//        SerialDisable();
//    }
//    exit(0);
//}

//int main()
//{
//    GPS_LOCATION_INFO* last;
//    GPS_LOCATION_INFO* loc = malloc(sizeof(GPS_LOCATION_INFO));
//    int result = GPSGetFixInformation(loc);
//    free(loc);
//    exit(result);
//}