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

bool splitLineToFields(int num_of_fields, int prefix_length, char
buf[MAX_NMEA_LEN], char**
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

bool parseRMC(char** split_line, GPS_LOCATION_INFO *location){
    // only fills date and time in the following order:
    // hhmmssDDMMYY
    strcpy(location->fixtime, split_line[RMC_TIME_FIELD]);
    strcpy(location->fixtime+6, split_line[RMC_DATE_FIELD]);
    return 0;
}

bool parseGGA(char** split_line, GPS_LOCATION_INFO *location){
    for (int j=0; j<GGA_MIN_FIELDS; j++){
        if (split_line[j] == NULL){
            return -1;
        }
    }
    int i = 0;
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
    // Longtitude
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
    // fix quality
    location->valid_fix = atoi(split_line[i]);
    i++;
    // num of satellites
    location->num_sats = atoi(split_line[i]);
    i++;
    // hDOP
    location->hdop = atoi(split_line[i])*5;
    i++;
    // Altitude, meters, above sea level
    // *10^2
    if (split_line[i] != NULL)
    {
        location->altitude = atoi(split_line[i]) * 100;
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
    return 0;
}

bool GPSGetFixInformation(GPS_LOCATION_INFO *location){
    // call GPSGetReadRaw
//    char buf[MAX_NMEA_LEN] = "";
//    uint32_t bytes_read = GPSGetReadRaw(buf, MAX_NMEA_LEN);

    // for testing:

//    char buf[MAX_NMEA_LEN] = "$GPGGA,123519,4807.038,N,07402.499,W,0,00,,,M,,M,,*5C";
    char buf[MAX_NMEA_LEN] = SAMPLE_LINE;

    int result = 0;
    int tokens_array_size = max(GGA_FIELDS_NUM, RMC_FIELDS_NUM);
    char* tokens_array[tokens_array_size];
    if (strncmp(buf, GGA_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(GGA_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == 0)
        {
            result = parseGGA(tokens_array, location);
        }
        return result;
    } else if (strncmp(buf, RMC_PREFIX, PREFIX_LEN) == 0) {
        result = splitLineToFields(RMC_FIELDS_NUM, PREFIX_LEN, buf, tokens_array);
        if (result == 0)
        {
            result = parseRMC(tokens_array, location);
        }
        return result;
    } else {
        // unimportant line
        return -1; 
    }

    return 0;
    }


void GPSDisable() {
    if (GPS_INITIALIZED) {
        SerialDisable();
        GPS_INITIALIZED = FALSE;
    }
}
