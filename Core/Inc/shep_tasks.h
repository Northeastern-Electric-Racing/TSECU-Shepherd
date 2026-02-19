
#ifndef SHEP_TASKS_H
#define SHEP_TASKS_H

#include "tx_api.h"
#include "datastructs.h"

#define ANALYZER_FLAG  0x1
#define SANITIZER_FLAG 0x2
#define DEBUG_FLAG     0x4

// #define DEBUG_HV_PLATE
// #define DEBUG_VOLTAGES
// #define DEBUG_TEMPS
// #define DEBUG_AlGOS

/* Initializes all ThreadX threads.
*  Calls to create_thread() should go in here
*/
uint8_t shep_threads_init(TX_BYTE_POOL *byte_pool);

/**
 * @brief Default Task for emitting heartbeat and petting watchdog
 */
void vDefaultTask(ULONG thread_input);

/**
 * @brief State Machine Task that handles the BMS state
 */
void vStateMachine(ULONG thread_input);

/**
 * @brief Task Handling listening and reacting to CAN messages
 */
void vCanReceive(ULONG thred_input);

/**
 * @brief Task for handling emitting CAN messages
 */
void vCanDispatch(ULONG thread_input);

/**
 * @brief Task Handling listening and reacting to Ethernet messages
 */
void vEthernetIncoming(ULONG thread_input);

/**
 * @brief Task for handling emitting Ethernet messages
 */
void vEthernetOutgoing(ULONG thread_input);

/**
 * @brief Analyzer Task for processing raw chip data from segments
 */
void vAnalyzer(ULONG thread_input);

/**
 * @brief Task for Retrieving raw segment data from the ADBMS6830
 */
void vGetSegmentData(ULONG thread_input);

/**
 * @brief Task for collecting data from the ADBMS2950 and performing Coulomb Counting
 */
void vHvPlateData(ULONG thread_input);
void vSanitizer(ULONG thread_input);

/**
 * @brief Task for saniziting temperature data to ensure integrity of algorithms
 */
void vSanitizer(ULONG thread_input);

/**
 * @brief Task for performing DCL and CCL algorithms
 */
void vBMSAlgorithms(ULONG thread_input);

/**
 * @brief Task for controlling fan
 */
void vControl(ULONG thread_input);

#endif
