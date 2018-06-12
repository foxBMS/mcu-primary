/**
 *
 * @copyright &copy; 2010 - 2018, Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V. All rights reserved.
 *
 * BSD 3-Clause License
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * We kindly request you to use one or more of the following phrases to refer to foxBMS in your hardware, software, documentation or advertising materials:
 *
 * &Prime;This product uses parts of foxBMS&reg;&Prime;
 *
 * &Prime;This product includes parts of foxBMS&reg;&Prime;
 *
 * &Prime;This product is derived from foxBMS&reg;&Prime;
 *
 */

/**
 * @file    os.h
 * @author  foxBMS Team
 * @date    27.08.2015 (date of creation)
 * @ingroup OS
 * @prefix  OS
 *
 * @brief   Implementation of the tasks used by the system, headers
 *
 */

#ifndef OS_H_
#define OS_H_

/*================== Includes =============================================*/
#include "cmsis_os.h"

/*================== Macros and Definitions ===============================*/
/**
 *Number of mutexes for the engine
 */
#define OS_ENG_NUM_OF_MUTEXES   0
/**
 *Number of events for the ENINGE
 */
#define OS_ENG_NUM_OF_EVENTS    0

/**
 *Number of mutexes for the application
 */
#define OS_APPL_NUM_OF_MUTEXES  0
/**
 *Number of events for the application
 */
#define OS_APPL_NUM_OF_EVENTS   0

/**
 * typedef for thread priority
 */
typedef enum  {
  OS_PRIORITY_IDLE           = osPriorityIdle,           /*!< priority: idle (lowest)        */
  OS_PRIORITY_LOW            = osPriorityLow,            /*!< priority: low                  */
  OS_PRIORITY_BELOW_NORMAL   = osPriorityBelowNormal,    /*!< priority: below normal         */
  OS_PRIORITY_NORMAL         = osPriorityNormal,         /*!< priority: normal (default)     */
  OS_PRIORITY_ABOVE_NORMAL   = osPriorityAboveNormal,    /*!< priority: above normal         */
  OS_PRIORITY_HIGH           = osPriorityHigh,           /*!< priority: high                 */
  OS_PRIORITY_ABOVE_HIGH     = osPriorityRealtime,       /*!< priority: above high           */
  OS_PRIORITY_VERY_HIGH      = osPriorityRealtime+1,       /*!< priority: very high          */
  OS_PRIORITY_BELOW_REALTIME = osPriorityRealtime+2,       /*!< priority: below realtime     */
  OS_PRIORITY_REALTIME       = osPriorityRealtime+3,       /*!< priority: realtime (highest) */
  OS_PRIORITY_ERROR          = osPriorityError           /*!< system cannot determine priority or thread has illegal priority  */
} OS_PRIORITY_e;

/**
 * enum of OS boot states
 */
typedef enum {
    OS_OFF                            = 0,    /*!< */
    OS_ENG_CREATE_QUEUES              = 1,    /*!< */
    OS_ENG_CREATE_MUTEX               = 2,    /*!< */
    OS_ENG_CREATE_EVENT               = 3,    /*!< */
    OS_ENG_CREATE_TASKS               = 4,    /*!< */
    OS_APPL_CREATE_MUTEX              = 5,    /*!< */
    OS_APPL_CREATE_EVENT              = 6,    /*!< */
    OS_APPL_CREATE_TASKS              = 7,    /*!< */
    OS_INIT_PREOS                     = 8,    /*!< */
    OS_INIT_OSRESOURCES               = 9,    /*!< */
    OS_INIT_OSSTARTKERNEL             = 10,    /*!< */
    OS_RUNNING                        = 11,   /*!< */
    OS_EEPR_INIT                      = 12,   /*!< */
    OS_BMS_INIT                   = 13,   /*!< */
    OS_SYSTEM_RUNNING                 = 14,   /*!< system is running  */
    OS_SAFETY_FEATURE_ENABLED         = 15,
    OS_SAFETY_FEATURE_DISABLED        = 16,
    OS_INIT_OS_FATALERROR_SCHEDULE    = 0x80, /*!< error in scheduler */
} OS_BOOT_STATE_e;

/**
 * enum of ECU operation modes
 */
typedef enum {
    ECU_UNDEFINED         = 0,    /*!< ecu mode is undefined      */
    ECU_NORMAL_MODE       = 1,    /*!< ecu mode is ok             */
    ECU_CALIBARION_MODE   = 2,    /*!< ecu is in calibration      */
    ECU_TEST_MODE         = 4,    /*!< actually not implemented   */
} ECU_OPERATION_MODE_e;

/**
 * OS-Timer
 */
typedef struct {
    uint8_t  Timer_1ms;   /*!< milliseconds       */
    uint8_t  Timer_10ms;  /*!< 10*milliseconds    */
    uint8_t  Timer_100ms; /*!< 100*milliseconds   */
    uint8_t  Timer_sec;   /*!< seconds            */
    uint8_t  Timer_min;   /*!< minutes            */
    uint8_t  Timer_h;     /*!< hours              */
    uint16_t Timer_d;     /*!< days               */
} OS_TIMER_s;

/**
 * struct for FreeRTOS task definition
 */
typedef struct {
    uint32_t Phase;          /*!< (ms)                                                                   */
    uint32_t CycleTime;      /*!< (ms)                                                                   */
    OS_PRIORITY_e Priority;  /*!<                                                                        */
    uint32_t Stacksize;      /*!<  Defines the size, in words, of the stack allocated to the idle task.  */
} BMS_Task_Definition_s;

/*================== Constant and Variable Definitions ====================*/
extern volatile OS_BOOT_STATE_e os_boot;
extern volatile OS_BOOT_STATE_e os_safety_state;
extern uint32_t os_schedulerstarttime;
extern volatile  OS_TIMER_s os_timer;

extern osMutexId ENG_Mutexes[];
extern EventGroupHandle_t ENG_Events[];

extern osMutexId APPL_Mutexes[];
extern EventGroupHandle_t APPL_Events[];

extern uint8_t eng_init;

/*================== Function Prototypes ==================================*/

/**
 * @brief   Initialization the RTOS interface
 *
 * The initialization functions inits mutexes, eventgroups and tasks.
 *
 * @return  void
 */
extern void OS_TaskInit();

/**
 * @brief   OS_IdleTask, called by vApplicationIdleHook()
 * @return  void
 */
extern void OS_IdleTask(void);

/**
 * @brief   Hook function for the idle task
 *
 * @return  void
 */
extern void vApplicationIdleHook(void);

/**
 * @brief   reads non-volatile memory and initializes the sys module
 *
 * Start up at scheduler start, called from 1ms-Task before the 1ms cyclic execution
 *
 * @return  void
 */
extern void OS_PostOSInit(void);

/**
 * @brief  auxiliary function to distinguish OS Task from an ISR
 *
 * checks the function context: task (thread) mode or interrupt (handler) mode
 *
 * @return  0 when OS-Task, 0x1 when ISR
 */
extern uint8_t OS_Check_Context(void);


/**
 * @brief  Enter Critical interface function for use in FreeRTOS-Tasks and FreeRTOS-ISR
 *
 * checks the function context (task/thread mode or interrupt (handler) mode) and calls the corresponding
 * enter-critical function
 *
 * @return  void
 */
extern void OS_TaskEnter_Critical(void);


/**
 * @brief  Exit Critical interface function for use in FreeRTOS-Tasks and FreeRTOS-ISR
 *
 * checks the function context (task/thread mode or interrupt (handler) mode) and calls the corresponding
 * exit-critical function
 *
 * @return  void
 */
extern void OS_TaskExit_Critical(void);

/**
 * @brief   increments the system timer os_timer
 *
 * The os_timer is a runtime-counter, counting the time since the last reset.
 *
 * @return  void
 */
extern void OS_TimerTrigger(void);
/*================== Function Implementations =============================*/

#endif /* OS_H_ */
