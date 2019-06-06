#include "gps.h"
#include "cellular.h"

#include <stdio.h>
#include "em_device.h"
#include "em_cmu.h"
#include "em_chip.h"
#include "em_emu.h"
#include "bsp.h"

#include "em_gpio.h"
#include "em_leuart.h"

#include "em_acmp.h"
#include "capsense.h"
#include "display.h"
#include "textdisplay.h"
#include "retargettextdisplay.h"


/****************************************************************************
 * 								DECLARATIONS
*****************************************************************************/
void Delay(uint32_t dlyTicks);
void infoOnDemand(GPS_LOCATION_INFO* last_location);
void speedLimitInterval(GPS_LOCATION_INFO* last_location, GPS_LOCATION_INFO* old_location);

/****************************************************************************
 * 								DEFS
*****************************************************************************/
#define PRINT_FORMAT "\fIteration #%d \nLatitude:\n%d\nLongitude:\n%d\nAltitude:\n%d\nTime:\n%s\nGoogle Maps:\n%f, %f\n"
#define MAX_IL_CELL_OPS 10
#define TRANSMIT_URL "https://en8wtnrvtnkt5.x.pipedream.net/write?db=mydb"
#define ONE_MINUTE_IN_MS 60000
#define FIVE_SECS_IN_MS 5000
#define MIN_SPEED_LIM 0
#define MAX_SPEED_LIM 30

enum PROCEDURE_TO_RUN{WAIT_FOR_USER, GPS_CELL_ON_DEMAND, SPEED_LIMIT_INIT, SPEED_LIMIT};

/**************************************************************************//**
 * 							GLOBAL VARIABLES
*****************************************************************************/
#ifdef _WIN64
static char* GPS_PORT = "COM4"; //todo check
static char* MODEM_PORT = "COM5";
#elif _WIN32
static char* GPS_PORT = "COM4";
static char* MODEM_PORT = "COM5";
#else
static char* GPS_PORT = "3";
static char* MODEM_PORT = "0";
#endif

bool DEBUG = true;
volatile uint32_t msTicks = 0; /* counts 1ms timeTicks */

enum PROCEDURE_TO_RUN CURRENT_OPERATION = WAIT_FOR_USER;
static int speed_limit = MIN_SPEED_LIM;
static bool high_speed_flag = false;
static bool transmit_speed_event = false;
static uint32_t last_location_Ticks = 0;
static uint32_t old_location_Ticks = 0;

/***************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counter
 ******************************************************************************/
void SysTick_Handler(void)
{
  msTicks++;       /* increment counter necessary in Delay()*/
}

/***************************************************************************//**
 * @brief Delays number of msTick Systicks (typically 1 ms)
 * @param dlyTicks Number of ticks to delay
 ******************************************************************************/
void Delay(uint32_t dlyTicks)
{
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) ;
}

/*******************************************************************************
 * @brief  This method will init all necessary device's settings.
 ******************************************************************************/
void initDeviceSettings() {
	/* Enable clock for GPIO purposes */
	CMU_ClockEnable(cmuClock_GPIO, true);
	/* Enable clock for USART module */
	CMU_ClockEnable(cmuClock_USART2, true);
	/* Enable clock for HF peripherals */
	CMU_ClockEnable(cmuClock_HFPER, true);


	/* Initialize the display module. */
	DISPLAY_Init();
	RETARGET_TextDisplayInit();

	/* setting stdout to flush prints immediately */
	setbuf(stdout, NULL);


	/* Configure PB0 as input and enable interrupt  */
	GPIO_PinModeSet(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, gpioModeInputPull, 1);
	GPIO_IntConfig(BSP_GPIO_PB0_PORT, BSP_GPIO_PB0_PIN, false, true, true);

	/* Configure PB1 as input and enable interrupt */
	GPIO_PinModeSet(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, gpioModeInputPull, 1);
	GPIO_IntConfig(BSP_GPIO_PB1_PORT, BSP_GPIO_PB1_PIN, false, true, true);

	/* Buttons interrupts config */
	NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
	NVIC_EnableIRQ(GPIO_ODD_IRQn);


	/* Setup SysTick Timer for 1 msec interrupts  */
	if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
		while (1) ;
	}

	/* Start capacitive sense buttons */
	CAPSENSE_Init();
}

/*******************************************************************************
 * @brief  Main function
 ******************************************************************************/
int main(void)
{

	/* Chip errata */
	CHIP_Init();

	/* Init DCDC regulator with kit specific parameters */
	EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
	EMU_DCDCInit(&dcdcInit);

	initDeviceSettings();

	/* General declarations and memory allocations for main use. */
	GPS_LOCATION_INFO* last_location = malloc(sizeof(GPS_LOCATION_INFO));
	if (last_location == NULL) {
	  printf("Memory Allocation Error");
	  return 1;
	} else {
		memset(last_location->fixtime, '\0', 18);
	}
	GPS_LOCATION_INFO* old_location = malloc(sizeof(GPS_LOCATION_INFO));
	if (old_location == NULL) {
		printf("Memory Allocation Error");
		return 1;
	} else {
		memset(old_location->fixtime, '\0', 18);
	}


	/* Init GPS & Cellular modules */
	printf("MCU initialized.\nTurn on GPS & Modem.\n");
	Delay(1000);
	GPSInit(GPS_PORT);
	CellularInit(MODEM_PORT);
	printf("\fDear user,\nPlease press any button.\n");
	printf("BTN0:\n  GPS+CELL on demand\n");
	printf("BTN1:\n  Speed limit\n");

	while(true) {
		if (CURRENT_OPERATION == GPS_CELL_ON_DEMAND) {
			CURRENT_OPERATION = WAIT_FOR_USER;
			infoOnDemand(last_location);

		} else if (CURRENT_OPERATION == SPEED_LIMIT_INIT) {
			speed_limit = MIN_SPEED_LIM;
			high_speed_flag = false;
			transmit_speed_event = false;
			// update location
			uint32_t iteration_counter = 10;
			while (iteration_counter > 0) {
				bool result;
				result = GPSGetFixInformation(last_location);
				if (result && last_location->valid_fix == 1){
					iteration_counter--;
				}
			}
			last_location_Ticks = msTicks;
			old_location = memcpy(old_location, last_location, sizeof(GPS_LOCATION_INFO));
			old_location_Ticks = last_location_Ticks;
			CURRENT_OPERATION = SPEED_LIMIT;

		} else if (CURRENT_OPERATION == SPEED_LIMIT) {
			printf("\fcurrent speed limit:\n%2d Km/h", speed_limit);
			if (msTicks - last_location_Ticks > FIVE_SECS_IN_MS) {
				speedLimitInterval(last_location, old_location);
			}

			Delay(100);
			CAPSENSE_Sense();

			if (CAPSENSE_getPressed(BUTTON1_CHANNEL)
				&& !CAPSENSE_getPressed(BUTTON0_CHANNEL)) {
				if (speed_limit < MAX_SPEED_LIM){
					speed_limit += 5;
				}
			  printf("\r%2d", speed_limit);
			} else if (CAPSENSE_getPressed(BUTTON0_CHANNEL)
					   && !CAPSENSE_getPressed(BUTTON1_CHANNEL)) {
			  if (speed_limit > MIN_SPEED_LIM) {
				  speed_limit -= 5;
			  }
			  printf("\r%2d", speed_limit);
			}
		}
	}

	printf("\nDisabling Cellular and exiting..\n");
	CellularDisable();
	GPSDisable();

	free(last_location);
	exit(0);
}

void infoOnDemand(GPS_LOCATION_INFO* last_location) {
	// update location
	uint32_t iteration_counter = 10;
	while (iteration_counter > 0) {
		bool result;
		result = GPSGetFixInformation(last_location);
		if (result && last_location->valid_fix == 1){
			iteration_counter--;
		}
	}

	// Makes sure it’s responding to AT commands.
	// looping to check modem responsiveness.
	while (!CellularCheckModem()) {
		Delay(WAIT_BETWEEN_CMDS_MS);
	}

	// Setting modem to unregister and remain unregistered.
	while (!CellularSetOperator(DEREGISTER, NULL)) {
		Delay(WAIT_BETWEEN_CMDS_MS);
	}


	// store data about found operators and registered ones.
	OPERATOR_INFO operators_info[MAX_IL_CELL_OPS];
	OPERATOR_INFO past_registerd_operators[MAX_IL_CELL_OPS];
	int num_operators_found = 0;
	int num_of_past_registerd = 0;

	/* Finds all available cellular operators. */
	printf("Finding all available cellular operators...");
	while (!CellularGetOperators(operators_info, MAX_IL_CELL_OPS, &num_operators_found)) {
		printf(".");
	}
	printf(" %d operators found.\n", num_operators_found);

	/* Tries to register with each one of them (one at a time). */
	printf("Trying to register with each one of them (one at a time)\n");
	for (int op_index = 0; op_index < num_operators_found; op_index++) {

		// unregister from current operator
		while (!CellularSetOperator(DEREGISTER, NULL));

		// register to specific operator
		printf("Trying to register with %s...", operators_info[op_index].operatorName);
		if (CellularSetOperator(SPECIFIC_OP, operators_info[op_index].operatorName)) {

			// If registered successfully, it prints the signal quality.
			int registration_status = 0;

			// verify registration to operator and check status
			if (CellularGetRegistrationStatus(&registration_status) &&
					(registration_status == 1 || registration_status == 5)) {
				printf("registered successfully\n");
				// registration_status == 1: Registered to home network
				// registration_status == 5: Registered,
				// roaming ME is registered at a foreign network (national or international network)
				int signal_quality = -1;
				if (CellularGetSignalQuality(&signal_quality)) {
					printf("Current signal quality: %ddBm\n", signal_quality);
					operators_info[op_index].csq = signal_quality;

				} else if (signal_quality == -1){
					printf("Modem didn't respond\n");

				} else if (signal_quality == 99) {
					printf("Current signal quality is UNKNOWN\n");
				}

				// copy operator info to past_registerd_operators archive
				memcpy(&past_registerd_operators[num_of_past_registerd++],
					   &operators_info[op_index], sizeof(operators_info[op_index]));
				continue;
			}
		}
		printf("Modem registration failed\n");
	}


	// Connects to an available operator (if available, if no, tries again after one minute)
	for (int op_index = 0; op_index < num_of_past_registerd; op_index++) {


		// unregister from current operator
		printf("Unregister from current operator\n");
		while (!CellularSetOperator(DEREGISTER, NULL));

		// register to operator
		printf("Trying to register with %s...\n", past_registerd_operators[op_index].operatorName);
		if (CellularSetOperator(SPECIFIC_OP, past_registerd_operators[op_index].operatorName)) {

			// If registered successfully, it prints the signal quality.
			int registration_status = 0;

			/* Trying to register to operator. (if available, if no, tries again after one minute)  */
			/* verify registration to operator and check status */
			if (!(CellularGetRegistrationStatus(&registration_status) &&
				  (registration_status == 1 || registration_status == 5))) {

				Delay(ONE_MINUTE_IN_MS);

				if (!(CellularGetRegistrationStatus(&registration_status) &&
					  (registration_status == 1 || registration_status == 5))) {
					continue;
				}
			}

			// we registered to operator, setup inet connection
			if (!CellularSetupInternetConnectionProfile(60)) {
				continue;
			}

			// get CCID
			char iccid[ICCID_BUFFER_SIZE] = "";
			CellularGetICCID(iccid);

			// get unix time
			char unix_time[35];
			GPSConvertFixtimeToUnixTime(last_location, unix_time);

			// transmit GPS data over HTTP
			char gps_payload[1000] = "";
			int gps_payload_len = GPSGetPayload(last_location, iccid, unix_time, gps_payload);

			char transmit_response[100] = "";
			while (CellularSendHTTPPOSTRequest(TRANSMIT_URL, gps_payload, gps_payload_len, transmit_response, 99) == -1);

			// transmit cell data over HTTP
			char cell_payload[1000] = "";
			int cell_payload_len = CellularGetPayload(past_registerd_operators, num_of_past_registerd, iccid, unix_time, cell_payload);
			while (CellularSendHTTPPOSTRequest(TRANSMIT_URL, cell_payload, cell_payload_len, transmit_response, 99) == -1);

			op_index = num_of_past_registerd;//todo check
			break;
		}
	}
}


void speedLimitInterval(GPS_LOCATION_INFO* last_location, GPS_LOCATION_INFO* old_location) {
	old_location = memcpy(old_location, last_location, sizeof(GPS_LOCATION_INFO));
	old_location_Ticks = last_location_Ticks;
	// update location
	uint32_t iteration_counter = 5;
	while (iteration_counter > 0) {
		bool result;
		result = GPSGetFixInformation(last_location);
		if (result && last_location->valid_fix == 1){
			iteration_counter--;
		}
	}
	last_location_Ticks = msTicks;

	// calculate speed
	double speed = GPSGetSpeedOfLocations(old_location, last_location);

	// check conditions
	if (speed > speed_limit){
		high_speed_flag = true;
		transmit_speed_event = true;
	}

	if (high_speed_flag && speed < speed_limit) {
		high_speed_flag = false;
		transmit_speed_event = true;
	}

	if (transmit_speed_event) {
		transmit_speed_event = false;

		// get CCID
		char iccid[ICCID_BUFFER_SIZE] = "";
		CellularGetICCID(iccid);

		// get unix time
		char unix_time[35];
		memset(unix_time, '\0', 35);
		GPSConvertFixtimeToUnixTime(last_location, unix_time);
		char gps_payload[1000] = "";
		int gps_payload_len = GPSGetSPEEDPayload(last_location, iccid, unix_time, high_speed_flag, gps_payload);

		// store data about found operators and registered ones.
		OPERATOR_INFO operators_info[MAX_IL_CELL_OPS];
		int num_operators_found = 0;

		/* Finds all available cellular operators. */
		printf("Finding all available cellular operators...");
		while (!CellularGetOperators(operators_info, MAX_IL_CELL_OPS, &num_operators_found)) {
			printf(".");
		}

		/* Tries to register with one of them (one at a time). */
		for (int op_index = 0; op_index < num_operators_found; op_index++) {

			// unregister from current operator
			while (!CellularSetOperator(DEREGISTER, NULL));

			// register to specific operator
			printf("Trying to register with %s...", operators_info[op_index].operatorName);
			if (CellularSetOperator(SPECIFIC_OP, operators_info[op_index].operatorName)) {

				// If registered successfully, it prints the signal quality.
				int registration_status = 0;

				// verify registration to operator and check status
				if (CellularGetRegistrationStatus(&registration_status) &&
						(registration_status == 1 || registration_status == 5)) {
					printf("registered successfully\n");

					// we registered to operator, setup inet connection
					if (!CellularSetupInternetConnectionProfile(60)) {
						continue;
					}

					char transmit_response[100] = "";
					while (CellularSendHTTPPOSTRequest(TRANSMIT_URL, gps_payload, gps_payload_len, transmit_response, 99) == -1);
					break;
				}
			}
		}

	}
}

/***************************************************************************//**
 * @brief Unified GPIO Interrupt handler (pushbuttons)
 *        PB0 Prints first name.
 *        PB1 Prints last name.
 *****************************************************************************/
void GPIO_Unified_IRQ(void) {
  /* Get and clear all pending GPIO interrupts */
  uint32_t interruptMask = GPIO_IntGet();
  GPIO_IntClear(interruptMask);

  /* Act on interrupts */
  if (interruptMask & (1 << BSP_GPIO_PB0_PIN)) {
	  /* Clear screen */
	  printf("\f");
	  /* set flag */
	  CURRENT_OPERATION = GPS_CELL_ON_DEMAND;
  }

  if (interruptMask & (1 << BSP_GPIO_PB1_PIN)) {
	  /* Clear screen */
	  printf("\f");
	  /* set flag */
	  CURRENT_OPERATION = SPEED_LIMIT_INIT;
  }
}

/***************************************************************************//**
 * @brief GPIO Interrupt handler for even pins
 ******************************************************************************/
void GPIO_EVEN_IRQHandler(void) {
  GPIO_Unified_IRQ();
}

/***************************************************************************//**
 * @brief GPIO Interrupt handler for odd pins
 ******************************************************************************/
void GPIO_ODD_IRQHandler(void) {
  GPIO_Unified_IRQ();
}
