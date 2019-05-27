#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#include "cellular.h"
#include "gps.h"
#include "string.h"

/*****************************************************************************
 * 								DEFS
*****************************************************************************/
#define MODEM_PORT "COM5"
#define MAX_IL_CELL_OPS 20


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
    //   if BTN0 clieck go back to park 1.


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
                } else if (signal_quality == -1){
                    printf("Modem didn't respond.\n");
                } else if (signal_quality == 99) {
                    printf("Current signal quality is UNKNOWN.\n");
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

