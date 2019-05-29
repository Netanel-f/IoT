#include "cellular.h"
#include "serial_io.h"


/****************************************************************************
 * 								DECLARATIONS
*****************************************************************************/
bool sendATcommand(unsigned char* command, unsigned int command_size);
bool waitForOK();
bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response, unsigned int response_size, int max_responses, unsigned int timeout_ms);
int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array, int max_tokens);
int splitCopsResponseToOpsTokens(unsigned char * cops_response, OPERATOR_INFO *opList, int max_ops);
bool splitOpTokensToOPINFO(unsigned char * op_token, OPERATOR_INFO *opInfo);

/*****************************************************************************
 * 								DEFS
*****************************************************************************/
#define MODEM_BAUD_RATE 115200
#define MAX_INCOMING_BUF_SIZE 1000
#define MAX_AT_CMD_LEN 100
#define GENERAL_RECV_TIMEOUT_MS 100
#define GENERAL_RECV_DLY_TIMEOUT_MS 150
#define GET_OPS_TIMEOUT_MS 120000
#define MAX_conProfileId 5
//#define MAX_srvProfileId 9
#define HTTP_POST_srvProfileId 6

#define SISS_CMD_HTTP_GET 0
#define SISS_CMD_HTTP_POST 1
#define SISS_CMD_HTTP_HEAD 2

#define RESPONSE_TOKENS_SIZE 10


/*****************************************************************************
 * 							GLOBAL VARIABLES
*****************************************************************************/

unsigned char command_to_send_buffer[MAX_AT_CMD_LEN] = "";  //todo
unsigned char * response_tokens[10] = {};
unsigned int num_of_tokens_found = 0;

unsigned char AT_CMD_SUFFIX[] = "\r\n";
// AT_COMMANDS
unsigned char AT_CMD_ECHO_OFF[] = "ATE0\r\n";
unsigned char AT_CMD_AT[] = "AT\r\n";
unsigned char AT_CMD_COPS_TEST[] = "AT+COPS=?\r\n";
const unsigned char AT_CMD_COPS_WRITE_PREFIX[] = "AT+COPS=";
unsigned char AT_CMD_CREG_READ[] = "AT+CREG?\r\n";
unsigned char AT_CMD_CSQ[] = "AT+CSQ\r\n";
unsigned char AT_CMD_SICS_WRITE_PRFX[] = "AT^SICS=";
unsigned char AT_CMD_SISS_WRITE_PRFX[] = "AT^SISS=";
unsigned char AT_CMD_SISO_WRITE_PRFX[] = "AT^SISO=";
unsigned char AT_CMD_SISR_WRITE_PRFX[] = "AT^SISR=";
unsigned char AT_CMD_SISW_WRITE_PRFX[] = "AT^SISW=";
unsigned char AT_CMD_SISC_WRITE_PRFX[] = "AT^SISC=";
unsigned char AT_CMD_SISE_WRITE_PRFX[] = "AT^SISE=";
unsigned char AT_CMD_CCID_READ[] = "AT+CCID?";
unsigned char AT_CMD_SHUTDOWN[] = "AT^SMSO\r\n";

// AT RESPONDS
unsigned char AT_RES_OK[] = "OK";
unsigned char AT_RES_ERROR[] = "ERROR";
unsigned char AT_RES_CME[] = "+CME";
unsigned char AT_RES_SYSSTART[] = "^SYSSTART";
unsigned char AT_RES_PBREADY[] = "+PBREADY";
unsigned char AT_URC_SHUTDOWN[] = "^SHUTDOWN";

// Internet connection profile identifier. 0..5
// The <conProfileId> identifies all parameters of a connection profile,
// and, when a service profile is created with AT^SISS the <conProfileId>
// needs to be set as "conId" value of the AT^SISS parameter <srvParmTag>.
int conProfileId = -1;

// Internet service profile identifier.0..9
// The <srvProfileId> is used to reference all parameters related to the same service profile. Furthermore,
// when using the AT commands AT^SISO, AT^SISR, AT^SISW, AT^SIST, AT^SISH and AT^SISC the
//<srvProfileId> is needed to select a specific service profile.
int srvProfileId = -1;

//char * last_errmsg = NULL;//todo del



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
        waitForATresponse(token_array, AT_RES_PBREADY, sizeof(AT_RES_PBREADY) - 1, 10, GENERAL_RECV_TIMEOUT_MS);

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
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, 5, GENERAL_RECV_TIMEOUT_MS)) {
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
    if (!CELLULAR_INITIALIZED) {
        return false;
    }
    // AT+CSQ
    // response : +CSQ <rssi>,<ber> followed by OK
    // rssi: 0,1,2-30,31,99, ber: 0-7,99unknown

    // send AT+CSQ
    if (sendATcommand(AT_CMD_CSQ, sizeof(AT_CMD_CSQ) - 1)) {
        unsigned char * token_array[5] = {};
        if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, 5, GENERAL_RECV_TIMEOUT_MS)) {
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

    unsigned char command_to_send[MAX_AT_CMD_LEN] = "";
    memset(command_to_send, '\0',MAX_AT_CMD_LEN);
    if (mode == REG_AUTOMATICALLY || mode == DEREGISTER) {
        int cmd_size = sprintf(command_to_send, "%s%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, AT_CMD_SUFFIX);

        // send command
        while(!sendATcommand(command_to_send, cmd_size - 1));

        // wait for OK
        return waitForOK();

    } else if (mode == SPECIFIC_OP) {

        int act = 0;
        int cmd_size = sprintf(command_to_send, "%s%d,0,%s,%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, operatorName, act, AT_CMD_SUFFIX);
        while(!sendATcommand(command_to_send, cmd_size - 1));

        if (waitForOK()){
            return true;
        } else {
            act = 2;
            cmd_size = sprintf(command_to_send, "%s%d,0,%s,%d%s", AT_CMD_COPS_WRITE_PREFIX, mode, operatorName, act, AT_CMD_SUFFIX);
            while(!sendATcommand(command_to_send, cmd_size - 1));

            return waitForOK();
        }
    } else {
        printf("invalid mode!\n");
        return false;
    }
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
    while (!sendATcommand(AT_CMD_COPS_TEST, sizeof(AT_CMD_COPS_TEST) - 1));

    unsigned char * token_array[10] = {};

    if (waitForATresponse(token_array, AT_RES_OK, sizeof(AT_RES_OK) - 1, 10, GET_OPS_TIMEOUT_MS)) {

        char operators[MAX_INCOMING_BUF_SIZE];
        memset(operators, '\0', MAX_INCOMING_BUF_SIZE);
        // this remove "+COPS: "
        strncpy(operators, &(token_array[0])[7], strlen(token_array[0]) - 7);

        int num_of_found_ops = splitCopsResponseToOpsTokens(operators, opList, maxops);
        // fill results
        if (num_of_found_ops != 0) {
            *numOpsFound = num_of_found_ops;
            return true;
        }
    }

    return false;
}


//bool sendCmdAndGetResponse(unsigned int cmd_size, unsigned char * expected_response,
//                           unsigned int response_size, int max_responses, unsigned int timeout_ms) {
//
//    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
//    unsigned char temp_buffer[MAX_INCOMING_BUF_SIZE] = "";
//    unsigned int bytes_received = 0;
//    num_of_tokens_found = 0;
//    response_tokens = {};
//
//    while (!SerialSend(command_to_send_buffer, cmd_size));
//
//    bool wait_for_response_or_error = true;
//
//    while (wait_for_response_or_error) {
//        memset(incoming_buffer, '\0', bytes_received);
//        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timeout_ms);
//        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
//        num_of_tokens_found = splitBufferToResponses(temp_buffer, response_tokens, max_responses);
//
//        for (int i=0; i < num_of_tokens_found; i++) {
//            if (memcmp(response_tokens[i], expected_response, response_size) == 0) {
//                return true;
//            } else if (memcmp(response_tokens[i], AT_RES_ERROR, 5) == 0) {
//                return false;
//            } else if (memcmp(response_tokens[i], AT_RES_CME, 4) == 0) {
//                return false;
//            }
//        }
//    }
////    do {
////        memset(incoming_buffer, '\0', bytes_received);
////        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timeout_ms);
////        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
////        num_of_tokens = splitBufferToResponses(temp_buffer, response_tokens, max_responses);
////
////    } while ((memcmp(response_tokens[num_of_tokens-1], expected_response, response_size) != 0) &&
////             (memcmp(response_tokens[num_of_tokens-1], AT_RES_ERROR, 5) != 0)   &&
////             (memcmp(response_tokens[num_of_tokens-1], AT_RES_CME, 4) != 0));
////
////    return memcmp(response_tokens[num_of_tokens-1], expected_response, response_size) == 0;
//
////todo
//
//}

bool sendATcommand(unsigned char* command, unsigned int command_size) {
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
        SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, GENERAL_RECV_TIMEOUT_MS);
        num_of_tokens = splitBufferToResponses(incoming_buffer, token_array, 10);

    } while ((memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) != 0) &&
             (memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, sizeof(AT_RES_ERROR) - 1) != 0) &&
             (memcmp(token_array[num_of_tokens-1], AT_RES_CME, 4) != 0));


    return memcmp(token_array[num_of_tokens-1], AT_RES_OK, sizeof(AT_RES_OK) - 1) == 0;
}


bool waitForATresponse(unsigned char ** token_array, unsigned char * expected_response,
        unsigned int response_size, int max_responses, unsigned int timeout_ms) {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned char temp_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned int bytes_received = 0;
    int num_of_tokens = 0;


    do {
        memset(incoming_buffer, '\0', bytes_received);
        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timeout_ms);
        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
        num_of_tokens = splitBufferToResponses(temp_buffer, token_array, max_responses);

    } while ((memcmp(token_array[num_of_tokens-1], expected_response, response_size) != 0) &&
              (memcmp(token_array[num_of_tokens-1], AT_RES_ERROR, 5) != 0)   &&
              (memcmp(token_array[num_of_tokens-1], AT_RES_CME, 4) != 0));

    return memcmp(token_array[num_of_tokens-1], expected_response, response_size) == 0;
}


int getSISURCs(unsigned char ** token_array, int total_expected_urcs, int max_urcs, unsigned int timeout_ms) {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned char temp_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned int bytes_received = 0;
    int num_of_tokens = 0;

    do {
        memset(incoming_buffer, '\0', bytes_received);
        bytes_received = SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timeout_ms);
        strncat(temp_buffer, (const char *) incoming_buffer, bytes_received);
        num_of_tokens = splitBufferToResponses(temp_buffer, token_array, max_urcs);

        if ((num_of_tokens > 0) && (strcmp(token_array[num_of_tokens - 1], "ERROR") == 0)) {
            break;
        }

    } while (num_of_tokens < total_expected_urcs);

    return num_of_tokens;
}

/**
 *
 * @param sis_result the suffix of "^SIS: " urc
 * @param urcCause pointer to save urcCause
 * @param urcInfoId pointer to save urcInfoId
 * @return false if found error, otherwise true.
 */
bool parseSISresponse(char * sis_result, int * urcCause, int * urcInfoId) {
    // ^SIS: <srvProfileId>, <urcCause>[, [<urcInfoId>][, <urcInfoText>]]
    char * temp_token = strtok(sis_result, ",");   //<srvProfileId>,
    temp_token = strtok(NULL, ",");         //<urcCause>
    *urcCause = atoi(temp_token);
    temp_token = strtok(NULL, ",");         //<urcInfoId>
    *urcInfoId = atoi(temp_token);

    // if <urcCause>==0
    if ((*urcCause == 0) && (1 <= *urcInfoId) && (*urcInfoId <= 2000)) {
//        temp_token = strtok(NULL, ",");         //<urcInfoText>
//        strcpy(last_errmsg, temp_token);//todo del
        return false;
    }
    return true;
}

/**
 * parse "^SISR: " urc or "^SISW: " urc suffixes
 * @param sisrw_result "^SISR: " urc or "^SISW: " urc suffixes
 * @return urcCauseID
 */
int parseSISRWresponse(char * sisrw_result) {
    //^SISR: <srvProfileId>, <urcCauseId>
    char * urcCauseId = "";
    strcpy(urcCauseId, &sisrw_result[2]);
    return atoi(urcCauseId);
}

/**
 * parse "+CCID: " urc
 * @param iccid_buffer buffer to store result
 * @return length of iccid found
 */
int resolveCCIDresult(char * iccid_buffer) {
    unsigned char * tokens_array[10] = {};
    int received_urcs = getSISURCs(tokens_array, 2, 10, GENERAL_RECV_DLY_TIMEOUT_MS);

    // +CCID: <ICCID> or OK or ERROR
    const char urc_delimiter[] = ": ";
    for (int urc_idx = 0; urc_idx < received_urcs; urc_idx++) {
        if (strcmp(tokens_array[urc_idx], "ERROR") == 0) {
            return 0;
        } else if (*tokens_array[urc_idx] == '+') {
            char * temp_token = "";

            temp_token = strtok(tokens_array[urc_idx], urc_delimiter);
            temp_token = strtok(NULL, urc_delimiter);
            strncpy(iccid_buffer, temp_token, ICCID_BUFFER_SIZE);
            return strlen(temp_token);
        }
    }
    return 0;
}

///**
// * parse "^SISE: " urc suffix
// * @param sise_result "^SISR: " urc suffixes
// * @return urcCauseID
// */
//int parseSISEresponse(char * sise_result, char * response_buffer) {
//    // ^SISE: <srvProfileId>, <infoID>[, <info>]
//    char * temp_token = strtok(sise_result, ",");   //<srvProfileId>
//    temp_token = strtok(NULL, ",");                 //<infoID>
//    char * urcCauseId = "";
//    strcpy(urcCauseId, &sisrw_result[2]);
//    return atoi(urcCauseId);
//}


/**
 * this method parse SIS URCs
 * @param token_array tokens of SIS responses
 * @param received_urcs num of tokens.
 * @return false if SIS URC reflecting error, true otherwise
 */
bool parseSISURCs(unsigned char ** token_array, int received_urcs, char * urc_read_buffer) {
    const char urc_delimiter[] = ": ";
    for (int urc_idx = 0; urc_idx < received_urcs; urc_idx++) {
        char * urc_prefix = "";
        char * urc_result = "";
        char * temp_token = "";

        temp_token = strtok(token_array[urc_idx], urc_delimiter);
        strcpy(urc_prefix, temp_token);
        temp_token = strtok(NULL, urc_delimiter);
        strcpy(urc_result, temp_token);

        if (strcmp(urc_prefix, "ERROR") == 0) {
            return false;

        } else if (strcmp(urc_prefix, "^SIS") == 0) {
            //^SIS: <srvProfileId>, <urcCause>[, [<urcInfoId>][, <urcInfoText>]]
            int urcCause, urcInfoId;
            if (!parseSISresponse(urc_result, &urcCause, &urcInfoId)) {
                return false;
            }

        } else if (strcmp(urc_prefix, "^SISE") == 0) {
            //^SISE: <srvProfileId>, <infoID>[, <info>]
            strcpy(urc_read_buffer, &urc_result[2]);
            return true;

        } else if (strcmp(urc_prefix, "^SISR") == 0) {
            //^SISR: <srvProfileId>, <urcCauseId>
            if (parseSISRWresponse(urc_result) != 1) { return false; }

        } else if (strcmp(urc_prefix, "^SISW") == 0) {
            //^SISW: <srvProfileId>, <urcCauseId>
            if (parseSISRWresponse(urc_result) != 2) { return false; }

        } else if (urc_prefix[0] == '{') {
            strcpy(urc_read_buffer, urc_prefix);
        }

    }
    return true;
}


bool inetServiceSetupProfile(int srvProfileId, char *URL, char *payload, int payload_len) {
    // AT^SISS=<srvProfileId>, <srvParmTag>, <srvParmValue>

    // AT^SISS=6,"SrvType","Http"
    int cmd_size = sprintf(command_to_send_buffer, "%s%d,\"SrvType\",\"Http\"%s",
                           AT_CMD_SISS_WRITE_PRFX, srvProfileId, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    if (!waitForOK()) { return false; }

    // AT^SISS=6,"conId","<conProfileId>"
    cmd_size = sprintf(command_to_send_buffer, "%s%d,\"conId\",\"%d\"%s",
                       AT_CMD_SISS_WRITE_PRFX, srvProfileId, conProfileId, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    if (!waitForOK()) { return false; }

    // AT^SISS=6,"address","<url>"
    cmd_size = sprintf(command_to_send_buffer, "%s%d,\"address\",\"%s\"%s",
                       AT_CMD_SISS_WRITE_PRFX, srvProfileId, URL, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    if (!waitForOK()) { return false; }


    // AT^SISS=6,"cmd","1"
    cmd_size = sprintf(command_to_send_buffer, "%s%d,\"cmd\",\"%d\"%s",
                       AT_CMD_SISS_WRITE_PRFX, srvProfileId, SISS_CMD_HTTP_POST, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    if (!waitForOK()) { return false; }


    // AT^SISS=6,"hcContLen","0"
    // If "hcContLen" = 0 then the data given in the "hcContent" string will be posted
    // without AT^SISW required.
    cmd_size = sprintf(command_to_send_buffer, "%s%d,\"hcContLen\",\"%d\"%s",
                       AT_CMD_SISS_WRITE_PRFX, srvProfileId, payload_len, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    if (!waitForOK()) { return false; }


    //AT^SISS=6,"hcContent","HelloWorld!"
    cmd_size = sprintf(command_to_send_buffer, "%s%d,\"hcContent\",\"%s\"%s",
                       AT_CMD_SISS_WRITE_PRFX, srvProfileId, payload, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));
    return waitForOK();
}

bool inetServiceOpen(int srvProfileID) {
    //AT^SISO=6
    int cmd_size = sprintf(command_to_send_buffer, "%s%d%s", AT_CMD_SISO_WRITE_PRFX, srvProfileId, AT_CMD_SUFFIX);
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));

    // OK or ERROR
    // ^SIS: 6,0,2200,"Http en8wtnrvtnkt5.x.pipedream.net:443"
    // ^SISW: 6,2
    // ^SISR: 6,1
    char * urc_read_buff = "";
    unsigned char * tokens_array[10] = {};
    int received_urcs = getSISURCs(tokens_array, 4, 10, GENERAL_RECV_DLY_TIMEOUT_MS);
    if (!parseSISURCs(tokens_array, received_urcs, urc_read_buff)) {
        return false;
    } else {
        return true;
    }
}

bool inetServiceClose(int srvProfileID) {
    //AT^SISC=6
    int cmd_size = sprintf(command_to_send_buffer, "%s%d%s", AT_CMD_SISC_WRITE_PRFX, srvProfileId, AT_CMD_SUFFIX);
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));

    return waitForOK();
}

int splitBufferToResponses(unsigned char * buffer, unsigned char ** tokens_array, int max_tokens) {
    const char delimiter[] = "\r\n";
    char * token;
    int i=0;
    token = strtok(buffer, delimiter);
    while (token != NULL && i < max_tokens) {
        tokens_array[i++] = (unsigned char *)token;
        token = strtok(NULL, delimiter);
    }
    return i;
}

int splitCopsResponseToOpsTokens(unsigned char *cops_response, OPERATOR_INFO *opList, int max_ops) {
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
    opInfo->csq = 99;
    return true;
}


/**
 * Initialize an internet connection profile (AT^SICS) with inactTO=inact_time_sec and conType= GPRS0
 * and apn="postm2m.lu".
 * @param inact_time_sec
 * @return
 */
bool CellularSetupInternetConnectionProfile(int inact_time_sec) {
    unsigned char command_to_send[MAX_AT_CMD_LEN] = "";

    int conProfileId_cand;

    for (conProfileId_cand = 0; conProfileId_cand <= MAX_conProfileId; conProfileId_cand++) {
        memset(command_to_send, '\0',MAX_AT_CMD_LEN);

        // AT^SICS=0,conType,GPRS0
        int cmd_size = sprintf(command_to_send, "%s%d,conType,GPRS0%s", AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, AT_CMD_SUFFIX);
        while (!sendATcommand(command_to_send, cmd_size - 1));

        if (!waitForOK()) {
            continue;
        }

        memset(command_to_send, '\0', MAX_AT_CMD_LEN);

        // AT^SICS=0,"inactTO", "20"
        cmd_size = sprintf(command_to_send, "%s%d,\"inactTO\", \"%d\"%s",
                           AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, inact_time_sec, AT_CMD_SUFFIX);
        while (!sendATcommand(command_to_send, cmd_size - 1));

        if (!waitForOK()) {
            continue;
        }

        memset(command_to_send, '\0', MAX_AT_CMD_LEN);

        // AT^SICS=0,apn,"postm2m.lu"
        cmd_size = sprintf(command_to_send, "%s%d,apn,\"postm2m.lu\"%s",
                           AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, AT_CMD_SUFFIX);
        while (!sendATcommand(command_to_send, cmd_size - 1));

        if (waitForOK()) {
            conProfileId = conProfileId_cand;
            return true;
        }
//        // TODO delete this code.
//        // AT^SICS=0,conType,GPRS0
//        int cmd_size = sprintf(command_to_send, "%s%d,conType,GPRS0%s", AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, AT_CMD_SUFFIX);
//        while (!sendATcommand(command_to_send, cmd_size - 1));
//
//        if (waitForOK()) {
//            memset(command_to_send, '\0', MAX_AT_CMD_LEN);
//
//            // AT^SICS=0,"inactTO", "20"
//            int cmd_size = sprintf(command_to_send, "%s%d,\"inactTO\", \"%d\"%s",
//                                   AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, inact_time_sec, AT_CMD_SUFFIX);
//            while (!sendATcommand(command_to_send, cmd_size - 1));
//
//            if (waitForOK()) {
//                memset(command_to_send, '\0', MAX_AT_CMD_LEN);
//
//                // AT^SICS=0,apn,"postm2m.lu"
//                int cmd_size = sprintf(command_to_send, "%s%d,apn,\"postm2m.lu\"%s",
//                                       AT_CMD_SICS_WRITE_PRFX, conProfileId_cand, AT_CMD_SUFFIX);
//                while (!sendATcommand(command_to_send, cmd_size - 1));
//
//                if (waitForOK()) {
//                    conProfileId = conProfileId_cand;
//                    return true;
//                }
//            }
//        }
    }

    return false;
}

/**
 * Send an HTTP POST request. Opens and closes the socket.
 * @param URL is the complete address of the page we are posting to, e.g. “https://helloworld.com/mystuf/thispagewillreceivemypost?andadditional=stuff”
 * @param payload payload is everything that is sent as HTTP POST content.
 * @param payload_len payload_len is the length of the payload parameter.
 * @param response response is a buffer that holds the content of the HTTP response.
 * @param response_max_len response_max_len is the response buffer length.
 * @return The return value indicates the number of read bytes in response.
 *         If there is any kind of error, return -1.
 */
int CellularSendHTTPPOSTRequest(char *URL, char *payload, int payload_len, char *response, int response_max_len) {
    // sanity check: conProfileId exists
    if (conProfileId == -1) { return -1;}

    int srvProfileId = HTTP_POST_srvProfileId;

    if (!inetServiceSetupProfile(srvProfileId, URL, payload, payload_len)) {
        return -1;
    }

    if (!inetServiceOpen(srvProfileId)) {
        return -1;
    }


    // ready to read
    char * urc_read_buff = "";
    unsigned char * tokens_array[10] = {};

    // AT^SISR=6,20
    int cmd_size = sprintf(command_to_send_buffer, "%s%d,%d%s", AT_CMD_SISR_WRITE_PRFX, srvProfileId, response_max_len, AT_CMD_SUFFIX);
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));

    // OK or ERROR
    // ^SISR: 6,16
    // {"success":true}
    // OK
    //
    // ^SISR: 6,2

    int received_urcs = getSISURCs(tokens_array, 5, 10, GENERAL_RECV_DLY_TIMEOUT_MS);
    if (!parseSISURCs(tokens_array, received_urcs, urc_read_buff)) {
        return -1;
    }

    if (!inetServiceClose(srvProfileId)) {
        return -1;
    }


    // all went fine
    strncpy(response, urc_read_buff, response_max_len);

    if (response_max_len < strlen(urc_read_buff)) {
        return response_max_len;
    } else {
        return strlen(urc_read_buff);
    }
}

/**
 * Returns additional information on the last error occurred during CellularSendHTTPPOSTRequest.
 * The response includes urcInfoId, then comma (‘,’), then urcInfoText, e.g. “200,Socket-Error:3”.
 * @param errmsg
 * @param errmsg_max_len
 * @return
 */
int CellularGetLastError(char *errmsg, int errmsg_max_len) {

    // AT^SISE=<srvProfileId>
    int cmd_size = sprintf(command_to_send_buffer, "%s%d%s",
                           AT_CMD_SISE_WRITE_PRFX, srvProfileId, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));

    char * urc_read_buff = "";
    unsigned char * tokens_array[10] = {};
    // response:
    // ^SISE: <srvProfileId>, <infoID>[, <info>]
    int received_urcs = getSISURCs(tokens_array, 2, 10, GENERAL_RECV_DLY_TIMEOUT_MS);
    if (!parseSISURCs(tokens_array, received_urcs, urc_read_buff)) {
        return -1;
    }

    // all went fine
    strncpy(errmsg, urc_read_buff, errmsg_max_len);

    if (errmsg_max_len < strlen(urc_read_buff)) {
        return errmsg_max_len;
    } else {
        return strlen(urc_read_buff);
    }
}

/**
 * Will send AT+CCID cmd and parse the response to return the sim ICCID
 * @param iccid buffer to storce ICCID
 * @return ICCID length
 */
int CellularGetICCID(char * iccid) {
    //AT+CCID?
    int cmd_size = sprintf(command_to_send_buffer, "%s%s", AT_CMD_CCID_READ, AT_CMD_SUFFIX);
    // send command and check response OK/ERROR
    while (!sendATcommand(command_to_send_buffer, cmd_size - 1));

    return resolveCCIDresult(iccid);
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
    int payload_len = sprintf(cell_payload, CELL_PAYLOAD_FORMAT, iccid, operators_payload, unix_time);
    return payload_len;
}
