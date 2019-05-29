/**************************************************************************//**
 * @gps.h
 * @brief Interface for reading and parsing data from the GPS.
 * @version 0.0.1
 *  ***************************************************************************/
#ifndef GPS_H_
#define GPS_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "serial_io.h"

/**************************************************************************//**
 * 								DEFS
*****************************************************************************/


#define BAUD_RATE 9600
#define MAX_NMEA_LEN 82
#define RECV_TIMEOUT_MS 3000

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
#define GGA_MIN_REQUIRED_FIELDS 7 // time .. #satellites
#define RMC_DATE_FIELD 8
#define RMC_TIME_FIELD 0

#define FLOAT_RMV_FACTOR 10000000
#define ALT_FACTOR 100
#define HDOP_FACTOR 5
#define LAT_DEG_DIGITS 2
#define LONGIT_DEG_DIGITS 3

#define DATE_FORMAT "%c%c:%c%c:%c%c %c%c.%c%c.%c%c"

#define GPS_PAYLOAD_FORMAT "--data-binary gps,name=NetanelFayoumi_SapirElyovitch,ICCID=%s latitude=%u,longitude= %u,altitude=%u,hdop=%u,valid_fix=%u,num_sats=%u %s"

typedef __packed struct _GPS_LOCATION_INFO {
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint8_t hdop;
    uint8_t valid_fix : 1;
    uint8_t reserved1 : 3;
    uint8_t num_sats : 4;
    char fixtime[18]; // hh:mm:ss DD.MM.YY\0
} GPS_LOCATION_INFO;

/**************************************************************************//**
 * 							GLOBAL VARIABLES
*****************************************************************************/

static bool GPS_INITIALIZED = false;

/**************************************************************************//**
 * @brief Initiate GPS connection.
*****************************************************************************/
void GPSInit(char * port);

/**
 * Gets line from GPS.
 * @param buf - output buffer to put the line into.
 * @param maxlen - max length of a line.
 * @return number of bytes received.
 */
uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen);

/**
 * Updates location with information from GPSGetReadRaw.
 * @param location - the struct to be filled.
 * @return 0 if successful, -1 otherwise.
 */
bool GPSGetFixInformation(GPS_LOCATION_INFO *location);



/**
 * Disable GPS connection.
 */
void GPSDisable();

void GPSConvertFixtimeToUnixTime(GPS_LOCATION_INFO * gps_data, char * unix_time);
#endif /* GPS_H_ */

/**
  *
  * @param gps_data GPS LOCATION as received from GPS module
  * @param gps_payload where to store result
  * @return length of payload
  */
int GPSGetPayload(GPS_LOCATION_INFO * gps_data, char * iccid, char * gps_payload);