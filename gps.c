#include "gps.h"

void GPSInit() {
    GPS_INITIALIZED = SerialInit(PORT, BAUD_RATE);
}

uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen) {
    unsigned char small_buf[84] = "";
    int i = 0;
    if (GPS_INITIALIZED) {
        bytes_read = SerialRecv(small_buf, MAX_NMEA_LEN, RECV_TIMEOUT_MS);

    }
    else {
        return 0;
    }
}


bool GPSGetFixInformation(GPS_LOCATION_INFO *location){

}

void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
