#include "gps.h"



void GPSInit() {
    GPS_INITIALIZED = SerialInit(PORT, BAUD_RATE);
}

uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen) {
    if (GPS_INITIALIZED) {
        return SerialRecv((unsigned char*) buf, maxlen, RECV_TIMEOUT_MS);

    }
    else {
        return 0;
    }
}



bool GPSGetFixInformation(GPS_LOCATION_INFO *location){
    // call GPSGetReadRaw
//    char buf[MAX_NMEA_LEN] = "";
//    uint32_t bytes_read = GPSGetReadRaw(buf, MAX_NMEA_LEN);

    // for testing:
    char buf[MAX_NMEA_LEN] = "$GPGSA,A,3,22,18,21,06,03,09,24,15,,,,,2.5,1.6,1.9*3E";

    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
        int i = 0;
        // split into tokens
        char *p = strtok (buf, DELIMITER);
        char *tokens_array[GSA_FIELDS_NUM];

        while (p != NULL)
        {
            // fill array with pointers to tokens
            tokens_array[i++] = p;
            p = strtok (NULL, DELIMITER);
        }

        for (i = 0; i < GSA_FIELDS_NUM; ++i)
        {
            // do something with the tokens TODO
        }

        return 0;

    }
}

void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
