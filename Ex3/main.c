
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "serial_io.h"  // TODO REMOVE
#include "cellular.h"
#include "string.h"

#define MODEM_PORT "COM5"
#define MAX_IL_CELL_OPS 10  //TODO check 10 is enough

//TODO print program's progress
int main() {
    // Initialize the cellular modems.
    CellularInit(MODEM_PORT);

    // Makes sure it’s responding to AT commands.
    // looping to check modem responsiveness.
    while (!CellularCheckModem()) {
        Delay(10);
    }

    // Setting modem to deregister and remain unregistered.
    while (!CellularSetOperator(DEREGISTER, NULL));

    // Finds all available cellular operators.
    int num_operators_found = 0;
    printf("~~~ #ops found: %d ~~~", num_operators_found);
    OPERATOR_INFO operators_info[MAX_IL_CELL_OPS];
    printf("Finding all available cellular operators...");
    while (!CellularGetOperators(operators_info, MAX_IL_CELL_OPS, &num_operators_found)) {
        printf("...");
    }
    printf("\n");

    // Tries to register with each one of them (one at a time).
    printf("Trying to register with each one of them (one at a time)\n");
    for (int op_index = 0; op_index <= num_operators_found; op_index++) {
//        // Deregister from current operator TODO check is this needed?
//        while (!CellularSetOperator(DEREGISTER, NULL));

        // register to operator
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
                    printf("Current signal quality: %d\n", signal_quality);
                    continue;
                }
            }
        }
        printf("Modem registration failed.\n");
    }
    // Print the program’s progress all along (e.g. “Checking modem…the modem is ready!”  etc.)
    CellularDisable();
    exit(0);
}

