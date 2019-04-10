
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/unistd.h>
#include "gps.h"

#define PRINT_FORMAT "Iteration #%d \nlatitude: %d\nlongitude: %d\naltitude: %d\ntime: %s\nGoogle Maps: %f, %f\n"

bool print_flag = false;

void alarm_handler(int signum) {
    print_flag = true;
    alarm(3);
}

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

    signal(SIGALRM, alarm_handler);
    alarm(3);
    float lat_deg;
    float long_deg;
    while (GPS_INITIALIZED) {
        iteration_counter++;
        result = GPSGetFixInformation(last_location);

        // Google Maps format
        // Decimal degrees (DD): 41.40338, 2.17403
        if (result && print_flag){

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

        }
    }
    free(last_location);
    GPSDisable();

    exit(0);
}

