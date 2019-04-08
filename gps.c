#include "gps.h"

/**
 * Initiate GPS connection.
 */
void GPSInit() {
    GPS_INITIALIZED = SerialInit(PORT, BAUD_RATE);
    if (!GPS_INITIALIZED) {
        exit(EXIT_FAILURE);
    }
}

/**
 * Gets line from GPS.
 * @param buf - output buffer to put the line into.
 * @param maxlen - max length of a line.
 * @return number of bytes received.
 */
uint32_t GPSGetReadRaw(char *buf, unsigned int maxlen) {
    if (GPS_INITIALIZED) {
        return SerialRecv((unsigned char*) buf, maxlen, RECV_TIMEOUT_MS);
    }
    else {
        return 0;
    }
}

/**
 * Gets a GPS line and splits it into tokens.
 * @param num_of_fields - number of fields in the type of line being split.
 * @param prefix_length - the length of the line prefix.
 * @param buf - the buffer.
 * @param tokens_array - the output token array.
 * @return TRUE if successful, FALSE otherwise.
 */
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
    return TRUE;
}

/**
 * Gets array (split_line) with tokens that are RMC's fields,
 * updates location accordingly.
 * @param split_line - RMC line split into tokens.
 * @param location - location struct to be filled.
 * @return TRUE if successful, FALSE otherwise.
 */
bool parseRMC(char** split_line, GPS_LOCATION_INFO *location){
    // only fills date and time in the following order:
    // hhmmssDDMMYY
    if (split_line[RMC_TIME_FIELD] == NULL ||
        split_line[RMC_DATE_FIELD] == NULL){
        return FALSE;
    }
    strcpy(location->fixtime, split_line[RMC_TIME_FIELD]);
    strcpy(location->fixtime+6, split_line[RMC_DATE_FIELD]);
    strcpy(location->prefix, "RMC");
    return TRUE;
}

/**
 * Gets array (split_line) with tokens that are GGA's fields,
 * updates location accordingly.
 * @param split_line - GGA line split into tokens.
 * @param location - location struct to be filled.
 * @return TRUE if successful, FALSE otherwise.
 */
bool parseGGA(char** split_line, GPS_LOCATION_INFO *location){

    // check for empty required fields
    for (int j=0; j<GGA_MIN_REQUIRED_FIELDS; j++){
        if (split_line[j] == NULL){
            return FALSE;
        }
    }

    // counter for the fields in split_line
    int i = 0;
    strcpy(location->prefix, "GGA");

    /* time */ //HHMMSS (UTC)
    strcpy(location->fixtime, split_line[i]);
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

    return TRUE;
}


bool parseRawData(char* buf, GPS_LOCATION_INFO* location){
    int result = 0;
    int tokens_array_size = max(GGA_FIELDS_NUM, RMC_FIELDS_NUM);
    char* tokens_array[tokens_array_size];
    /* GGA Line */
    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == TRUE)
        {
            result = parseGGA(tokens_array, location);
        }
        return result;
        /* RMC Line */
    } else if (strncmp(buf, RMC_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(RMC_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == TRUE)
        {
            result = parseRMC(tokens_array, location);
        }
        return result;
    } else {
        // unimportant line
        return FALSE;
    }
}

/**
 * Updates location with information from GPSGetReadRaw.
 * @param location - the struct to be filled.
 * @return TRUE if successful, FALSE otherwise.
 */
bool GPSGetFixInformation(GPS_LOCATION_INFO *location){

//    char buf[MAX_NMEA_LEN] = "$GPGGA,123519,4807.038,N,07402.499,W,0,00,,,M,,M,,*5C";
//    char buf[MAX_NMEA_LEN] = SAMPLE_LINE;
    char buf[MAX_NMEA_LEN] = "";
    uint32_t bytes_read;

    bool result = FALSE;
    while (!result) {
        // call GPSGetReadRaw
        bytes_read = GPSGetReadRaw(buf, MAX_NMEA_LEN);
        if (bytes_read > 0) {
            result = parseRawData(buf, location);
        }
    }

    return result;

//    int result = 0;
//    int tokens_array_size = max(GGA_FIELDS_NUM, RMC_FIELDS_NUM);
//    char* tokens_array[tokens_array_size];
//    /* GGA Line */
//    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
//        result = splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
//        if (result == 0)
//        {
//            result = parseGGA(tokens_array, location);
//        }
//        return result;
//    /* RMC Line */
//    } else if (strncmp(buf, RMC_PREFIX, PREFIX_LEN) == 0) {
//        result = splitLineToFields(RMC_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
//        if (result == 0)
//        {
//            result = parseRMC(tokens_array, location);
//        }
//        return result;
//    } else {
//        // unimportant line
//        return -1;
//    }
//    return 0;
}

/**
 * Disable GPS connection.
 */
void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
