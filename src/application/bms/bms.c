/**
 *
 * @copyright &copy; 2010 - 2017, Fraunhofer-Gesellschaft zur Foerderung der angewandten Forschung e.V. All rights reserved.
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
 * @file    bms.c
 * @author  foxBMS Team
 * @date    21.09.2015 (date of creation)
 * @ingroup ENGINE
 * @prefix  BMS
 *
 * @brief   bms driver implementation
 */


/*================== Includes =============================================*/
#include "bms.h"
#include "contactor.h"
#include "interlock.h"
#include "os.h"
#include "diag.h"
#include "database.h"
#include "bal.h"
#include "batterycell_cfg.h"

/*================== Macros and Definitions ===============================*/

/**
 * Saves the last state and the last substate
 */
#define BMS_SAVELASTSTATES()    bms_state.laststate = bms_state.state; \
                                bms_state.lastsubstate = bms_state.substate;

/*================== Constant and Variable Definitions ====================*/

/**
 * contains the state of the contactor state machine
 */
static BMS_STATE_s bms_state = {
    .timer                  = 0,
    .statereq               = BMS_STATE_NO_REQUEST,
    .state                  = BMS_STATEMACH_UNINITIALIZED,
    .substate               = BMS_ENTRY,
    .laststate              = BMS_STATEMACH_UNINITIALIZED,
    .lastsubstate           = 0,
    .triggerentry           = 0,
    .ErrRequestCounter      = 0,
    .counter                = 0,
};

/*================== Function Prototypes ==================================*/

static BMS_RETURN_TYPE_e BMS_CheckStateRequest(BMS_STATE_REQUEST_e statereq);
static BMS_STATE_REQUEST_e BMS_GetStateRequest(void);
static BMS_STATE_REQUEST_e BMS_TransferStateRequest(void);
static uint8_t BMS_CheckReEntrance(void);
static uint8_t BMS_CheckCANRequests(void);
static uint8_t BMS_CheckBalancingRequests(void);
static STD_RETURN_TYPE_e BMS_CheckAnyErrorFlagSet(void);
static void BMS_CheckVoltages(void);
static void BMS_CheckTemperatures(void);
static void BMS_CheckCurrent(void);

/*================== Function Implementations =============================*/

/**
 * @brief   re-entrance check of SYS state machine trigger function
 *
 * @details This function is not re-entrant and should only be called time- or event-triggered. It
 *          increments the triggerentry counter from the state variable ltc_state. It should never
 *          be called by two different processes, so if it is the case, triggerentry should never
 *          be higher than 0 when this function is called.
 *
 * @return  retval  0 if no further instance of the function is active, 0xff else
 */
static uint8_t BMS_CheckReEntrance(void) {
    uint8_t retval = 0;
    OS_TaskEnter_Critical();
    if (!bms_state.triggerentry) {
        bms_state.triggerentry++;
    } else {
        retval = 0xFF;  // multiple calls of function
    }
    OS_TaskExit_Critical();
    return (retval);
}

/**
 * @brief   gets the current state request.
 *
 * @details This function is used in the functioning of the SYS state machine.
 *
 * @return  current state request, taken from BMS_STATE_REQUEST_e
 */
static BMS_STATE_REQUEST_e BMS_GetStateRequest(void) {
    BMS_STATE_REQUEST_e retval = BMS_STATE_NO_REQUEST;

    OS_TaskEnter_Critical();
    retval    = bms_state.statereq;
    OS_TaskExit_Critical();

    return (retval);
}


BMS_STATEMACH_e BMS_GetState(void) {
    return (bms_state.state);
}

/**
 * @brief   transfers the current state request to the state machine.
 *
 * @details This function takes the current state request from cont_state and transfers it to th
 *          state machine. It resets the value from cont_state to BMS_STATE_NO_REQUEST
 *
 * @return  retVal          current state request, taken from BMS_STATE_REQUEST_e
 */
static BMS_STATE_REQUEST_e BMS_TransferStateRequest(void) {
    BMS_STATE_REQUEST_e retval = BMS_STATE_NO_REQUEST;

    OS_TaskEnter_Critical();
    retval    = bms_state.statereq;
    bms_state.statereq = BMS_STATE_NO_REQUEST;
    OS_TaskExit_Critical();
    return (retval);
}



BMS_RETURN_TYPE_e BMS_SetStateRequest(BMS_STATE_REQUEST_e statereq) {
    BMS_RETURN_TYPE_e retVal = BMS_STATE_NO_REQUEST;

    OS_TaskEnter_Critical();
    retVal = BMS_CheckStateRequest(statereq);

    if (retVal == BMS_OK) {
            bms_state.statereq = statereq;
    }
    OS_TaskExit_Critical();

    return (retVal);
}

/**
 * @brief   checks the state requests that are made.
 *
 * @details This function checks the validity of the state requests. The results of the checked is
 *          returned immediately.
 *
 * @param   statereq    state request to be checked
 *
 * @return  result of the state request that was made, taken from BMS_RETURN_TYPE_e
 */
static BMS_RETURN_TYPE_e BMS_CheckStateRequest(BMS_STATE_REQUEST_e statereq) {
    if (statereq == BMS_STATE_ERROR_REQUEST) {
        return BMS_OK;
    }

    if (bms_state.statereq == BMS_STATE_NO_REQUEST) {
        // init only allowed from the uninitialized state
        if (statereq == BMS_STATE_INIT_REQUEST) {
            if (bms_state.state == BMS_STATEMACH_UNINITIALIZED) {
                return BMS_OK;
            } else {
                return BMS_ALREADY_INITIALIZED;
            }
        } else {
            return BMS_ILLEGAL_REQUEST;
        }
    } else {
        return BMS_REQUEST_PENDING;
    }
}

void BMS_Trigger(void) {
    BMS_STATE_REQUEST_e statereq = BMS_STATE_NO_REQUEST;
    CONT_STATEMACH_e contstate = CONT_STATEMACH_UNDEFINED;
    DATA_BLOCK_SYSTEMSTATE_s systemstate = {0};
    uint8_t bal_request = 0;

    DIAG_SysMonNotify(DIAG_SYSMON_BMS_ID, 0);  // task is running, state = ok

    if (bms_state.state != BMS_STATEMACH_UNINITIALIZED) {
        BMS_CheckVoltages();
        BMS_CheckTemperatures();
        BMS_CheckCurrent();
    }
    // Check re-entrance of function
    if (BMS_CheckReEntrance()) {
        return;
    }

    if (bms_state.timer) {
        if (--bms_state.timer) {
            bms_state.triggerentry--;
            return;    // handle state machine only if timer has elapsed
        }
    }

    /****Happens every time the state machine is triggered**************/
    switch (bms_state.state) {
        /****************************UNINITIALIZED***********************************/
        case BMS_STATEMACH_UNINITIALIZED:
            // waiting for Initialization Request
            statereq = BMS_TransferStateRequest();
            if (statereq == BMS_STATE_INIT_REQUEST) {
                BMS_SAVELASTSTATES();
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.state = BMS_STATEMACH_INITIALIZATION;
                bms_state.substate = BMS_ENTRY;
            } else if (statereq == BMS_STATE_NO_REQUEST) {
                // no actual request pending //
            } else {
                bms_state.ErrRequestCounter++;  // illegal request pending
            }
            break;


        /****************************INITIALIZATION**********************************/
        case BMS_STATEMACH_INITIALIZATION:
            BMS_SAVELASTSTATES();
            // CONT_SetStateRequest(CONT_STATE_INIT_REQUEST);

            bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
            bms_state.state = BMS_STATEMACH_INITIALIZED;
            bms_state.substate = BMS_ENTRY;

            break;

        /****************************INITIALIZED*************************************/
        case BMS_STATEMACH_INITIALIZED:
            BMS_SAVELASTSTATES();
            bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
            bms_state.state = BMS_STATEMACH_IDLE;
            bms_state.substate = BMS_ENTRY;
            break;

        /****************************IDLE*************************************/
        case BMS_STATEMACH_IDLE:
            BMS_SAVELASTSTATES();

            if (bms_state.substate == BMS_ENTRY) {
                DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                systemstate.bms_state = BMS_STATEMACH_IDLE;
                DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS) {
                if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_STANDBY;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                }
            }
            break;


        /****************************STANDBY*************************************/
        case BMS_STATEMACH_STANDBY:
            BMS_SAVELASTSTATES();

            if (bms_state.substate == BMS_ENTRY){
                CONT_SetStateRequest(CONT_STATE_STANDBY_REQUEST);
                ILCK_SetStateRequest(ILCK_STATE_CLOSE_REQUEST);
                bal_request = BMS_CheckBalancingRequests();
                if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                }
                else if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                }
                else if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_ACTIVE_REQUEST);
                } else {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_REQUEST);
                }
                bms_state.timer = BMS_STATEMACH_MEDIUMTIME_MS;
                bms_state.substate = BMS_CHECK_ERROR_FLAGS_INTERLOCK;
                DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                systemstate.bms_state = BMS_STATEMACH_STANDBY;
                DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS_INTERLOCK) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_INTERLOCK_CHECKED;
                    break;
                }
            }
            else if (bms_state.substate == BMS_INTERLOCK_CHECKED){
                bms_state.timer = BMS_STATEMACH_VERYLONGTIME_MS;
                bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_BALANCING_REQUESTS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_BALANCING_REQUESTS) {
                bal_request = BMS_CheckBalancingRequests();
                if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_ACTIVE_REQUEST);
                }
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                break;
            } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS) {
                if (BMS_CheckCANRequests() == BMS_REQ_ID_NORMAL) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_PRECHARGE;
                    bms_state.substate = BMS_ENTRY;
                    break;
                }
                if (BMS_CheckCANRequests() == BMS_REQ_ID_CHARGE){
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_CHARGE_PRECHARGE;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                }
            }
            break;

        /****************************PRECHARGE*************************************/
        case BMS_STATEMACH_PRECHARGE:
            BMS_SAVELASTSTATES();

            if (bms_state.substate == BMS_ENTRY) {
                DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                systemstate.bms_state = BMS_STATEMACH_PRECHARGE;
                DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                bal_request = BMS_CheckBalancingRequests();
                if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                }
                else if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                }
                else if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                } else {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_REQUEST);
                }
                CONT_SetStateRequest(CONT_STATE_NORMAL_REQUEST);
                bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_BALANCING_REQUESTS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_BALANCING_REQUESTS) {
                bal_request = BMS_CheckBalancingRequests();
                if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                }
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                break;
            } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS) {
                if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_STANDBY;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_CONTACTOR_NORMAL_STATE;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_CONTACTOR_NORMAL_STATE) {
                contstate = CONT_GetState();
                if (contstate == CONT_STATEMACH_NORMAL) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_NORMAL;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else if (contstate == CONT_STATEMACH_ERROR) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                }
            }
            break;

        /****************************NORMAL*************************************/
        case BMS_STATEMACH_NORMAL:
            BMS_SAVELASTSTATES();
            if (bms_state.substate == BMS_ENTRY) {
                DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                systemstate.bms_state = BMS_STATEMACH_NORMAL;
                DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_ERROR;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_BALANCING_REQUESTS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_BALANCING_REQUESTS) {
                bal_request = BMS_CheckBalancingRequests();
                if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                }
                if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                    BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                }
                bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                break;
            } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS) {
                if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_STANDBY;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                }
            }
            break;

            /****************************CHARGE_PRECHARGE*************************************/
            case BMS_STATEMACH_CHARGE_PRECHARGE:
                BMS_SAVELASTSTATES();

                if (bms_state.substate == BMS_ENTRY){
                    DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                    systemstate.bms_state = BMS_STATEMACH_CHARGE_PRECHARGE;
                    DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                    bal_request = BMS_CheckBalancingRequests();
                    if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                    }
                    else if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                    }
                    else if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                    } else {
                        BAL_SetStateRequest(BAL_STATE_ACTIVE_REQUEST);
                    }
                    CONT_SetStateRequest(CONT_STATE_CHARGE_REQUEST);
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    break;
                }
                else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS){
                    if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_ERROR;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else{
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.substate = BMS_CHECK_BALANCING_REQUESTS;
                        break;
                    }
                }
                else if (bms_state.substate == BMS_CHECK_BALANCING_REQUESTS) {
                    bal_request = BMS_CheckBalancingRequests();
                    if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                    }
                    if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                    }
                    if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                    }
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                    break;
                } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS){
                    if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_STANDBY;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else {
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.substate = BMS_CHECK_CONTACTOR_CHARGE_STATE;
                        break;
                    }
                }
                else if (bms_state.substate == BMS_CHECK_CONTACTOR_CHARGE_STATE){
                    contstate = CONT_GetState();
                    if (contstate == CONT_STATEMACH_CHARGE){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_CHARGE;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else if (contstate == CONT_STATEMACH_ERROR){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_ERROR;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else{
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    }
                }
                break;




            /****************************CHARGE*************************************/
            case BMS_STATEMACH_CHARGE:
                BMS_SAVELASTSTATES();

                if (bms_state.substate == BMS_ENTRY){
                    DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                    systemstate.bms_state = BMS_STATEMACH_CHARGE;
                    DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS){
                    if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_ERROR;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else{
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.substate = BMS_CHECK_BALANCING_REQUESTS;
                        break;
                    }
                }
                else if (bms_state.substate == BMS_CHECK_BALANCING_REQUESTS) {
                    bal_request = BMS_CheckBalancingRequests();
                    if (bal_request == BMS_BAL_INACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_INACTIVE_OVERRIDE_REQUEST);
                    }
                    if (bal_request == BMS_BAL_ACTIVE_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_ACTIVE_OVERRIDE_REQUEST);
                    }
                    if (bal_request == BMS_BAL_OUT_OF_OVERRIDE) {
                        BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                    }
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                    break;
                } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS){
                    if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY){
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.state = BMS_STATEMACH_STANDBY;
                        bms_state.substate = BMS_ENTRY;
                        break;
                    } else {
                        bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                        bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                        break;
                    }
                }
                break;

        /****************************ERROR*************************************/
        case BMS_STATEMACH_ERROR:
            BMS_SAVELASTSTATES();
            if (bms_state.substate == BMS_ENTRY) {
                BAL_SetStateRequest(BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST);
                CONT_SetStateRequest(CONT_STATE_ERROR_REQUEST);
                bms_state.timer = BMS_STATEMACH_MEDIUMTIME_MS;
                bms_state.substate = BMS_OPEN_INTERLOCK;
                DATA_GetTable(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                systemstate.bms_state = BMS_STATEMACH_ERROR;
                DATA_StoreDataBlock(&systemstate, DATA_BLOCK_ID_SYSTEMSTATE);
                break;
            } else if (bms_state.substate == BMS_OPEN_INTERLOCK) {
                ILCK_SetStateRequest(ILCK_STATE_OPEN_REQUEST);
                bms_state.timer = BMS_STATEMACH_VERYLONGTIME_MS;
                bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                break;
            } else if (bms_state.substate == BMS_CHECK_ERROR_FLAGS) {
                if (BMS_CheckAnyErrorFlagSet() == E_NOT_OK) {
                    // we stay already in requested state, nothing to do
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_STATE_REQUESTS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_STATE_REQUESTS) {
                if (BMS_CheckCANRequests() == BMS_REQ_ID_STANDBY) {
                    ILCK_SetStateRequest(ILCK_STATE_CLOSE_REQUEST);
                    bms_state.timer = BMS_STATEMACH_MEDIUMTIME_MS;
                    bms_state.substate = BMS_CHECK_INTERLOCK_CLOSE_AFTER_ERROR;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                }
            } else if (bms_state.substate == BMS_CHECK_INTERLOCK_CLOSE_AFTER_ERROR) {
                if (ILCK_GetInterlockFeedback() == ILCK_SWITCH_ON) {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.state = BMS_STATEMACH_STANDBY;
                    bms_state.substate = BMS_ENTRY;
                    break;
                } else {
                    bms_state.timer = BMS_STATEMACH_SHORTTIME_MS;
                    bms_state.substate = BMS_CHECK_ERROR_FLAGS;
                    break;
                }
            }
            break;
        default:
            break;
    }  // end switch(bms_state.state)

    bms_state.triggerentry--;
    bms_state.counter++;
}

/*================== Static functions =====================================*/
/*
 * @brief   Checks the state requests made to the BMS state machine
 *
 * @details Checks of the state request in the database and sets this value as return value
 *
 * @return  requested state
 */
static uint8_t BMS_CheckCANRequests(void) {
    uint8_t retVal = BMS_REQ_ID_NOREQ;
    DATA_BLOCK_STATEREQUEST_s request;

    DATA_GetTable(&request, DATA_BLOCK_ID_STATEREQUEST);

    if (request.state_request == BMS_REQ_ID_STANDBY) {
        retVal = BMS_REQ_ID_STANDBY;
    } else if (request.state_request == BMS_REQ_ID_NORMAL) {
        retVal = BMS_REQ_ID_NORMAL;
    }
#if BS_SEPARATE_POWERLINES == 1
    else if (request.state_request == BMS_REQ_ID_CHARGE){
        retVal = BMS_REQ_ID_CHARGE;
    }
#endif // BS_SEPARATE_POWERLINES == 1

    return retVal;
}

/*
 * @brief   Checks the balancing requests made to the BMS state machine
 *
 * @details Checks of the balancing requests in the database and sets this value as return value
 *
 * @return  requested state
 */
static uint8_t BMS_CheckBalancingRequests(void) {
    uint8_t retVal = BMS_REQ_ID_NOREQ;
    DATA_BLOCK_BALANCING_CONTROL_s bal_request;

    DATA_GetTable(&bal_request, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);

    if (bal_request.request == BMS_BAL_INACTIVE_OVERRIDE) {
        retVal = BMS_BAL_INACTIVE_OVERRIDE;
    } else if (bal_request.request == BMS_BAL_OUT_OF_OVERRIDE) {
        retVal = BMS_BAL_OUT_OF_OVERRIDE;
    } else if (bal_request.request == BMS_BAL_ACTIVE_OVERRIDE) {
        retVal = BMS_BAL_ACTIVE_OVERRIDE;
    } else {
        retVal = BMS_BAL_NO_REQUEST;
    }

    bal_request.request = BMS_BAL_NO_REQUEST;
    DATA_StoreDataBlock(&bal_request, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);

    return retVal;
}


/**
 * @brief   checks the abidance by the safe operating area
 *
 * @details verify for cell voltage measurements (U), if minimum and maximum values are out of range
 */
static void BMS_CheckVoltages(void) {
    DATA_BLOCK_MINMAX_s minmax;

    DATA_GetTable(&minmax, DATA_BLOCK_ID_MINMAX);

    if (minmax.voltage_max > BC_VOLTMAX) {
        DIAG_Handler(DIAG_CH_CELLVOLTAGE_OVERVOLTAGE, DIAG_EVENT_NOK, 0, NULL_PTR);
    } else {
        DIAG_Handler(DIAG_CH_CELLVOLTAGE_OVERVOLTAGE, DIAG_EVENT_OK, 0, NULL_PTR);
    }

    if (minmax.voltage_min < BC_VOLTMIN) {
        DIAG_Handler(DIAG_CH_CELLVOLTAGE_UNDERVOLTAGE, DIAG_EVENT_NOK, 0, NULL_PTR);
    } else {
        DIAG_Handler(DIAG_CH_CELLVOLTAGE_UNDERVOLTAGE, DIAG_EVENT_OK, 0, NULL_PTR);
    }
}


/**
 * @brief   checks the abidance by the safe operating area
 *
 * @details verify for cell temperature measurements (T), if minimum and maximum values are out of range
 */
static void BMS_CheckTemperatures(void) {
    DATA_BLOCK_MINMAX_s minmax;
    DATA_BLOCK_CURRENT_s curr_tab;

    DATA_GetTable(&curr_tab, DATA_BLOCK_ID_CURRENT);
    DATA_GetTable(&minmax, DATA_BLOCK_ID_MINMAX);

    if (BS_CheckCurrentValue_Direction(curr_tab.current) == BS_CURRENT_DISCHARGE){
        if (minmax.temperature_max > BC_TEMPMAX_DISCHARGE) {
            DIAG_Handler(DIAG_CH_TEMP_OVERTEMPERATURE_DISCHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_TEMP_OVERTEMPERATURE_DISCHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    } else{
        if (minmax.temperature_max > BC_TEMPMAX_CHARGE) {
            DIAG_Handler(DIAG_CH_TEMP_OVERTEMPERATURE_CHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_TEMP_OVERTEMPERATURE_CHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    }

    if (BS_CheckCurrentValue_Direction(curr_tab.current) == BS_CURRENT_DISCHARGE){
        if (minmax.temperature_min < BC_TEMPMIN_DISCHARGE) {
            DIAG_Handler(DIAG_CH_TEMP_UNDERTEMPERATURE_DISCHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_TEMP_UNDERTEMPERATURE_DISCHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    } else{
        if (minmax.temperature_min < BC_TEMPMIN_CHARGE) {
            DIAG_Handler(DIAG_CH_TEMP_UNDERTEMPERATURE_CHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_TEMP_UNDERTEMPERATURE_CHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    }
}





/**
 * @brief   checks the abidance by the safe operating area
 *
 * @details verify for cell current measurements (I), if minimum and maximum values are out of range
 */
static void BMS_CheckCurrent(void) {
    DATA_BLOCK_SOX_s sof_tab;
    DATA_BLOCK_CURRENT_s curr_tab;

    DATA_GetTable(&sof_tab, DATA_BLOCK_ID_SOX);
    DATA_GetTable(&curr_tab, DATA_BLOCK_ID_CURRENT);

#if MEAS_TEST_CELL_SOF_LIMITS == TRUE
    if (((curr_tab.current < (-1000*(sof_tab.sof_continuous_charge))) ||
            (curr_tab.current > (1000*sof_tab.sof_continuous_discharge)))) {
        retVal = FALSE;
    }
#endif

    if (BS_CheckCurrentValue_Direction(curr_tab.current) == BS_CURRENT_CHARGE){
        if (curr_tab.current<0.0){
            curr_tab.current = - curr_tab.current;
        }
        if (curr_tab.current > BC_CURRENTMAX_CHARGE) {
            DIAG_Handler(DIAG_CH_OVERCURRENT_CHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_OVERCURRENT_CHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    } else{
        if (curr_tab.current<0.0){
            curr_tab.current = - curr_tab.current;
        }
        if (curr_tab.current > BC_CURRENTMAX_DISCHARGE) {
            DIAG_Handler(DIAG_CH_OVERCURRENT_DISCHARGE,DIAG_EVENT_NOK,0, NULL_PTR);
        } else{
            DIAG_Handler(DIAG_CH_OVERCURRENT_DISCHARGE,DIAG_EVENT_OK,0, NULL_PTR);
        }
    }

}

/**
 * @brief   Checks the errorflags
 *
 * @details Checks all the error flags from the database and returns an error if at least onee is set.
 *
 * @return  E_OK if no error flag is set, otherwise E_NOT_OK
 */
static STD_RETURN_TYPE_e BMS_CheckAnyErrorFlagSet(void) {
    STD_RETURN_TYPE_e retVal = E_NOT_OK;
    DATA_BLOCK_SYSTEMSTATE_s error_flags;

    DATA_GetTable(&error_flags, DATA_BLOCK_ID_SYSTEMSTATE);

    if (error_flags.main_plus                   == 1 ||
        error_flags.main_minus                  == 1 ||
        error_flags.precharge                   == 1 ||
        error_flags.charge_main_plus            == 1 ||
        error_flags.charge_main_minus           == 1 ||
        error_flags.charge_precharge            == 1 ||
        error_flags.interlock                   == 1 ||
        error_flags.over_current_charge         == 1 ||
        error_flags.over_current_discharge      == 1 ||
        error_flags.over_voltage                == 1 ||
        error_flags.under_voltage               == 1 ||
        error_flags.over_temperature_charge     == 1 ||
        error_flags.over_temperature_discharge  == 1 ||
        error_flags.under_temperature_charge    == 1 ||
        error_flags.under_temperature_discharge == 1 ||
        error_flags.crc_error                   == 1 ||
        error_flags.currentsensorresponding     == 1 ||
        error_flags.can_timing_cc               == 1 ||
        error_flags.can_timing                  == 1 ) {
        retVal = E_NOT_OK;
        error_flags.general_error = 1;

    } else {
        retVal = E_OK;
        error_flags.general_error = 0;
    }

    DATA_StoreDataBlock(&error_flags, DATA_BLOCK_ID_SYSTEMSTATE);
    return retVal;
}
