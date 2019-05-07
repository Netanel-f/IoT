
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "serial_io.h"  // TODO REMOVE
#include "cellular.h"
#include "string.h"

#define MODEM_PORT "COM5"

int main() {
    // Initialize the cellular modems.
    CellularInit(MODEM_PORT);
    // Makes sure it’s responding to AT commands.
    while (!CellularCheckModem()) {
        Delay(10);
    }

    // Finds all available cellular operators.
    int max_operators = 10; //TODO decide
    int num_operators_found = 0;
    OPERATOR_INFO operators_info[max_operators];
    printf("Finding all available cellular operators.\n");
    while (num_operators_found == 0) {
        CellularGetOperators(operators_info, max_operators, &num_operators_found);
    }

    // Tries to register with each one of them (one at a time).
    for (int op_index = 0; op_index <= num_operators_found; op_index++) {
        if (CellularSetOperator(SPECIFIC_OP, operators_info[op_index].operatorName)) {
            // If registered successfully, it prints the signal quality.
            int registration_status = 0;
            // verify registration to operator and check status
            if (CellularGetRegistrationStatus(&registration_status) &&
                (registration_status == 1 || registration_status == 5)) {
                // registration_status == 1: Registered to home network
                // registration_status == 5: Registered,
                // roaming ME is registered at a foreign network (national or international network)
                int signal_quality = -1;
                if (CellularGetSignalQuality(&signal_quality)) {
                    printf("signal quality %d\n", signal_quality);
                }
            }
        }
    }
    // Print the program’s progress all along (e.g. “Checking modem…the modem is ready!”  etc.)
    CellularDisable();
    exit(0);
}

