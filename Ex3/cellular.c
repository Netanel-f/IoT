#include "cellular.h"
#include "serial_io.h"

bool sendATcommand(unsigned char* command, unsigned int command_size); //TODO declare
bool waitForOK();
bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response, unsigned int response_size);
int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array);
/**************************************************************************//**
 * 								DEFS
*****************************************************************************/
#define MODEM_BAUD_RATE 115200
#define MAX_INCOMING_BUF_SIZE 1000


/*****************************************************************************
 * 							GLOBAL VARIABLES
*****************************************************************************/
unsigned int recv_timeout_ms = 100;

unsigned char AT_CMD_SUFFIX[] = "\r\n";
// AT_COMMANDS
unsigned char AT_CMD_ECHO_OFF[] = "ATE0\r\n";
unsigned char AT_CMD_AT[] = "AT\r\n";
unsigned char AT_CMD_COPS_TEST[] = "AT+COPS=?\r\n";
const unsigned char AT_CMD_COPS_WRITE_PREFIX[] = "AT+COPS=";
unsigned char AT_CMD_CREG_READ[] = "AT+CREG?\n";
unsigned char AT_CMD_CSQ[] = "AT+CSQ\r\n";
unsigned char AT_CMD_SHUTDOWN[] = "AT^SMSO\r\n";

// AT RESPONDS
unsigned char AT_RES_OK[] = "OK";
unsigned char AT_RES_ERROR[] = "ERROR";
unsigned char AT_RES_SYSSTART[] = "^SYSSTART";
unsigned char AT_RES_PBREADY[] = "+PBREADY";




/**
 * Initialize whatever is needed to start working with the cellular modem (e.g. the serial port).
 * @param port
 */
void CellularInit(char *port){
    printf("Initializing Cellular... ");

    if (!CELLULAR_INITIALIZED) {
        CELLULAR_INITIALIZED = SerialInit(port, MODEM_BAUD_RATE);
        if (!CELLULAR_INITIALIZED) {
            printf("Initialization FAILED\n");
            exit(EXIT_FAILURE);
        }


        // check modem responded with ^+PBREADY
        unsigned char * token_array[10] = {};    //Todo set 10 as MAGIC
        waitForATresponse(token_array, AT_RES_PBREADY, sizeof(AT_RES_PBREADY) - 1);

        bool echo_off = false;
        printf("turning echo off... ");
        while (!echo_off) {
            // set modem echo off
            while (!sendATcommand(AT_CMD_ECHO_OFF, sizeof(AT_CMD_ECHO_OFF) - 1));

            // verify echo off
            echo_off = waitForOK();
        }

        printf("Initialization SUCCEEDED\n");
        //TODO maybe need timeout if device is OFF?
    }
}


/**
 * Deallocate / close whatever resources CellularInit() allocated.
 */
void CellularDisable(){
    if (CELLULAR_INITIALIZED) {
        // shut down modem TODO do we need to shutdown the modem?
        while (!sendATcommand(AT_CMD_SHUTDOWN, sizeof(AT_CMD_SHUTDOWN) - 1));

        // verify modem off
        waitForOK(); //TODO NEED TO CHECK FOR ERROR

        SerialDisable();
        CELLULAR_INITIALIZED = false;
    }
}


/**
 * Checks if the modem is responding to AT commands.
 * @return Return true if it does, returns false otherwise.
 */
bool CellularCheckModem(void){
    printf("Checking modem... ");
    if (CELLULAR_INITIALIZED) {
        // send "hello" (AT\r\n)
        while (!sendATcommand(AT_CMD_AT, sizeof(AT_CMD_AT) - 1));

        // verify modem response
//        if (waitForATresponse(AT_RES_OK, sizeof(AT_RES_OK) - 1)) {
        if (waitForOK()) {
            printf("modem is ready!\n");
            return true;
        } else {
            printf("modem is NOT ready!\n");
            return false;
        }
    } else {
        printf("modem wasn't initialized!\n");
        return false;
    }
}


/**
 * @param status
 * @return Returns false if the modem did not respond or responded with an error.
 * Returns true if the command was successful and the registration status was obtained
 * from the modem. In that case, the status parameter will be populated with the numeric
 * value of the <regStatus> field of the “+CREG” AT command.

 */
bool CellularGetRegistrationStatus(int *status){
    // AT+CREG?
    // response: +CREG: <Mode>, <regStatus>[, <netLac>, <netCellId>[, <AcT>]] followed by OK
    if (sendATcommand(AT_CMD_CREG_READ, sizeof(AT_CMD_CREG_READ) - 1)) {
        unsigned char * token_array[5] = {};
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1)) {
            // TODO SPLIT TOKEN ARRAY BY ,
            *status = atoi(token_array[1]);
            return true;
        }
    }
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
    // TODO after init and registration
    // AT+CSQ
    // response : +CSQ <rssi>,<ber> followed by OK
    // rssi: 0,1,2-30,31,99, ber: 0-7,99unknown

    // send AT+CSQ
    if (DEBUG) { printf("Sending: AT+CSQ ..."); }
    if (sendATcommand(AT_CMD_CSQ, sizeof(AT_CMD_CSQ) - 1)) {
        if (DEBUG) { printf(" sent\n"); }
        unsigned char * token_array[5] = {};
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1)) {
            // TODO SPLIT TOKEN ARRAY BY ,
            if (strcmp(token_array[0], "99") != 0) {
                // -113 + 2* rssi
                int rssi = atoi(token_array[0]);

                *csq = -113 + (2 * rssi);
                return true;
            }
        }
    }
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
    // TODO: on init we should set  AT+COPS=1 / 2
    // 1: Manual operator selection.
    // Write command requires <opName> in numeric format, i.e. <format> shall be 2.
    // 2: Manually deregister from network and remain unregistered until <mode>=0 or
    // 1 or 4 is selected. Please keep in mind that the AT+COPS write command is SIM PIN protected.
    // When SIM PIN is disabled or after SIM PIN authentication has completed and
    // "+PBREADY" URC has shown up the powerup default <mode>=2 automatically
    // changes to <mode>=0, causing the ME to select a network.

    //TODO: AT+COPS? will return something like:
    // +COPS: 0,0,"IL Pelephone",2
    //
    // OK
    unsigned char command_to_send[] = ""; //TODO magic
    if (mode == REG_AUTOMATICALLY || mode == DEREGISTER){
        sprintf(command_to_send, "%s%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, AT_CMD_SUFFIX);

        // send command
        if (DEBUG) { printf("Sending: %s ... ", command_to_send); }
        while(!sendATcommand(command_to_send, sizeof(command_to_send) - 1));
        if (DEBUG) { printf("sent\n"); }

    } else if (mode == SPECIFIC_OP){

        int act = 0;// TODO check for ACT
        sprintf(command_to_send, "%s%d,0,%s,%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, operatorName, act, AT_CMD_SUFFIX);
        if (DEBUG) { printf("Sending: %s ... ", command_to_send); }
        while(!sendATcommand(command_to_send, sizeof(command_to_send) - 1));
        if (DEBUG) { printf("sent\n"); }
    }
    // wait for ok
    return waitForOK();
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
    // send AT+COPS=?
    if (DEBUG) { printf("Sending: AT+COPS=? ..."); }
    while (!sendATcommand(AT_CMD_COPS_TEST, sizeof(AT_CMD_COPS_TEST) - 1));
    if (DEBUG) { printf("sent.\n"); }

    unsigned char * token_array[10] = {};    //Todo set 10 as MAGIC
    if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1)) {

        // TODO parse "+COPS: (....) AND TIMEOUT
        // "+COPS: (1,\"Orange IL\",\"OrangeIL\",\"42501\",2),(1,\"IL Pelephone\",\"PCL\",\"42503\",2),(1,\"\",\"\",\"42507\",2),(1,\"Orange IL\",\"OrangeIL\",\"42501\",0),(1,\"JAWWAL-PALESTINE\",\"JAWWAL\",\"42505\",0)"
        // fill results
        return true;

    } else {
        return false;
    }
}

bool sendATcommand(unsigned char* command, unsigned int command_size){
    if (!SerialSend(command, command_size)){
        return false;
    }
//    Delay(100); // TODO
    return true;
}

bool waitForOK() {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    memset(incoming_buffer, 0, MAX_INCOMING_BUF_SIZE);
    unsigned char * token_array[10] = {};    //Todo set 10 as MAGIC
    int num_of_tokens = 0;

    do {
        SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, recv_timeout_ms);
        num_of_tokens = splitBufferToResponses(incoming_buffer, token_array);
    } while (memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) != 0 &&
             memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, sizeof(AT_RES_ERROR) - 1) != 0);

    return memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) == 0;
}


bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response, unsigned int response_size) {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned char temp_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned int bytes_received = 0;
    int num_of_tokens = 0;


    do {
        memset(incoming_buffer, 0, MAX_INCOMING_BUF_SIZE);
        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, recv_timeout_ms);
        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
        num_of_tokens = splitBufferToResponses(incoming_buffer, token_array);

    } while (memcmp(token_array[num_of_tokens-1], expected_response, response_size) != 0 &&
             memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, response_size) != 0);

    num_of_tokens = splitBufferToResponses(temp_buffer, token_array);
    return memcmp(token_array[num_of_tokens-1], expected_response, response_size) == 0;
}


bool getATresponse(){
    unsigned char buf[200] = ""; //TODO: change size
    int timeout_ms = 100; // TODO: change/define
    SerialRecv(buf, 200, timeout_ms);
    //TODO: continue
}


int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array) {
    const char delimiter[] = "\r\n";
    char * token;
    int i=0;
    token = strtok(buffer, delimiter);
    while (token != NULL) {
        tokens_array[i++] = (unsigned char *)token; //TODO
        token = strtok(NULL, delimiter);
    }
    return i;
}