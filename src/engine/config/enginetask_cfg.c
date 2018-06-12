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
 * @file    enginetask_cfg.c
 * @author  foxBMS Team
 * @date    26.08.2015 (date of creation)
 * @ingroup ENGINE_CONF
 * @prefix  ENG
 *
 * @brief   Engine task configuration
 *
 */

/*================== Includes =============================================*/
/* recommended include order of header files:
 * 
 * 1.    include general.h
 * 2.    include module's own header
 * 3...  other headers
 *
 */
#include "general.h"
#include "enginetask_cfg.h"

#include "database.h"
#include "meas.h"
#include "ltc.h"
#include "sys.h"
#include "bms.h"
#include "interlock.h"
#include "contactor.h"
#include "can.h"
#include "cansignal.h"
#include "isoguard.h"
#include "wdg.h"
#include "bal.h"
#include "intermcu.h"
#include "adc_ex.h"
/*================== Macros and Definitions ===============================*/

/*================== Constant and Variable Definitions ====================*/
BMS_Task_Definition_s eng_tskdef_cyclic_1ms     = { 0,      1,  OS_PRIORITY_ABOVE_HIGH,        1024/4};
BMS_Task_Definition_s eng_tskdef_cyclic_10ms    = { 2,     10,  OS_PRIORITY_HIGH,              1024/4};
BMS_Task_Definition_s eng_tskdef_cyclic_100ms   = {56,    100,  OS_PRIORITY_ABOVE_NORMAL,      1024/4};
BMS_Task_Definition_s eng_tskdef_eventhandler   = { 0,      1,  OS_PRIORITY_VERY_HIGH,         1024/4};
BMS_Task_Definition_s eng_tskdef_diagnosis      = { 0,      1,  OS_PRIORITY_BELOW_REALTIME,    1024/4};

/*================== Function Prototypes ==================================*/

/*================== Function Implementations =============================*/


void ENG_Init(void) {
    SYS_RETURN_TYPE_e sys_retVal = SYS_ILLEGAL_TASK_TYPE;

    DATA_BLOCK_ERRORSTATE_s error_flags;

    DB_ReadBlock(&error_flags, DATA_BLOCK_ID_ERRORSTATE);

    error_flags.general_error               = 0;

    error_flags.currentsensorresponding     = 0;

    error_flags.main_plus                   = 0;
    error_flags.main_minus                  = 0;
    error_flags.precharge                   = 0;
    error_flags.charge_main_plus            = 0;
    error_flags.charge_main_minus           = 0;
    error_flags.charge_precharge            = 0;

    error_flags.interlock                   = 0;

    error_flags.over_current_charge         = 0;
    error_flags.over_current_discharge      = 0;
    error_flags.over_voltage                = 0;
    error_flags.under_voltage               = 0;
    error_flags.over_temperature_charge     = 0;
    error_flags.over_temperature_discharge  = 0;
    error_flags.under_temperature_charge    = 0;
    error_flags.under_temperature_discharge = 0;
    error_flags.crc_error                   = 0;
    error_flags.mux_error                   = 0;
    error_flags.spi_error                   = 0;

    error_flags.can_timing                  = 0;
    error_flags.can_timing_cc               = 0;

    error_flags.can_cc_used                 = 1;

    DB_WriteBlock(&error_flags, DATA_BLOCK_ID_ERRORSTATE);

    // Init Sys
    sys_retVal = SYS_SetStateRequest(SYS_STATE_INIT_REQUEST);

#if BUILD_MODULE_ENABLE_SAFETY_FEATURES == 1
    IMC_enableInterrupt();
#endif
}

void ENG_Cyclic_1ms(void) {
    MEAS_Ctrl();
    LTC_Trigger();
    EEPR_Trigger();


}

void ENG_Cyclic_10ms(void) {
    SYS_Trigger();
    CONT_Trigger();
    ILCK_Trigger();
#if CAN_USE_CAN_NODE0
    CAN_TxMsgBuffer(CAN_NODE0);
#endif
#if CAN_USE_CAN_NODE1
    CAN_TxMsgBuffer(CAN_NODE1);
#endif

#if BUILD_MODULE_ENABLE_WATCHDOG
    WDG_IWDG_Refresh();
#endif

#ifdef BUILD_MODULE_CPULOAD_MEASUREMENT
    CPULOAD_Calc();
#endif
}

void ENG_Cyclic_100ms(void) {
    static uint8_t counter = 0;

    ADC_Ctrl();

    // Read every 200ms because of possible jitter and lowest Bender frequency 10Hz -> 100ms
    if (counter % 2 == 0) {
        ISO_MeasureInsulation();
    }
    if (counter == 255) {
        NVM_SetOperatingHours();
    }
    counter++;
}


void ENG_EventHandler(void) {
}

void ENG_Diagnosis(void) {
}

