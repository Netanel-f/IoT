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

bool splitLineToFields(int num_of_fields, int prefix_length, char buf[MAX_NMEA_LEN], char**
tokens_array){


    char* p = &buf[prefix_length]; // start line after prefix
    int i=0;
    char* last_token = NULL;
    char** first_token = tokens_array;

    while (i < num_of_fields) {
        if (*p == DELIMITER)
        {
            *p = END_OF_TOKEN;
            if (last_token != NULL){
//                tokens_array[i++] = last_token;
                tokens_array[i++] = last_token;
            }
            // new field
            p++;
            if (*p == DELIMITER)
            {
                // empty field
                tokens_array[i++] = NULL;
                last_token = NULL;
            } else {
                // non-empty field
                last_token = p;
            }
        } else {
            // same field
            if (i == num_of_fields-1) // last field
            {
                tokens_array[i++] = last_token;
            }
            p++;
        }
    }
    return 0;
}

bool GPSGetFixInformation(GPS_LOCATION_INFO *location){
    // call GPSGetReadRaw
//    char buf[MAX_NMEA_LEN] = "";
//    uint32_t bytes_read = GPSGetReadRaw(buf, MAX_NMEA_LEN);

    // for testing:
    char buf[MAX_NMEA_LEN] = "$GPGGA,9,1,N,07402.499,W,0,00,,,M,,M,,*5C";

    char* tokens_array[GGA_FIELDS_NUM];
    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
        splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
    } else if (strncmp(buf, RMC_PREFIX, PREFIX_LEN) == 0) {
        char* tokens_array[RMC_FIELDS_NUM];
        splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
    } else {
        // unimpoortant line
    }
    return 0;
    }


void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
