/**************************************************************************//**
 * @cellular.h
 * @brief
 * @version 0.0.1
 *  ***************************************************************************/
#ifndef IOT_CELLULAR_H
#define IOT_CELLULAR_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************
 * 								DEFS
*****************************************************************************/

typedef struct __OPERATOR_INFO {
    char operatorName[20];
    int  operatorCode;
    char accessTechnology[4];
} OPERATOR_INFO;

/**************************************************************************//**
 * 							GLOBAL VARIABLES
*****************************************************************************/
static bool CELLULAR_INITIALIZED = false;
enum MODE{REG_AUTOMATICALLY, SPECIFIC_OP, DEREGISTER};
enum ERROR_MODE {DISABLED, NUMERIC, VERBOSE};


/**
 * Initialize whatever is needed to start working with the cellular modem (e.g. the serial port).
 * @param port
 */
void CellularInit(char *port);

/**
 * Deallocate / close whatever resources CellularInit() allocated.
 */
void CellularDisable();

/**
 * Checks if the modem is responding to AT commands.
 * @return Return true if it does, returns false otherwise.
 */
bool CellularCheckModem(void);

/**
 * @param status
 * @return Returns false if the modem did not respond or responded with an error.
 * Returns true if the command was successful and the registration status was obtained
 * from the modem. In that case, the status parameter will be populated with the numeric
 * value of the <regStatus> field of the “+CREG” AT command.

 */
bool CellularGetRegistrationStatus(int *status);

/**
 * @param csq
 * @return Returns false if the modem did not respond or responded with an error
 * (note, CSQ=99 is also an error!)Returns true if the command was successful and
 * the signal quality was obtained from the modem. In that case, the csq parameter
 * will be populated with the numeric value between -113dBm and -51dBm.

 */
bool CellularGetSignalQuality(int *csq);


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
bool CellularSetOperator(int mode, char *operatorName);


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
bool CellularGetOperators(OPERATOR_INFO *opList, int maxops, int *numOpsFound);

/**
 * Initialize an internet connection profile (AT^SICS) with inactTO=inact_time_sec and conType= GPRS0
 * and apn="postm2m.lu".
 * @param inact_time_sec
 * @return
 */
bool CellularSetupInternetConnectionProfile(int inact_time_sec);

/**
 * Send an HTTP POST request. Opens and closes the socket.
 * @param URL
 * @param payload
 * @param payload_len
 * @param response
 * @param response_max_len
 * @return
 */
int CellularSendHTTPPOSTRequest(char *URL, char *payload, int payload_len, char *response, int response_max_len);

/**
 * Returns additional information on the last error occurred during CellularSendHTTPPOSTRequest.
 * The response includes urcInfoId, then comma (‘,’), then urcInfoText, e.g. “200,Socket-Error:3”.
 * @param errmsg
 * @param errmsg_max_len
 * @return
 */
int CellularGetLastError(char *errmsg, int errmsg_max_len);


/**
 * Will send AT+CCID cmd and parse the response to return the sim ICCID
 * @param iccid buffer to storce ICCID
 * @return ICCID length
 */
int CellularGetICCID(char * iccid);

#endif //IOT_CELLULAR_H
