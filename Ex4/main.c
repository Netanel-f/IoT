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
#ifdef _WIN64
static char* GPS_PORT = "COM3";//todo check
static char* MODEM_PORT = "COM5";
#elif _WIN32
static char* GPS_PORT = "COM3";
static char* MODEM_PORT = "COM5";
#else
static char* GPS_PORT = "3";
static char* MODEM_PORT = "5";
#endif

#define MAX_IL_CELL_OPS 20
#define NAMES "NetanelFayoumi_SapirElyovitch"
#define TRANSMIT_URL "https://en8wtnrvtnkt5.x.pipedream.net/write?db=mydb"


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
    GPSInit(GPS_PORT);

    //todo free
    GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
    if (last_location == NULL)
    {
        printf("Memory Allocation Error");
        return EXIT_FAILURE;
    }

    //todo if BTN0 pressed:
    bool BTN_flag = true;
    while (BTN_flag) {

        //todo GET GPS DATA
        if (!GPSGetFixInformation(last_location)) {
            // todo do i need only RMC?
            //https://moodle2.cs.huji.ac.il/nu18/mod/forum/discuss.php?d=56363
            // if no GPS data could be retrieved, don't send anything.
            BTN_flag = false;
            break;
        }

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
        OPERATOR_INFO past_registerd_operators[MAX_IL_CELL_OPS];
        int num_of_past_registerd = 0;

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
                    } else if (signal_quality == -1) {
                        printf("Modem didn't respond.\n");
                        operators_info[op_index].csq = 99;
                    } else if (signal_quality == 99) {
                        printf("Current signal quality is UNKNOWN.\n");
                        operators_info[op_index].csq = signal_quality;
                    }

                    // copy operator info to archive
                    memcpy(&past_registerd_operators[num_of_past_registerd++],
                           &operators_info[op_index], sizeof(operators_info[op_index]));
                    continue;
                }
            }
            printf("Modem registration failed.\n");
        }

        // Connects to an available operator (if available, if no, tries again after one minute
        for (int op_index = 0; op_index < num_of_past_registerd; op_index++) {
            // Deregister from current operator
            printf("Deregister from current operator.\n");
            while (!CellularSetOperator(DEREGISTER, NULL));

            // register to operator
            printf("Trying to register with %s...\n", past_registerd_operators[op_index].operatorName);
            if (CellularSetOperator(SPECIFIC_OP, past_registerd_operators[op_index].operatorName)) {

                // If registered successfully, it prints the signal quality.
                int registration_status = 0;

                // verify registration to operator and check status
                if (!(CellularGetRegistrationStatus(&registration_status) &&
                      (registration_status == 1 || registration_status == 5))) {
                    Delay(60000);
                    if (!(CellularGetRegistrationStatus(&registration_status) &&
                          (registration_status == 1 || registration_status == 5))) {
                        continue;
                    }
                }

                // we registered to operator, setup inet connection
                CellularSetupInternetConnectionProfile(60);

                // get CCID
                char iccid[ICCID_BUFFER_SIZE];
                CellularGetICCID(iccid);

                // todo transmit GPS data over HTTP
                // transmit GPS data
                char gps_payload[1000] = "";
                int gps_payload_len = GPSGetPayload(last_location, iccid, gps_payload);
                char transmit_response[100] = "";
                while (CellularSendHTTPPOSTRequest(TRANSMIT_URL, gps_payload, gps_payload_len, transmit_response, 99) == -1);

                // todo transmit CELLULAR data over HTTP
                // transmit cell data
                char unix_time[35];
                GPSConvertFixtimeToUnixTime(last_location, unix_time);

                char cell_payload[1000] = "";
                int cell_payload_len = CellularGetPayload(past_registerd_operators, num_of_past_registerd, unix_time, cell_payload);
                while (CellularSendHTTPPOSTRequest(TRANSMIT_URL, cell_payload, cell_payload_len, transmit_response, 99) == -1);

                break;
            }
        }

        // todo BTN false


    }
    //todo dont disable
    // Print the program’s progress all along (e.g. “Checking modem…the modem is ready!”  etc.)
    printf("Disabling Cellular and exiting...\n");
    CellularDisable();
    free(last_location);//todo
    exit(0);
}





