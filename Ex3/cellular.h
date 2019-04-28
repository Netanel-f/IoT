#ifndef IOT_CELLULAR_H
#define IOT_CELLULAR_H

#include <stdbool.h>

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


#endif //IOT_CELLULAR_H
