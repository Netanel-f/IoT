
#ifndef IOT_GPS_H
#define IOT_GPS_H

#include <stdbool.h>
#include <stdint.h>
#include "serial_io.h"

#define PORT "COM5"
#define BAUD_RATE 9600
#define MAX_NMEA_LEN 84
#define RECV_TIMEOUT_MS 5000    // TODO is it correct?

#define __packed __attribute__((__packed__))

/* Parsing defs */
#define GGA_PREFIX "$GPGGA" // GGA - most info
#define RMC_PREFIX "$GPRMC" // has date
#define GSA_PREFIX "$GPGSA"
#define PREFIX_LEN 6
#define DELIMITER ','
#define END_OF_TOKEN '\0'
#define GGA_FIELDS_NUM 14
#define RMC_FIELDS_NUM 11
#define GSA_FIELDS_NUM 17
#define GGA_MIN_FIELDS 7 // time .. #satellites
#define RMC_DATE_FIELD 8
#define RMC_TIME_FIELD 0

#define FLOAT_RMV_FACTOR 10000000
#define LAT_DEG_DIGITS 2
#define LONGIT_DEG_DIGITS 3
#define ALT_DEG_DIGITS 3

/* FOR TESTING */ //TODO: remove
#define SAMPLE_LINE "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
// "$GPGGA,123519,4807.038,N,07402.499,W,0,00,,,M,,M,,*5C";
/***************/

static bool GPS_INITIALIZED = FALSE;

typedef __packed struct _GPS_LOCATION_INFO {
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint8_t hdop;
    uint8_t valid_fix : 1;
    uint8_t reserved1 : 3;
    uint8_t num_sats : 4;
    char fixtime[13]; // hhmmssDDMMYY \0
} GPS_LOCATION_INFO;

void GPSInit();
uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen);
bool GPSGetFixInformation(GPS_LOCATION_INFO *location);
void GPSDisable();



#endif //IOT_GPS_H
