
#ifndef IOT_GPS_H
#define IOT_GPS_H

#include <stdbool.h>
#include <stdint.h>

typedef __packed struct _GPS_LOCATION_INFO {
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    u_int8_t hdop;
    uint8_t valid_fix : 1;
    uint8_t reserved1 : 3;
    uint8_t num_sats : 4;
    char fixtime[10];
} GPS_LOCATION_INFO;

void GPSInit();
uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen);
bool GPSGetFixInformation(GPS_LOCATION_INFO *location);
void GPSDisable();



#endif //IOT_GPS_H
