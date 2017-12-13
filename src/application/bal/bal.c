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
 * @file    bal.c
 * @author  foxBMS Team
 * @date    26.02.2016 (date of creation)
 * @ingroup DRIVERS
 * @prefix  BAL
 *
 * @brief   Driver for the Balancing module
 *
 */

/*================== Includes =============================================*/
#include "bal.h"
#include "general.h"
#include "mcu.h"
#include "diag.h"
#include "cmsis_os.h"
#include "database.h"
/*================== Macros and Definitions ===============================*/

/**
 * Saves the last state and the last substate
 */
#define BAL_SAVELASTSTATES()    bal_state.laststate=bal_state.state; \
                                bal_state.lastsubstate = bal_state.substate


/*================== Constant and Variable Definitions ====================*/
static DATA_BLOCK_CURRENT_s bal_current;
static DATA_BLOCK_MINMAX_s bal_minmax;

/**
 * contains the state of the contactor state machine
 *
 */
static BAL_STATE_s bal_state = {
    .timer                  = 0,
    .statereq               = BAL_STATE_NO_REQUEST,
    .state                  = BAL_STATEMACH_UNINITIALIZED,
    .substate               = BAL_ENTRY,
    .laststate              = BAL_STATEMACH_UNINITIALIZED,
    .lastsubstate           = 0,
    .triggerentry           = 0,
    .ErrRequestCounter      = 0,
    .active                 = FALSE,
    .resting                = TRUE,
    .rest_timer             = BAL_TIME_BEFORE_BALANCING_S*100,
    .balancing_threshold    = BAL_THRESHOLD_MV + BAL_HYSTERESIS_MV,
};


/*================== Function Prototypes ==================================*/

static BAL_RETURN_TYPE_e BAL_CheckStateRequest(BAL_STATE_REQUEST_e statereq);
static BAL_STATE_REQUEST_e BAL_GetStateRequest(void);
static BAL_STATE_REQUEST_e BAL_TransferStateRequest(void);
static uint8_t BAL_CheckReEntrance(void);

static void BAL_Init(void);
static void BAL_Deactivate(void);
static uint8_t BAL_Activate(void);


/*================== Function Implementations =============================*/
static void BAL_Init(void) {
    DATA_BLOCK_BALANCING_CONTROL_s bal_balancing;

    DATA_GetTable(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);
    bal_balancing.enable_balancing = 0;
    DATA_StoreDataBlock(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);
}

static void BAL_Deactivate(void) {
    DATA_BLOCK_BALANCING_CONTROL_s bal_balancing;
    uint16_t i;

    DATA_GetTable(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);

    for (i=0;i<BS_NR_OF_BAT_CELLS;i++) {
        bal_balancing.value[i] = 0;
    }

    bal_balancing.previous_timestamp = bal_balancing.timestamp;
    bal_balancing.timestamp = MCU_GetTimeStamp();
    DATA_StoreDataBlock(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);

}

static uint8_t BAL_Activate(void) {
    static DATA_BLOCK_BALANCING_CONTROL_s bal_balancing;
    static DATA_BLOCK_CELLVOLTAGE_s bal_cellvoltage;
    static DATA_BLOCK_MINMAX_s bal_minmax;
    uint32_t i = 0;
    uint16_t min = 0;
    uint8_t finished = TRUE;

    DATA_GetTable(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);
    DATA_GetTable(&bal_cellvoltage, DATA_BLOCK_ID_CELLVOLTAGE);
    DATA_GetTable(&bal_minmax, DATA_BLOCK_ID_MINMAX);

    min = bal_minmax.voltage_min;

    for (i=0;i<BS_NR_OF_BAT_CELLS;i++) {
        if ( bal_cellvoltage.voltage[i] > min+bal_state.balancing_threshold ) {
            bal_balancing.value[i] = 1;
            finished = FALSE;
        } else {
            bal_balancing.value[i] = 0;
        }
    }


    bal_balancing.previous_timestamp = bal_balancing.timestamp;
    bal_balancing.timestamp = MCU_GetTimeStamp();
    DATA_StoreDataBlock(&bal_balancing, DATA_BLOCK_ID_BALANCING_CONTROL_VALUES);

    return finished;

}



/**
 * @brief   re-entrance check of BAL state machine trigger function
 *
 * This function is not re-entrant and should only be called time- or event-triggered.
 * It increments the triggerentry counter from the state variable bal_state.
 * It should never be called by two different processes, so if it is the case, triggerentry
 * should never be higher than 0 when this function is called.
 *
 *
 * @return  retval  0 if no further instance of the function is active, 0xff else
 *
 */
static uint8_t BAL_CheckReEntrance(void)
{
    uint8_t retval=0;

    taskENTER_CRITICAL();
    if(!bal_state.triggerentry)
    {
        bal_state.triggerentry++;
    }
    else
        retval = 0xFF;    // multiple calls of function
    taskEXIT_CRITICAL();

    return (retval);
}



/**
 * @brief   gets the current state request.
 *
 * This function is used in the functioning of the BAL state machine.
 *
 * @return  retval  current state request, taken from BAL_STATE_REQUEST_e
 */
static BAL_STATE_REQUEST_e BAL_GetStateRequest(void) {

    BAL_STATE_REQUEST_e retval = BAL_STATE_NO_REQUEST;

    taskENTER_CRITICAL();
    retval    = bal_state.statereq;
    taskEXIT_CRITICAL();

    return (retval);
}


/**
 * @brief   gets the current state.
 *
 * This function is used in the functioning of the BAL state machine.
 *
 * @return  current state, taken from BAL_STATEMACH_e
 */
BAL_STATEMACH_e BAL_GetState(void) {
    return (bal_state.state);
}



/**
 * @brief   transfers the current state request to the state machine.
 *
 * This function takes the current state request from bal_state and transfers it to the state machine.
 * It resets the value from bal_state to BAL_STATE_NO_REQUEST
 *
 * @return  retVal          current state request, taken from BAL_STATE_REQUEST_e
 *
 */
static BAL_STATE_REQUEST_e BAL_TransferStateRequest(void)
{
    BAL_STATE_REQUEST_e retval = BAL_STATE_NO_REQUEST;

    taskENTER_CRITICAL();
    retval    = bal_state.statereq;
    bal_state.statereq = BAL_STATE_NO_REQUEST;
    taskEXIT_CRITICAL();

    return (retval);
}


/**
 * @brief   sets the current state request of the state variable bal_state.
 *
 * This function is used to make a state request to the state machine,e.g, start voltage measurement,
 * read result of voltage measurement, re-initialization
 * It calls BAL_CheckStateRequest() to check if the request is valid.
 * The state request is rejected if is not valid.
 * The result of the check is returned immediately, so that the requester can act in case
 * it made a non-valid state request.
 *
 * @param   statereq                state request to set
 *
 * @return  retVal                  current state request, taken from BAL_STATE_REQUEST_e
 */
BAL_RETURN_TYPE_e BAL_SetStateRequest(BAL_STATE_REQUEST_e statereq)
{
    BAL_RETURN_TYPE_e retVal = BAL_STATE_NO_REQUEST;

    taskENTER_CRITICAL();
    retVal=BAL_CheckStateRequest(statereq);

    if (retVal==BAL_OK)
        {
            bal_state.statereq   = statereq;
        }
    taskEXIT_CRITICAL();

    return (retVal);
}



/**
 * @brief   checks the state requests that are made.
 *
 * This function checks the validity of the state requests.
 * The resuls of the checked is returned immediately.
 *
 * @param   statereq    state request to be checked
 *
 * @return              result of the state request that was made, taken from BAL_RETURN_TYPE_e
 */
static BAL_RETURN_TYPE_e BAL_CheckStateRequest(BAL_STATE_REQUEST_e statereq) {

    if (statereq == BAL_STATE_ERROR_REQUEST){
        return BAL_OK;
    }

    if (bal_state.statereq == BAL_STATE_NO_REQUEST){
        //init only allowed from the uninitialized state
        if (statereq == BAL_STATE_INIT_REQUEST) {
            if (bal_state.state==BAL_STATEMACH_UNINITIALIZED) {
                return BAL_OK;
            } else {
                return BAL_ALREADY_INITIALIZED;
            }
        }

        if( (statereq == BAL_STATE_ACTIVE_REQUEST)               ||
            (statereq == BAL_STATE_INACTIVE_REQUEST)             ||
            (statereq == BAL_STATE_ACTIVE_OVERRIDE_REQUEST)      ||
            (statereq == BAL_STATE_INACTIVE_OVERRIDE_REQUEST)    ||
            (statereq == BAL_STATE_STOP_OVERRIDE_ACTIVE_REQUEST) ||
            (statereq == BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST) ){
            return BAL_OK;
        }
        else {
            return BAL_ILLEGAL_REQUEST;
        }
    }
    else {
        return BAL_REQUEST_PENDING;
    }

}







/**
 * @brief   trigger function for the BAL driver state machine.
 *
 * This function contains the sequence of events in the BAL state machine.
 * It must be called time-triggered, every 1ms.
 *
 * @return  void
 */
void BAL_Trigger(void)
{

    BAL_STATE_REQUEST_e statereq = BAL_STATE_NO_REQUEST;
    uint8_t finished = FALSE;

    if (bal_state.rest_timer > 0) {
        bal_state.rest_timer--;
    }

    // Check re-entrance of function
    if (BAL_CheckReEntrance())
        return;

    if(bal_state.timer)
    {
        if(--bal_state.timer)
        {
            bal_state.triggerentry--;
            return;    // handle state machine only if timer has elapsed
        }
    }

    DATA_GetTable(&bal_current, DATA_BLOCK_ID_CURRENT);
    if (bal_current.current < 0.0) {
        bal_current.current = -bal_current.current;
    }
    if (bal_current.current<BAL_REST_CURRENT) {
        if (bal_state.resting == FALSE) {
            bal_state.resting = TRUE;
            bal_state.rest_timer = BAL_TIME_BEFORE_BALANCING_S*100;
        }
    }
    else {
        bal_state.resting = FALSE;
    }


    switch(bal_state.state) {

        /****************************UNINITIALIZED***********************************/
        case BAL_STATEMACH_UNINITIALIZED:
            // waiting for Initialization Request
            statereq = BAL_TransferStateRequest();
            if(statereq == BAL_STATE_INIT_REQUEST){
                BAL_SAVELASTSTATES();
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_INITIALIZATION;
                bal_state.substate = BAL_ENTRY;
            }
            else if(statereq == BAL_STATE_NO_REQUEST){
                // no actual request pending //
            }
            else{
                bal_state.ErrRequestCounter++;   // illegal request pending
            }
            break;


        /****************************INITIALIZATION**********************************/
        case BAL_STATEMACH_INITIALIZATION:

            BAL_SAVELASTSTATES();
            BAL_Init();

            bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
            bal_state.state = BAL_STATEMACH_INITIALIZED;
            bal_state.substate = BAL_ENTRY;

            break;

        /****************************INITIALIZED*************************************/
        case BAL_STATEMACH_INITIALIZED:
            BAL_SAVELASTSTATES();
            bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
            bal_state.state = BAL_STATEMACH_INACTIVE;
            bal_state.substate = BAL_ENTRY;

            break;

        /****************************INACTIVE*************************************/
        case BAL_STATEMACH_INACTIVE:
            BAL_SAVELASTSTATES();
            if (bal_state.active == TRUE) {
                BAL_Deactivate();
                bal_state.active = FALSE;
            }
            bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;

            statereq=BAL_TransferStateRequest();
            if (statereq == BAL_STATE_ACTIVE_OVERRIDE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_ACTIVE_OVERRIDE;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            if (statereq == BAL_STATE_INACTIVE_OVERRIDE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_INACTIVE_OVERRIDE;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            if (statereq == BAL_STATE_ACTIVE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_ACTIVE;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            break;

        /****************************ACTIVE*************************************/
        case BAL_STATEMACH_ACTIVE:
            BAL_SAVELASTSTATES();
            bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;

            statereq=BAL_TransferStateRequest();
            if (statereq == BAL_STATE_ACTIVE_OVERRIDE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_ACTIVE_OVERRIDE;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            if (statereq == BAL_STATE_INACTIVE_OVERRIDE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_INACTIVE_OVERRIDE;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            if (statereq == BAL_STATE_INACTIVE_REQUEST){
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.state = BAL_STATEMACH_INACTIVE;
                bal_state.substate = BAL_ENTRY;
                break;
            }

            if (bal_state.substate == BAL_ENTRY){
                if (bal_state.resting == TRUE) {
                    if (bal_state.rest_timer == 0) {
                        bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                        bal_state.substate = BAL_BALANCE_ACTIVE;
                        break;
                    }
                    else {
                        bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                        bal_state.substate = BAL_BALANCE_INACTIVE;
                        break;
                    }
                }
                else {
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.substate = BAL_BALANCE_INACTIVE;
                }
                break;
            }
            else if (bal_state.substate == BAL_BALANCE_ACTIVE){
                DATA_GetTable(&bal_minmax,DATA_BLOCK_ID_MINMAX);
                //do not balance under a certain voltage level
                if (bal_minmax.voltage_min>=BAL_LOWER_VOLTAGE_LIMIT_MV) {
                    finished = BAL_Activate();
                }
                else {
                    bal_state.substate = BAL_BALANCE_INACTIVE;
                    break;
                }
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                if (finished == FALSE) {
                    bal_state.active = TRUE;
                    bal_state.balancing_threshold = BAL_THRESHOLD_MV;
                    bal_state.substate = BAL_ENTRY;
                }
                else {
                    bal_state.substate = BAL_BALANCE_ACTIVE_FINISHED;
                }
                break;
            }
            else if (bal_state.substate == BAL_BALANCE_INACTIVE){
                if (bal_state.active == TRUE) {
                    BAL_Deactivate();
                    bal_state.balancing_threshold = BAL_THRESHOLD_MV + BAL_HYSTERESIS_MV;
                    bal_state.active = FALSE;
                }
                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                bal_state.substate = BAL_ENTRY;
                break;
            }
            else if (bal_state.substate == BAL_BALANCE_ACTIVE_FINISHED){
                //send CAN message if needed
                if (bal_state.active == TRUE) {
                    BAL_Deactivate();
                }
                bal_state.balancing_threshold = BAL_THRESHOLD_MV + BAL_HYSTERESIS_MV;
                bal_state.active = FALSE;
                bal_state.timer = BAL_STATEMACH_LONGTIME_10MS; //balancing reached: wait longer time to avoid oscillations
                bal_state.substate = BAL_STATEMACH_INACTIVE;
                bal_state.substate = BAL_ENTRY;
                break;
            }

            break;

            /****************************INACTIVE OVERRIDE*************************************/
            case BAL_STATEMACH_INACTIVE_OVERRIDE:
                BAL_SAVELASTSTATES();

                BAL_Deactivate();
                bal_state.active = FALSE;

                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;

                statereq=BAL_TransferStateRequest();
                if (statereq == BAL_STATE_ACTIVE_OVERRIDE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_ACTIVE_OVERRIDE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }
                if (statereq == BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_INACTIVE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }
                if (statereq == BAL_STATE_STOP_OVERRIDE_ACTIVE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_ACTIVE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }

                break;

            /****************************ACTIVE OVERRIDE*************************************/
            case BAL_STATEMACH_ACTIVE_OVERRIDE:
                BAL_SAVELASTSTATES();

                bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;

                finished = BAL_Activate();
                bal_state.active = TRUE;

                statereq=BAL_TransferStateRequest();

                if (finished == TRUE) {
                    bal_state.state = BAL_BALANCE_ACTIVE;
                    bal_state.substate = BAL_BALANCE_ACTIVE_FINISHED;
                    break;
                }

                if (statereq == BAL_STATE_INACTIVE_OVERRIDE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_INACTIVE_OVERRIDE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }
                if (statereq == BAL_STATE_STOP_OVERRIDE_INACTIVE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_INACTIVE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }
                if (statereq == BAL_STATE_STOP_OVERRIDE_ACTIVE_REQUEST){
                    bal_state.timer = BAL_STATEMACH_SHORTTIME_10MS;
                    bal_state.state = BAL_STATEMACH_ACTIVE;
                    bal_state.substate = BAL_ENTRY;
                    break;
                }

                break;

        default:
            break;

    } // end switch(bal_state.state)

    bal_state.triggerentry--;

}



























