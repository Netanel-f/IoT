
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
    while (GPS_INITIALIZED && iteration_counter < 50) { \\ TODO remove iteration_counter < 50 condition
        iteration_counter++;
        result = GPSGetFixInformation(last_location);

        // TODO print it + Google Maps format + counter
        // Degrees, minutes, and seconds (DMS): 41°24'12.2"N 2°10'26.5"E
        // Degrees and decimal minutes (DMM): 41 24.2028, 2 10.4418
        // Decimal degrees (DD): 41.40338, 2.17403
        if (result == TRUE){
            printf("Iteration #%d\t", last_location->prefix);
            printf("latitude: %d\t", last_location->latitude);
            printf("longitude: %d\t", last_location->longitude);
            printf("altitude: %d\t", last_location->altitude);
            printf("time: %s\n", last_location->fixtime);

            Sleep(3);
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
