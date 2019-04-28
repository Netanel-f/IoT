#include "cellular.h"
#include "serial_io.h"

/**
 * Initialize whatever is needed to start working with the cellular modem (e.g. the serial port).
 * @param port
 */
void CellularInit(char *port){
    CELLULAR_INITIALIZED = SerialInit(PORT, BAUD_RATE);
    if (!CELLULAR_INITIALIZED) {
        exit(EXIT_FAILURE);
    }
}


/**
 * Deallocate / close whatever resources CellularInit() allocated.
 */
void CellularDisable(){
    if (CELLULAR_INITIALIZED) {
        SerialDisable();
        CELLULAR_INITIALIZED = false;
    }
}


/**
 * Checks if the modem is responding to AT commands.
 * @return Return true if it does, returns false otherwise.
 */
bool CellularCheckModem(void){
    // send hello (AT\r\n)
    unsigned int hello_size = 5;
    unsigned char hello[5] = "AT\r\n";
    unsigned int input_size = 6;
    unsigned char input_buf[6]; // OK/ERROR
    unsigned int timeout_ms = 100;

    if (!SerialSend(hello, input_size)){
        return false;
    }

    // Wait for response (OK)
    SerialRecv(input_buf, input_size, timeout_ms);
    return (strncmp(input_buf, "OK", 2) == 0);
}


/**
 * @param status
 * @return Returns false if the modem did not respond or responded with an error.
 * Returns true if the command was successful and the registration status was obtained
 * from the modem. In that case, the status parameter will be populated with the numeric
 * value of the <regStatus> field of the “+CREG” AT command.

 */
bool CellularGetRegistrationStatus(int *status){
    return false;
}

/**
 * @param csq
 * @return Returns false if the modem did not respond or responded with an error
 * (note, CSQ=99 is also an error!)
 * Returns true if the command was successful and the signal quality was obtained from the modem.
 * In that case, the csq parameter will be populated with the numeric value between -113dBm and
 * -51dBm.

 */
bool CellularGetSignalQuality(int *csq){
    return false;
}

/**
 * Forces the modem to register/deregister with a network.
 * If mode=0, sets the modem to automatically register with an operator
 * (ignores the operatorName parameter).
 * If mode=1, forces the modem to work with a specific operator, given in operatorName.
 * If mode=2, deregisters from the network (ignores the operatorName parameter).
 * See the “+COPS=<mode>,…” command for more details.
 * @param mode
 * @param operatorName
 * @return Returns true if the command was successful, returns false otherwise.
 */
bool CellularSetOperator(int mode, char *operatorName){

    if (mode == REG_AUTOMATICALLY){

    } else if (mode == SPECIFIC_OP){

    } else if (mode == DEREGISTER) {

    }
}

/**
 * Forces the modem to search for available operators (see “+COPS=?” command).
 * @param opList - a pointer to the first item of an array of type CELLULAR_OP_INFO, which is
 * allocated by the caller of this function.
 * @param maxops - The array contains a total of maxops items.
 * @param numOpsFound - numOpsFound is allocated by the caller and will contain the number
 * of operators found and populated into the opList.
 * @return Returns false if an error occurred or no operators found.
 * Returns true and populates opList and opsFound if the command succeeded.
 */
bool CellularGetOperators(OPERATOR_INFO *opList, int maxops, int *numOpsFound){
    return false;
}

bool sendATcommand(unsigned char* command){
    int command_size = sizeof(command); //TODO: is this ok
    if (!SerialSend(command, command_size)){
        return false;
    }
    Delay(100); // TODO
    return true;
}

bool getATresponse(){
    unsigned char buf[200] = ""; //TODO: change size
    int timeout_ms = 100; // TODO: change/define
    SerialRecv(buf, 200, timeout_ms);
    //TODO: continue
}