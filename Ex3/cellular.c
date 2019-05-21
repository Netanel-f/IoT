#include "cellular.h"
#include "serial_io.h"


/****************************************************************************
 * 								DECLARATIONS
*****************************************************************************/
bool sendATcommand(unsigned char* command, unsigned int command_size);
bool waitForOK();
bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response, unsigned int response_size, unsigned int timeout_ms);
int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array);
int splitCopsResponseToOpsTokens(unsigned char * cops_response, OPERATOR_INFO *opList, int max_ops);
bool splitOpTokensToOPINFO(unsigned char * op_token, OPERATOR_INFO *opInfo);


/*****************************************************************************
 * 								DEFS
*****************************************************************************/
#define MODEM_BAUD_RATE 115200
#define MAX_INCOMING_BUF_SIZE 1000
#define MAX_AT_CMD_LEN 100
#define GENERAL_RECV_TIMEOUT_MS 100
#define GET_OPS_TIMEOUT_MS 120000
bool DEBUG = true;// TODO remove

/*****************************************************************************
 * 							GLOBAL VARIABLES
*****************************************************************************/
unsigned char AT_CMD_SUFFIX[] = "\r\n";
// AT_COMMANDS
unsigned char AT_CMD_ECHO_OFF[] = "ATE0\r\n";
unsigned char AT_CMD_AT[] = "AT\r\n";
unsigned char AT_CMD_COPS_TEST[] = "AT+COPS=?\r\n";
const unsigned char AT_CMD_COPS_WRITE_PREFIX[] = "AT+COPS=";
unsigned char AT_CMD_CREG_READ[] = "AT+CREG?\r\n";
unsigned char AT_CMD_CSQ[] = "AT+CSQ\r\n";
unsigned char AT_CMD_SHUTDOWN[] = "AT^SMSO\r\n";

// AT RESPONDS
unsigned char AT_RES_OK[] = "OK";
unsigned char AT_RES_ERROR[] = "ERROR";
unsigned char AT_RES_SYSSTART[] = "^SYSSTART";
unsigned char AT_RES_PBREADY[] = "+PBREADY";
unsigned char AT_URC_SHUTDOWN[] = "^SHUTDOWN";



/**
 * Initialize whatever is needed to start working with the cellular modem (e.g. the serial port).
 * @param port
 */
void CellularInit(char *port){
    printf("Initializing Cellular modem... ");

    if (!CELLULAR_INITIALIZED) {
        CELLULAR_INITIALIZED = SerialInit(port, MODEM_BAUD_RATE);
        if (!CELLULAR_INITIALIZED) {
            printf("Initialization FAILED\n");
            exit(EXIT_FAILURE);
        }

        // check modem responded with ^+PBREADY
        unsigned char * token_array[10] = {};
        waitForATresponse(token_array, AT_RES_PBREADY, sizeof(AT_RES_PBREADY) - 1, GENERAL_RECV_TIMEOUT_MS);

        bool echo_off = false;
        printf("\nturning echo off... ");
        while (!echo_off) {
            // set modem echo off
            while (!sendATcommand(AT_CMD_ECHO_OFF, sizeof(AT_CMD_ECHO_OFF) - 1));

            // verify echo off
            echo_off = waitForOK();
        }

        printf("successfully.\n");
        printf("\nCellular modem initialized successfully.\n");
    }
}


/**
 * Deallocate / close whatever resources CellularInit() allocated.
 */
void CellularDisable(){
    if (CELLULAR_INITIALIZED) {
        // shut down modem
        while (!sendATcommand(AT_CMD_SHUTDOWN, sizeof(AT_CMD_SHUTDOWN) - 1));

        // Disable serial connection
        SerialDisable();
        CELLULAR_INITIALIZED = false;
    }
}


/**
 * Checks if the modem is responding to AT commands.
 * @return Return true if it does, returns false otherwise.
 */
bool CellularCheckModem(void){
    printf("\nChecks that the modem is responding... ");
    if (CELLULAR_INITIALIZED) {
        // send "hello" (AT\r\n)
        while (!sendATcommand(AT_CMD_AT, sizeof(AT_CMD_AT) - 1));

        // verify modem response
        if (waitForOK()) {
            printf("modem is responsive.\n");
            return true;
        } else {
            printf("modem is NOT responsive!\n");
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
 * value of the <regStatus> field of the "+CREG" AT command.

 */
bool CellularGetRegistrationStatus(int *status){
    // AT+CREG?
    // response: +CREG: <Mode>, <regStatus>[, <netLac>, <netCellId>[, <AcT>]] followed by OK
    if (sendATcommand(AT_CMD_CREG_READ, sizeof(AT_CMD_CREG_READ) - 1)) {
        unsigned char * token_array[5] = {};
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, GENERAL_RECV_TIMEOUT_MS)) {
            // "+CREG: <Mode>,<regStatus>"
            char * token;
            token = strtok(token_array[0], ",");
            token = strtok(NULL, ",");
            *status = atoi(token);
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
bool CellularGetSignalQuality(int *csq) {
    if (DEBUG) { printf("\n*** Sending: AT+CSQ ..."); }
    if (!CELLULAR_INITIALIZED) {
        return false;
    }
    // AT+CSQ
    // response : +CSQ <rssi>,<ber> followed by OK
    // rssi: 0,1,2-30,31,99, ber: 0-7,99unknown

    // send AT+CSQ
    if (sendATcommand(AT_CMD_CSQ, sizeof(AT_CMD_CSQ) - 1)) {
        if (DEBUG) { printf(" sent ***\n"); }
        unsigned char * token_array[5] = {};
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, GENERAL_RECV_TIMEOUT_MS)) {
            char * token;
            token = strtok(&(token_array[0])[5], ",");
            if (strcmp(token, "99") != 0) {
                // -113 + 2* rssi
                int rssi = atoi(token);
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
 * See the "+COPS=<mode>,..." command for more details.
 * @param mode
 * @param operatorName
 * @return Returns true if the command was successful, returns false otherwise.
 */
bool CellularSetOperator(int mode, char *operatorName){
    // 1: Manual operator selection.
    // Write command requires <opName> in numeric format, i.e. <format> shall be 2.
    // 2: Manually deregister from network and remain unregistered until <mode>=0 or
    // 1 or 4 is selected. Please keep in mind that the AT+COPS write command is SIM PIN protected.
    // When SIM PIN is disabled or after SIM PIN authentication has completed and
    // "+PBREADY" URC has shown up the power up default <mode>=2 automatically
    // changes to <mode>=0, causing the ME to select a network.

    if (DEBUG) { printf("\nSetting operator mode: %d and operatorName is: %s\n", mode, operatorName); }
    unsigned char command_to_send[MAX_AT_CMD_LEN] = "";
    memset(command_to_send, '\0',MAX_AT_CMD_LEN);
    if (mode == REG_AUTOMATICALLY || mode == DEREGISTER) {
        int cmd_size = sprintf(command_to_send, "%s%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, AT_CMD_SUFFIX);

        // send command
        if (DEBUG) { printf("\n***CSO- Sending cmd: %s ** Size:%d... ", command_to_send, cmd_size); }
        while(!sendATcommand(command_to_send, cmd_size - 1));
        if (DEBUG) { printf("sent ***\n"); }
        // wait for OK
        return waitForOK();

    } else if (mode == SPECIFIC_OP) {

        int act = 0;
        int cmd_size = sprintf(command_to_send, "%s%d,0,%s,%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, operatorName, act, AT_CMD_SUFFIX);
        if (DEBUG) { printf("\n***CSO- Sending cmd: %s ** Size:%d... ", command_to_send, cmd_size); }
        while(!sendATcommand(command_to_send, cmd_size - 1));
        if (DEBUG) { printf("sent ***\n"); }
        if (waitForOK()){
            return true;
        } else {
            act = 2;
            cmd_size = sprintf(command_to_send, "%s%d,0,%s,%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, operatorName, act, AT_CMD_SUFFIX);
            if (DEBUG) { printf("\n***CSO- Sending cmd: %s ** Size:%d... ", command_to_send, cmd_size); }
            while(!sendATcommand(command_to_send, cmd_size - 1));
            if (DEBUG) { printf("sent ***\n"); }
            return waitForOK();
        }
    } else {
        printf("invalid mode!\n");
        return false;
    }
//    // wait for ok
//    return waitForOK();
}

/**
 * Forces the modem to search for available operators (see "+COPS=?" command).
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
    if (DEBUG) { printf("\n*** CGO- Sending: AT+COPS=? ..."); }
    while (!sendATcommand(AT_CMD_COPS_TEST, sizeof(AT_CMD_COPS_TEST) - 1));
    if (DEBUG) { printf("sent. ***\n"); }

    unsigned char * token_array[10] = {};

    if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, GET_OPS_TIMEOUT_MS)) {

        char operators[MAX_INCOMING_BUF_SIZE];
        memset(operators, '\0', MAX_INCOMING_BUF_SIZE);
        // this remove "+COPS: "
        strncpy(operators, &(token_array[0])[7], strlen(token_array[0]) - 7);

        int num_of_found_ops = splitCopsResponseToOpsTokens(operators, opList, maxops);
        // fill results
        if (DEBUG) { printf("*** found %d ops", num_of_found_ops); }
        if (num_of_found_ops != 0) {
            *numOpsFound = num_of_found_ops;
            return true;
        }
    }

    return false;
}


bool sendATcommand(unsigned char* command, unsigned int command_size){
    if (!SerialSend(command, command_size)){
        return false;
    }
    return true;
}


bool waitForOK() {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    memset(incoming_buffer, '\0', MAX_INCOMING_BUF_SIZE);
    unsigned char * token_array[10] = {};
    int num_of_tokens = 0;

    do {
        if (DEBUG) { printf("\n*** wait for OK ***\n"); }
        if (DEBUG) { printf("\n*** incoming buf: %s ***\n", incoming_buffer); }
        SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, GENERAL_RECV_TIMEOUT_MS);
        if (DEBUG) { printf("\n*** incoming buf: %s ***\n", incoming_buffer); }
        num_of_tokens = splitBufferToResponses(incoming_buffer, token_array);
        if (DEBUG) { printf("***waitForOK--- token: %s\n", token_array[num_of_tokens-1]); }
    } while (memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) != 0 &&
             memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, sizeof(AT_RES_ERROR) - 1) != 0);


    return memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) == 0;
}


bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response, unsigned int response_size, unsigned int timeout_ms) {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned char temp_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned int bytes_received = 0;
    int num_of_tokens = 0;


    do {
        if (DEBUG) { printf("\n *** waiting for AT response ***\n"); }
        memset(incoming_buffer, '\0', bytes_received);
        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timeout_ms);
//        if (bytes_received == SERIAL_TIMEOUT) { //TODO check
//            return false;
//        }
        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
        num_of_tokens = splitBufferToResponses(temp_buffer, token_array);

        if (DEBUG) { printf("\n *** token_array[num_of_tokens-1]: %s ***\n", token_array[num_of_tokens-1]); }

    } while (memcmp(token_array[num_of_tokens-1], expected_response, response_size) != 0 &&
             memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, response_size) != 0);

    //num_of_tokens = splitBufferToResponses(temp_buffer, token_array);
    return memcmp(token_array[num_of_tokens-1], expected_response, response_size) == 0;
}


int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array) {
    if (DEBUG) { printf("\n*** splitBufferToResponses ***\n"); }
    const char delimiter[] = "\r\n";
    char * token;
    int i=0;
    token = strtok(buffer, delimiter);
    while (token != NULL) {
        tokens_array[i++] = (unsigned char *)token;
        token = strtok(NULL, delimiter);
    }
    return i;
}

int splitCopsResponseToOpsTokens(unsigned char *cops_response, OPERATOR_INFO *opList, int max_ops) {
    if (DEBUG) { printf("\n*** splitCopsResponseToOpsTokens ***\n"); }
    // [list of supported (<opStatus>, long alphanumeric <opName>, short alphanumeric <opName>,
    //numeric <opName>, <AcT>)s ]
    const char op_start_delimiter[] = "(";
    char * operator_token;
    int op_index = 0;
    operator_token = strtok(cops_response, op_start_delimiter);
    unsigned char * operators_tokens[max_ops];

    for (int i=0; i < max_ops; i++) {
        operators_tokens[i] = (unsigned char *) malloc(MAX_INCOMING_BUF_SIZE);
    }

    while (operator_token != NULL) {
        if (op_index >= max_ops) {
            // found max operators
            break;
        }

        // set char[] for helper method
        strcpy(operators_tokens[op_index], operator_token);
        operator_token = strtok(NULL, op_start_delimiter);
        op_index++;
    }

    int parsed_ops_index = 0;
    for (; op_index > 0; op_index--) {
        if (splitOpTokensToOPINFO(operators_tokens[parsed_ops_index], &opList[parsed_ops_index])) {
            parsed_ops_index++;
        }

    }
    return parsed_ops_index;
}


bool splitOpTokensToOPINFO(unsigned char * op_token, OPERATOR_INFO *opInfo) {
    if (DEBUG) { printf("\n*** splitOpTokensToOPINFO ***\n"); }

    const char op_field_delimiter[] = ",";
    int field_index = 0;
    unsigned char * field_token = strtok(op_token, op_field_delimiter);

    while (field_token != NULL) {
        if (field_index == 1) {
            // long alphanumeric <opName>
            memset(opInfo->operatorName, '\0', 20);
            strcpy(opInfo->operatorName, field_token);

        } else if (field_index == 3) {
            //numeric <opName>
            opInfo->operatorCode = atoi(&field_token[1]);

        } else if (field_index == 4) {
            memset(opInfo->accessTechnology, '\0', 4);
            if (field_token[0] == '0') {
                strcpy(opInfo->accessTechnology, "2G");
            } else if (field_token[0] == '2') {
                strcpy(opInfo->accessTechnology, "3G");
            } else {
                return false;
            }
        }

        field_token = (unsigned char *) strtok(NULL, op_field_delimiter);
        field_index++;
    }

    return true;
}