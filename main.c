
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gps.h"

#define PRINT_FORMAT "\fIteration #%d \nLatitude:\n%d\nLongitude:\n%d\nAltitude:\n%d\nTime:\n%s\nGoogle Maps:\n%f, %f\n"

int main() {
    GPSInit();
    GPS_INITIALIZED = true;
    uint32_t iteration_counter = 0;
    GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
    if (last_location == NULL)
    {
        printf("Memory Allocation Error");
        return EXIT_FAILURE;
    }

    bool result;

    time_t init_time = time(NULL);
    time_t current_time;
    double diff_seconds;

    float lat_deg;
    float long_deg;
    while (GPS_INITIALIZED) {
        iteration_counter++;
        result = GPSGetFixInformation(last_location);
        current_time = time(NULL);
        diff_seconds = difftime(current_time, init_time);
        // Google Maps format
        // Decimal degrees (DD): 41.40338, 2.17403
        if (result && diff_seconds >= 3){
//        if (result){

            lat_deg = (last_location->latitude) / 10000000.0;
            long_deg = (last_location->longitude) / 10000000.0;
            printf(PRINT_FORMAT,
                    iteration_counter,
                    last_location->latitude,
                    last_location->longitude,
                    last_location->altitude,
                    last_location->fixtime,
                    lat_deg,
                    long_deg);

            fflush(stdout);

            init_time = current_time;
        }
    }
    free(last_location);
    GPSDisable();

    exit(0);
}

