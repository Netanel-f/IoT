#include "gps.h"

void GPSInit() {
    GPS_INITIALIZED = SerialInit(PORT, BAUD_RATE);
}

uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen) {
    if (GPS_INITIALIZED) {
        return SerialRecv((unsigned char*) buf, MAX_NMEA_LEN, RECV_TIMEOUT_MS);
    }
    else {
        return 0;
    }
}


bool GPSGetFixInformation(GPS_LOCATION_INFO *location);

void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
