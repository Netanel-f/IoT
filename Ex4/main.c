#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include "cellular.h"
#include "gps.h"
#include "string.h"
#include "time.h"

/*****************************************************************************
 * 								DEFS
*****************************************************************************/
#define MODEM_PORT "COM5"
#define MAX_IL_CELL_OPS 20
#define NAMES "NetanelFayoumi_SapirElyovitch"


int main() {

    //TODO Main tasks:
    // print progress always.
    // ---PART 1---
    // when BTN0 is pressed
    //   Get GPS data
    //   Init modem
    //   verify modem is responding
    //   find all cell ops.
    //   try register each one
    //      if succeed print csq
    //      connect [init inet conn.] to an available op. (if available, if not tries again after one minute)
    //      transmit the GPS data over HTTP
    //      transmit the cellular data over HTTP
    // ---PART 2---
    // click on BTN1:
    //   start track GPS every 5 seconds.
    //   use touch slider to define speed limit left:0,5,10,15,25,right:30 km/h
    //   if speed > limit:
    //      transmit current GPS cords + tag: highspeed = true
    //   if we passed the limit but now we are ok:
    //      transmit current GPS cords + tag: highspeed = false
    //   if BTN0 click go back to part 1.


    // setting stdout to flush prints immediately
    setbuf(stdout, NULL);


    // Initialize the GPS.
    GPSInit();

    // Initialize the cellular modems.
    CellularInit(MODEM_PORT);

    // Makes sure it’s responding to AT commands.
    // looping to check modem responsiveness.
    while (!CellularCheckModem()) {
        Sleep(10);
    }

    // Setting modem to deregister and remain unregistered.
    while (!CellularSetOperator(DEREGISTER, NULL));

    // Finds all available cellular operators.
    int num_operators_found = 0;
    OPERATOR_INFO operators_info[MAX_IL_CELL_OPS];
    printf("Finding all available cellular operators...");
    while (!CellularGetOperators(operators_info, MAX_IL_CELL_OPS, &num_operators_found)) {
        printf(".");
    }
    printf("found %d operators.\n", num_operators_found);

    // Tries to register with each one of them (one at a time).
    printf("Trying to register with each one of them (one at a time)\n");
    for (int op_index = 0; op_index < num_operators_found; op_index++) {
        // Deregister from current operator
        printf("Deregister from current operator.\n");
        while (!CellularSetOperator(DEREGISTER, NULL));

        // register to operator
        printf("Trying to register with %s...\n", operators_info[op_index].operatorName);
        if (CellularSetOperator(SPECIFIC_OP, operators_info[op_index].operatorName)) {

            // If registered successfully, it prints the signal quality.
            int registration_status = 0;

            // verify registration to operator and check status
            if (CellularGetRegistrationStatus(&registration_status) &&
                (registration_status == 1 || registration_status == 5)) {
                printf("Modem registered successfully.\n");
                // registration_status == 1: Registered to home network
                // registration_status == 5: Registered,
                // roaming ME is registered at a foreign network (national or international network)
                int signal_quality = -1;
                if (CellularGetSignalQuality(&signal_quality)) {
                    printf("Current signal quality: %ddBm\n", signal_quality);
                    operators_info[op_index].csq = signal_quality;
                } else if (signal_quality == -1){
                    printf("Modem didn't respond.\n");
                    operators_info[op_index].csq = 99;
                } else if (signal_quality == 99) {
                    printf("Current signal quality is UNKNOWN.\n");
                    operators_info[op_index].csq = signal_quality;
                }
                continue;
            }
        }
        printf("Modem registration failed.\n");
    }

    // Print the program’s progress all along (e.g. “Checking modem…the modem is ready!”  etc.)
    printf("Disabling Cellular and exiting...\n");
    CellularDisable();
    exit(0);
}


/**
 * return gps payload string lat=,long=,alt=,hdop=,valid_fix=,num_sats= unix_timestamp
 * @param gps_payload where to store result
 * @return length of payload
 */
int GPSGetPayload(GPS_LOCATION_INFO * gps_data, char * gps_payload) {
    char unix_time[35];
    GPSConvertFixtimeToUnixTime(gps_data, unix_time);
    char iccid[ICCID_BUFFER_SIZE];
    CellularGetICCID(iccid);
    //latitude= 31.7498445,longitude= 35.1838178,altitude=10,hdop=2,valid_fix=1,num_sats=4 1557086230000000000
    int payload_len = sprintf(gps_payload, "--data-binary gps,name=%s,ICCID=%s latitude=%u,longitude= %u,altitude=%u,hdop=%u,valid_fix=%u,num_sats=%u %s",
            NAMES, iccid, gps_data->latitude, gps_data->longitude, gps_data->altitude, gps_data->hdop, gps_data->valid_fix, gps_data->num_sats, unix_time);
    return payload_len;
}

/**
 *
 * @param opList OPERATOR_INFO array of successfully registerd operators.
 * @param num_of_ops num of operators in array
 * @param unix_time unix time provided from gps
 * @param cell_payload buffer to store payload into.
 * @return length of payload copied to buffer.
 */
int CellularGetPayload(OPERATOR_INFO *opList, int num_of_ops, char * unix_time, char * cell_payload) {
    char iccid[ICCID_BUFFER_SIZE];
    CellularGetICCID(iccid);

    char operators_payload[300] = "";

    for (int op_idx = 0; op_idx < num_of_ops; op_idx++) {
        char current_op[30] = "";
        sprintf(current_op, "opt%dname=%d,opt%dcsq=%d",
                op_idx, opList[op_idx].operatorCode, op_idx, opList[op_idx].csq);

        strcat(operators_payload, current_op);
        if (op_idx != num_of_ops -1) {
            strcat(operators_payload, ",");
        }
    }

    //8935201641400948300 opt1name= 42501,opt1csq=17,opt2name=42502,opt2csq=5,opt3name=42503,opt3csq=28 1557086230000000000
    int payload_len = sprintf(cell_payload, "--data-binary cellular,name=%s,ICCID=%s %s %s",
                              NAMES, iccid, operators_payload, unix_time);
    return payload_len;
}

