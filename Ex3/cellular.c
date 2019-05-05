#include "cellular.h"
#include "serial_io.h"

bool sendATcommand(unsigned char* command); //TODO declare
bool waitForATresponse(unsigned char * expected_response, unsigned int response_size);
/**************************************************************************//**
 * 								DEFS
*****************************************************************************/
#define MODEM_BAUD_RATE 115200
#define MAX_INCOMING_BUF_SIZE 1000
#define MAX_OUTGOING_BUF_SIZE 100


/*****************************************************************************
 * 							GLOBAL VARIABLES
*****************************************************************************/
static bool REGISTERED = false;
//static unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE];    // TODO how we deallocate?
unsigned int recv_timeout_ms = 100;

//TODO CMDS:
// here or at header file? static ?
// SAPIR do I need do deallocate? how we deallocate?

// AT_COMMANDS
unsigned char AT_CMD_ECHO_OFF[] = "ATE0\r\n";
unsigned char AT_CMD_SHUTDOWN[] = "AT^SMSO\r\n";

// AT RESPONDS
unsigned char AT_RES_OK[] = "\r\nOK\r\n";
unsigned char AT_RES_SYSSTART[] = "\r\n^SYSSTART\r\n";
unsigned char AT_RES_PBREADY[] = "\r\n+PBREADY\r\n";




/**
 * Initialize whatever is needed to start working with the cellular modem (e.g. the serial port).
 * @param port
 */
void CellularInit(char *port){
    // TODO do we need to check if it already ON?
    CELLULAR_INITIALIZED = SerialInit(port, MODEM_BAUD_RATE);
    if (!CELLULAR_INITIALIZED) {
        exit(EXIT_FAILURE);
    }

    // check modem responded with ^+PBREADY
    waitForATresponse(AT_RES_PBREADY, sizeof(AT_RES_PBREADY) - 1);

    // set modem echo off
    while (!sendATcommand(AT_CMD_ECHO_OFF));

    // verify echo off
    waitForATresponse(AT_RES_OK, sizeof(AT_RES_OK) - 1);
}


/**
 * Deallocate / close whatever resources CellularInit() allocated.
 */
void CellularDisable(){
    if (CELLULAR_INITIALIZED) {
        // shut down modem TODO do we need to shutdown the modem?
        while (!sendATcommand(AT_CMD_SHUTDOWN));

        // verify modem off
        waitForATresponse(AT_RES_OK, sizeof(AT_RES_OK) - 1);//TODO NEED TO CHECK FOR ERROR

        SerialDisable();
        CELLULAR_INITIALIZED = false;
    }
}


/**
 * Checks if the modem is responding to AT commands.
 * @return Return true if it does, returns false otherwise.
 */
bool CellularCheckModem(void){
    if (CELLULAR_INITIALIZED) {
        // send hello (AT\r\n)
        //TODO THIS IS PART OF CODE WE SHOULD MAKE GLOBAL
        unsigned char ok[] = "\r\nOK\r\n";
        unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
        unsigned int timout_ms = 100;
        unsigned int zero = 0;
        // send hello
        unsigned char hello_cmd[] = "AT\r\n";
        if (!SerialSend(hello_cmd, sizeof(hello_cmd))) {
            fprintf(stderr, "send error\n");
        }
        // verify modem reponse
        memset(incoming_buffer, zero, MAX_INCOMING_BUF_SIZE); // TODO maybe implement as part of serial receive?
        SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timout_ms);
        return (memcmp(incoming_buffer, ok, (size_t) sizeof(ok)) == 0);

//        unsigned int hello_size = 5;
//        unsigned char hello[5] = "AT\r\n";
//        unsigned int input_size = 6;
//        unsigned char input_buf[MAX_INCOMING_BUF_SIZE]; // OK/ERROR
//        unsigned int timeout_ms = 100;
//
//        if (!SerialSend(hello, input_size)) {
//            return false;
//        }
//
//         Wait for response (OK)
//        SerialRecv(input_buf, input_size, timeout_ms);
//        return (strncmp(input_buf, "OK", 2) == 0);
    } else {
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
    // TODO: on init we should set  AT+COPS= 1 / 2 to set manual operator select?
    //TODO THIS IS PART OF CODE WE SHOULD MAKE GLOBAL
    unsigned char at_creg_command[] = "AT+CREG?\r\n";
    unsigned char ok[] = "\r\nOK\r\n";
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    unsigned int timout_ms = 100;
    unsigned int zero = 0;
    // send creg command
    if (!SerialSend(at_creg_command, sizeof(at_creg_command))) {
        fprintf(stderr, "send error\n");
    }
    // get answer
    memset(incoming_buffer, zero, MAX_INCOMING_BUF_SIZE); // TODO maybe implement as part of serial receive?
    SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, timout_ms);

    // TODO parse
    // return bool and set status.
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
    unsigned int command_size = sizeof(command) - 1;
    if (!SerialSend(command, command_size)){
        return false;
    }
    Delay(100); // TODO
    return true;
}

bool waitForATresponse(unsigned char * expected_response, unsigned int response_size) {
    unsigned char incoming_buffer[MAX_INCOMING_BUF_SIZE] = "";
    memset(incoming_buffer, 0, MAX_INCOMING_BUF_SIZE);
    do {
        SerialRecv(incoming_buffer, MAX_INCOMING_BUF_SIZE, recv_timeout_ms);
    } while (memcmp(incoming_buffer, expected_response, response_size) != 0);
    return true;
}


bool getATresponse(){
    unsigned char buf[200] = ""; //TODO: change size
    int timeout_ms = 100; // TODO: change/define
    SerialRecv(buf, 200, timeout_ms);
    //TODO: continue
}