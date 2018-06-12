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
 * @file    enginetask.c
 * @author  foxBMS Team
 * @date    27.08.2015 (date of creation)
 * @ingroup ENGINE
 * @prefix  ENG
 *
 * @brief   Calls of functions within engine task
 *
 */

/*================== Includes =============================================*/
#include "general.h"
#include "enginetask.h"
#include "database.h"
#include "os.h"
#include "bkpsram.h"


/*================== Macros and Definitions ===============================*/


/*================== Constant and Variable Definitions ====================*/
static BMS_Task_Definition_s eng_tskdef_engine  = { 0,      1,  OS_PRIORITY_REALTIME,          1024/4};

/**
 * Definition of task handle of the engine task
 */
static xTaskHandle eng_handle_engine;

/**
 * Definition of task handle 1 millisecond task
 */
static xTaskHandle eng_handle_tsk_1ms;

/**
 * Definition of task handle 10 milliseconds task
 */
static xTaskHandle eng_handle_tsk_10ms;

/**
 * Definition of task handle 100 milliseconds task
 */
static xTaskHandle eng_handle_tsk_100ms;

/**
 * Definition of task handle Diagnosis task
 */
static xTaskHandle eng_handle_tsk_diagnosis;

/**
 * Definition of task handle EventHandler
 */
static xTaskHandle eng_handle_tsk_eventhandler;

QueueHandle_t data_queueID;

/*================== Function Prototypes ==================================*/


/*================== Function Implementations =============================*/

void ENG_CreateTask(void) {
    // Database Task
    osThreadDef(TSK_Engine, (os_pthread) ENG_TSK_Engine,
            eng_tskdef_engine.Priority, 0, eng_tskdef_engine.Stacksize);
    eng_handle_engine = osThreadCreate(osThread(TSK_Engine), NULL);

    // Cyclic Task 1ms
    osThreadDef(TSK_Cyclic_1ms, (os_pthread) ENG_TSK_Cyclic_1ms,
            eng_tskdef_cyclic_1ms.Priority, 0, eng_tskdef_cyclic_1ms.Stacksize);
    eng_handle_tsk_1ms = osThreadCreate(osThread(TSK_Cyclic_1ms), NULL);

    // Cyclic Task 10ms
    osThreadDef(TSK_Cyclic_10ms, (os_pthread) ENG_TSK_Cyclic_10ms,
            eng_tskdef_cyclic_10ms.Priority, 0, eng_tskdef_cyclic_10ms.Stacksize);
    eng_handle_tsk_10ms = osThreadCreate(osThread(TSK_Cyclic_10ms), NULL);

    // Cyclic Task 100ms
    osThreadDef(TSK_Cyclic_100ms, (os_pthread) ENG_TSK_Cyclic_100ms,
            eng_tskdef_cyclic_100ms.Priority, 0, eng_tskdef_cyclic_100ms.Stacksize);
    eng_handle_tsk_100ms = osThreadCreate(osThread(TSK_Cyclic_100ms), NULL);

    // EventHandler Task
    osThreadDef(TSK_EventHandler, (os_pthread) ENG_TSK_EventHandler,
            eng_tskdef_eventhandler.Priority, 0, eng_tskdef_eventhandler.Stacksize);
    eng_handle_tsk_eventhandler = osThreadCreate(osThread(TSK_EventHandler), NULL);

    // Diagnosis Task
    osThreadDef(TSK_Diagnosis, (os_pthread) ENG_TSK_Diagnosis,
            eng_tskdef_diagnosis.Priority, 0, eng_tskdef_diagnosis.Stacksize);
    eng_handle_tsk_diagnosis = osThreadCreate(osThread(TSK_Diagnosis), NULL);
}

void ENG_CreateMutex(void) {
}

void ENG_CreateEvent(void) {
}

void ENG_CreateQueues(void) {
    /* Create a queue capable of containing a pointer of type DATA_QUEUE_MESSAGE_s
    Data of Messages are passed by pointer as they contain a lot of data. */
    data_queueID = xQueueCreate( 1, sizeof( DATA_QUEUE_MESSAGE_s) );

    if (data_queueID == NULL_PTR) {
        // Failed to create the queue
        ;            // @ TODO Error Handling
    }
}

void ENG_TSK_Engine(void) {
    OS_PostOSInit();

    os_boot = OS_SYSTEM_RUNNING;

    for (;;) {
        DATA_Task();    /* Call database manager */
        DIAG_SysMon();  /* Call Overall System Monitoring */
    }
}


void ENG_TSK_Cyclic_1ms(void) {
    while (os_boot != OS_SYSTEM_RUNNING) {
        ;
    }

    if (eng_init == FALSE) {
        ENG_Init();
        eng_init = TRUE;
    }

    osDelayUntil(&os_schedulerstarttime, eng_tskdef_cyclic_1ms.Phase);

    while (1) {
        uint32_t currentTime = osKernelSysTick();
        OS_TimerTrigger();  // Increment system timer os_timer
        NVM_OperatingHoursTrigger();  // Increment operating hours timer
        ENG_Cyclic_1ms();
        osDelayUntil(&currentTime, eng_tskdef_cyclic_1ms.CycleTime);
    }
}


void ENG_TSK_Cyclic_10ms(void) {
    while (os_boot != OS_SYSTEM_RUNNING) {
        ;
    }

    osDelayUntil(&os_schedulerstarttime, eng_tskdef_cyclic_10ms.Phase);

    while (1) {
        uint32_t currentTime = osKernelSysTick();
        ENG_Cyclic_10ms();
        osDelayUntil(&currentTime, eng_tskdef_cyclic_10ms.CycleTime);
    }
}

void ENG_TSK_Cyclic_100ms(void) {
    while (os_boot != OS_SYSTEM_RUNNING) {
        ;
    }

    osDelayUntil(&os_schedulerstarttime, eng_tskdef_cyclic_100ms.Phase);

    while (1) {
        uint32_t currentTime = osKernelSysTick();
        ENG_Cyclic_100ms();
        osDelayUntil(&currentTime, eng_tskdef_cyclic_100ms.CycleTime);
    }
}


void ENG_TSK_EventHandler(void) {
    while (os_boot != OS_SYSTEM_RUNNING) {
        ;
    }

    osDelayUntil(&os_schedulerstarttime, eng_tskdef_eventhandler.Phase);

    while (1) {
        uint32_t currentTime = osKernelSysTick();
        ENG_EventHandler();
        osDelayUntil(&currentTime, eng_tskdef_eventhandler.CycleTime);
    }
}

void ENG_TSK_Diagnosis(void) {
    while (os_boot != OS_SYSTEM_RUNNING) {
        ;
    }

    osDelayUntil(&os_schedulerstarttime, eng_tskdef_diagnosis.Phase);

    while (1) {
        uint32_t currentTime = osKernelSysTick();
        ENG_Diagnosis();
        osDelayUntil(&currentTime, eng_tskdef_diagnosis.CycleTime);
    }
}
