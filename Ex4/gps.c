/**************************************************************************//**
 * @gps.c
 * @brief Interface for reading and parsing data from the GPS.
 * @version 0.0.1
 *  ***************************************************************************/
#include <stdlib.h>
#include <time.h>
#include "gps.h"


/**************************************************************************//**
 * @brief Initiate GPS connection.
 *****************************************************************************/
void GPSInit(char * port) {
    GPS_INITIALIZED = SerialInitGPS(port, GPS_BAUD_RATE);
    if (!GPS_INITIALIZED) {
        exit(EXIT_FAILURE);
    }
}

/**************************************************************************//**
 * Gets line from GPS.
 * @param buf - output buffer to put the line into.
 * @param maxlen - max length of a line.
 * @return number of bytes received.
 *****************************************************************************/
uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen) {
    if (GPS_INITIALIZED) {
        return SerialRecvGPS((unsigned char*) buf, maxlen, RECV_TIMEOUT_MS);
    }
    else {
        return 0;
    }
}

/**************************************************************************//**
 * Gets a GPS line and splits it into tokens.
 * @param num_of_fields - number of fields in the type of line being split.
 * @param prefix_length - the length of the line prefix.
 * @param buf - the buffer.
 * @param tokens_array - the output token array.
 * @return true if successful, false otherwise.
 *****************************************************************************/
bool splitLineToFields(int num_of_fields, int prefix_length, char
buf[MAX_NMEA_LEN], char**
tokens_array){

    char* p = &buf[prefix_length]; // start line after prefix
    int i=0;
    char* last_token = NULL;

    while (i < num_of_fields) {
        if (*p == DELIMITER)
        {
            *p = END_OF_TOKEN;
            if (last_token != NULL){
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
    return true;
}

/**************************************************************************//**
 * Gets array (split_line) with tokens that are RMC's fields,
 * updates location accordingly.
 * @param split_line - RMC line split into tokens.
 * @param location - location struct to be filled.
 * @return true if successful, false otherwise.
 *****************************************************************************/
bool parseRMC(char** split_line, GPS_LOCATION_INFO *location){
    // only fills date and time in the following order:
    // hhmmssDDMMYY
    if (split_line[RMC_TIME_FIELD] == NULL ||
        split_line[RMC_DATE_FIELD] == NULL){
        return false;
    }
//    strcpy(location->fixtime, split_line[RMC_TIME_FIELD]);
//    strcpy(location->fixtime+6, split_line[RMC_DATE_FIELD]);
    sprintf(location->fixtime,
            DATE_FORMAT,
            split_line[RMC_TIME_FIELD][0],
            split_line[RMC_TIME_FIELD][1],
            split_line[RMC_TIME_FIELD][2],
            split_line[RMC_TIME_FIELD][3],
            split_line[RMC_TIME_FIELD][4],
            split_line[RMC_TIME_FIELD][5],
            split_line[RMC_DATE_FIELD][0],
            split_line[RMC_DATE_FIELD][1],
            split_line[RMC_DATE_FIELD][2],
            split_line[RMC_DATE_FIELD][3],
            split_line[RMC_DATE_FIELD][4],
            split_line[RMC_DATE_FIELD][5]);
    return true;
}

/**************************************************************************//**
 * Gets array (split_line) with tokens that are GGA's fields,
 * updates location accordingly.
 * @param split_line - GGA line split into tokens.
 * @param location - location struct to be filled.
 * @return true if successful, false otherwise.
 *****************************************************************************/
bool parseGGA(char** split_line, GPS_LOCATION_INFO *location){

    // check for empty required fields
    for (int j=0; j<GGA_MIN_REQUIRED_FIELDS; j++){
        if (split_line[j] == NULL){
            return false;
        }
    }

    // counter for the fields in split_line
    int i = 0;

    /* time */ //HHMMSS (UTC)
//    strcpy(location->fixtime, split_line[i]);
    i++;

    /* latitude */
    int32_t degrees = (split_line[i][0] - '0') * 10 + (split_line[i][1] - '0');
    split_line[i] += LAT_DEG_DIGITS;
    double minutes = atof(split_line[i]) / 60;
    minutes += degrees;
    degrees = minutes * FLOAT_RMV_FACTOR;
    i++;

    /* +N/S- */
    if (*split_line[i] == 'S')
    {
        // negate result for south
        degrees = 0 - degrees;
    }
    location->latitude = degrees;
    i++;

    /* Longtitude */
    degrees = (split_line[i][0] - '0') * 100 + (split_line[i][1] - '0') * 10 +
              (split_line[i][2] - '0');
    split_line[i] += LONGIT_DEG_DIGITS;
    minutes = (double) atof(split_line[i]) / 60;
    minutes += degrees;
    degrees = minutes * FLOAT_RMV_FACTOR;
    i++;

    /* +E/W- */
    if (*split_line[i] == 'W')
    {
        // negate result for west
        degrees = 0 - degrees;
    }
    location->longitude = degrees;
    i++;

    /* fix quality */
    location->valid_fix = atoi(split_line[i]);
    i++;

    /* num of satellites */
    location->num_sats = atoi(split_line[i]);
    i++;

    /* hDOP */
    location->hdop = atoi(split_line[i]) * HDOP_FACTOR;
    i++;

    /* Altitude, meters, above sea level */
    if (split_line[i] != NULL)
    {
        location->altitude = atoi(split_line[i]) * ALT_FACTOR;
    }
    i++;

    // M of altitude
    i++;
    // Height of geoid
    i++;
    // time in seconds since last DGPS update
    i++;
    // DGPS station ID num
    i++;
    // checksum data, begins with *
    i++;

    return true;
}

/**************************************************************************//**
 *  Gets a buffer and a GPS_LOCATION_INFO struct and fills it with data
 *  from the buffer.
 *  @param buf - buffer for the GPS.
 *  @param location - the GPS info struct.
 *  @return true if successful, false otherwise.
 *****************************************************************************/
bool parseRawData(char* buf, GPS_LOCATION_INFO* location){
    int result = 0;
    int tokens_array_size = GGA_FIELDS_NUM;
    char* tokens_array[tokens_array_size];
    /* GGA Line */
    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == true)
        {
            result = parseGGA(tokens_array, location);
        }
        return result;
        /* RMC Line */
    } else if (strncmp(buf, RMC_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(RMC_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == true)
        {
            result = parseRMC(tokens_array, location);
        }
        return result;
    } else {
        // unimportant line
        return false;
    }
}

/**************************************************************************//**
 * Updates location with information from GPSGetReadRaw.
 * @param location - the struct to be filled.
 * @return true if successful, false otherwise.
 *****************************************************************************/
bool GPSGetFixInformation(GPS_LOCATION_INFO *location){
    char buf[MAX_NMEA_LEN] = "";
    uint32_t bytes_read;

    bool result = false;
    while (!result) {
        // call GPSGetReadRaw
        bytes_read = GPSGetReadRaw(buf, MAX_NMEA_LEN);
        if (bytes_read > 0) {
            printf("%s\n", buf);//todo del
            result = parseRawData(buf, location);
        }
    }

    return result;
}

/**************************************************************************//**
 * @brief Disable GPS connection.
 *****************************************************************************/
void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisableGPS();
        GPS_INITIALIZED = false;
    }
}

void GPSConvertFixtimeToUnixTime(GPS_LOCATION_INFO * gps_data, char * unix_time) {
    struct tm current_time;
    char hour[3];
    strncpy(hour, gps_data->fixtime, 2);
    char minutes[3];
    strncpy(minutes, &gps_data->fixtime[3], 2);
    char seconds[3];
    strncpy(seconds, &gps_data->fixtime[6], 2);
    char day[3];
    strncpy(day, &gps_data->fixtime[9], 2);
    char month[3];
    strncpy(month, &gps_data->fixtime[12], 2);
    char year[3];
    strncat(year, &gps_data->fixtime[15], 2);



    current_time.tm_hour = atoi(hour);      //hours since midnight – [0, 23]
    current_time.tm_min = atoi(minutes);    //minutes after the hour – [0, 59]
    current_time.tm_sec = atoi(seconds);    //seconds after the minute – [0, 60]
    current_time.tm_mday = atoi(day);       //day of the month – [1, 31]
    current_time.tm_mon = atoi(month) - 1;  //months since January – [0, 11]
    current_time.tm_year = atoi(year) + 100;//years since 1900

    time_t unix_time_stamp = mktime(&current_time);
    sprintf(unix_time, "%lu", unix_time_stamp);
}


/**
 *
 * @param gps_data GPS LOCATION as received from GPS module
 * @param gps_payload where to store result
 * @return length of payload
 */
int GPSGetPayload(GPS_LOCATION_INFO * gps_data, char * iccid, char * gps_payload) {
    char unix_time[35];
    GPSConvertFixtimeToUnixTime(gps_data, unix_time);
//    char iccid[ICCID_BUFFER_SIZE];
//    CellularGetICCID(iccid);
    //latitude= 31.7498445,longitude= 35.1838178,altitude=10,hdop=2,valid_fix=1,num_sats=4 1557086230000000000
    int payload_len = sprintf(gps_payload, GPS_PAYLOAD_FORMAT,
                              iccid, gps_data->latitude, gps_data->longitude, gps_data->altitude,
                              gps_data->hdop, gps_data->valid_fix, gps_data->num_sats, unix_time);

    return payload_len;
}