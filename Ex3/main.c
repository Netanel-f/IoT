
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "serial_io.h"  // TODO REMOVE
#include "cellular.h"
#include "string.h"

#define PRINT_FORMAT "Iteration #%d \nLatitude:\n%d\nLongitude:\n%d\nAltitude:\n%d\nTime:\n%s\nGoogle Maps:\n%f, %f\n"
int main() {
    // Initialize the cellular modems.
    CellularInit(PORT);
    // Makes sure it’s responding to AT commands.
    while (CellularCheckModem()) {
        Delay(10);
    }
    // Finds all available cellular operators.
    int max_operators = 10; //TODO decide
    int num_operators_found = 0;
    OPERATOR_INFO operators_info[max_operators];
    while (num_operators_found == 0) {
        CellularGetOperators(operators_info, max_operators, &num_operators_found);
    }

    // Tries to register with each one of them (one at a time).
    int specific_operator_mode = 1;
    for (int op_index = 0; op_index <= num_operators_found; op_index++) {
        if (CellularSetOperator(specific_operator_mode, operators_info[op_index].operatorName)) {
            // If registered successfully, it prints the signal quality.
            int signal_quality = -1;
            if (CellularGetSignalQuality(&signal_quality)) {
                printf("signal quality %d\n", signal_quality);
            }
        }
    }
    // Print the program’s progress all along (e.g. “Checking modem…the modem is ready!”  etc.)
    exit(0);
}
//int old_main() {
//    SerialInit("COM5", 115200);
//
//    unsigned char systart[] = "\r\n^SYSSTART\r\n";
//    unsigned char buff[50] = "";
//    int maxlen = 50;
//    unsigned int timout_ms = 100;
//
//    do {
//        SerialRecv(buff, maxlen, timout_ms);
//        printf("loop - %s", buff);
//    } while (memcmp(buff, systart, (size_t) 13) != 0);
//
//    fprintf(stderr, "end of loop\n");
//
//    unsigned char stopecho[] = "ATE0\r\n";
//    if (!SerialSend(stopecho, sizeof("ATE0\r\n"))) {
//        fprintf(stderr, "send error\n");
//    }
//    printf("buffer001: %s", buff);
//    memset(buff, 0, 50);
//
//    SerialRecv(buff, maxlen, timout_ms);
//    printf("buffer002: %s", buff);
//    memset(buff, 0, 50);
//
//    unsigned char hello[] = "AT\r\n";
//    if (!SerialSend(hello, sizeof("AT\r\n"))) {
//        fprintf(stderr, "send error\n");
//    }
//    printf("buffer1: %s", buff);
//    memset(buff, 0, 50);
//    SerialRecv(buff, maxlen, timout_ms);
//    printf("buffer2: %s", buff);
//    memset(buff, 0, 50);
//    printf("%s", buff);
//
//    unsigned char goodbye[] = "AT^SMSO\r\n";
//    if (!SerialSend(goodbye, sizeof("AT^SMSO\r\n"))) {
//        fprintf(stderr, "send error\n");
//    }
//    SerialDisable();
//    exit(0);
//
////    GPSInit();
////    GPS_INITIALIZED = true;
////    uint32_t iteration_counter = 0;
////    GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
////    if (last_location == NULL)
////    {
////        printf("Memory Allocation Error");
////        return EXIT_FAILURE;
////    }
////
////    bool result;
////
////    time_t init_time = time(NULL);
////    time_t current_time;
////    double diff_seconds;
////
////    float lat_deg;
////    float long_deg;
////    while (GPS_INITIALIZED) {
////        iteration_counter++;
////        result = GPSGetFixInformation(last_location);
////        current_time = time(NULL);
////        diff_seconds = difftime(current_time, init_time);
////        // Google Maps format
////        // Decimal degrees (DD): 41.40338, 2.17403
////        if (result && diff_seconds >= 3){
////
////            lat_deg = (last_location->latitude) / 10000000.0;
////            long_deg = (last_location->longitude) / 10000000.0;
////            printf(PRINT_FORMAT,
////                    iteration_counter,
////                    last_location->latitude,
////                    last_location->longitude,
////                    last_location->altitude,
////                    last_location->fixtime,
////                    lat_deg,
////                    long_deg);
////
////            fflush(stdout);
////
////            init_time = current_time;
////        }
////    }
////    free(last_location);
////    GPSDisable();
//
//    exit(0);
//}

