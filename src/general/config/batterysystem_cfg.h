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
 * @file    batterysystem_cfg.h
 * @author  foxBMS Team
 * @date    20.02.2017 (date of creation)
 * @ingroup BATTER_SYSTEM_CONF
 * @prefix  BS
 *
 * @brief   Configuration of the battery system (e.g., number of battery modules, battery cells, temperature sensors)
 *
 * This files contains basic macros of the battery system in order to derive needed inputs
 * in other parts of the software. These macros are all depended on the hardware.
 *
 */

#ifndef BATTERYSYSTEM_CFG_H_
#define BATTERYSYSTEM_CFG_H_

/*================== Includes =============================================*/
#include "general.h"

/*================== Macros and Definitions ===============================*/


/**
 * Symbolic names for current flow direction in the battery
 */
typedef enum {
    BS_CURRENT_CHARGE     = 0,    /*!<       */
    BS_CURRENT_DISCHARGE  = 1,    /*!<       */
} BS_CURRENT_DIRECTION_e;


/**
 * Define if discharge current are positive negative, default is positive
 */
#define POSITIVE_DISCHARGE_CURRENT          TRUE

/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * number of modules in battery pack
 * \par Type:
 * int
 * \par Default:
 * 2
*/
#define BS_NR_OF_MODULES                           1

/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * number of battery cells per battery module (parallel cells are counted as one)
 * \par Type:
 * int
 * \par Default:
 * 12
*/
#define BS_NR_OF_BAT_CELLS_PER_MODULE               12

#if BS_NR_OF_BAT_CELLS_PER_MODULE<=12
    #define BS_MAX_SUPPORTED_CELLS         12
#elif BS_NR_OF_BAT_CELLS_PER_MODULE<=15
    #define BS_MAX_SUPPORTED_CELLS         15
#elif BS_NR_OF_BAT_CELLS_PER_MODULE<=18
    #define BS_MAX_SUPPORTED_CELLS         18
#else
    #error "Unsupported number of cells per module, higher than 18"
#endif

/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * Number of LTC-ICs per battery module
 * \par Type:
 * int
 * \par Default:
 * 1
*/
#define BS_NR_OF_LTC_PER_MODULE                     1
/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * number of temperature sensors per battery module
 * \par Type:
 * int
 * \par Default:
 * 6
*/
#define BS_NR_OF_TEMP_SENSORS_PER_MODULE            4


#define BS_NR_OF_BAT_CELLS                         (BS_NR_OF_MODULES * BS_NR_OF_BAT_CELLS_PER_MODULE)
#define BS_NR_OF_TEMP_SENSORS                      (BS_NR_OF_MODULES * BS_NR_OF_TEMP_SENSORS_PER_MODULE)


/**
 * If set to FALSE, foxBMS does not check for the presence of a current sensor.
 * If set to TRUE, foxBMS checks for the presence of a current sensor. If sensor stops responding during runtime, an error is raised
 *
*/
#define CURRENT_SENSOR_PRESENT               TRUE

/**
 * If set to FALSE, foxBMS does not check CAN timing.
 * If set to TRUE, foxBMS checks CAN timing. A valid request must come every 100ms, within the 95-150ms window.
 *
*/
#define CHECK_CAN_TIMING                     TRUE

/**
 * If set to TRUE, balancing is deactivated completely.
 * If set to FALSE, foxBMS checks when balancing must be done and activates it accordingly.
 *
*/
#define BALANCING_DEFAULT_INACTIVE           TRUE


/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * number of pack voltage inputs measured by current sensors (like IVT-MOD)
 * \par Type:
 * int
 * \par Default:
 * 3
*/
#define BS_NR_OF_VOLTAGES_FROM_CURRENT_SENSOR      3

/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * number of contactors
 * On the foxBMS Basic Board 6 contactors are supported. On the foxBMS
 * Basic Extension board one can use 3 more contactors.
 * \par Type:
 * int
 * \par Default:
 * 3
*/
#define BS_NR_OF_CONTACTORS                        3

/**
 * @ingroup CONFIG_BATTERYSYSTEM
 * separation of charge and discharge power line
 * \par Type:
 * select(2)
 * \par Default:
 * 1
*/
#define BS_SEPARATE_POWERLINES 0
//#define BS_SEPARATE_POWERLINES 1

#if BS_NR_OF_CONTACTORS > 3 && BS_SEPARATE_POWERLINES == 0
#error "Configuration mismatch: Can't use BS_SEPARATE_POWERLINES with only 3 contactors"
#endif  //


/*================== Constant and Variable Definitions ====================*/

/*================== Function Prototypes ==================================*/

/**
 * @brief   Checks current direction, read current from database
 *
 * @return  BS_CURRENT_DISCHARGE or BS_CURRENT_CHARGE depending
 *          on current direction ( (type: BS_CURRENT_DIRECTION_e)
 */
extern BS_CURRENT_DIRECTION_e BS_CheckCurrent_Direction(void);

 /**
  * @brief   Checks current direction, current value as function parameter
  *
  * @return  BS_CURRENT_DISCHARGE or BS_CURRENT_CHARGE depending
  *          on current direction ( (type: BS_CURRENT_DIRECTION_e)
  */
extern BS_CURRENT_DIRECTION_e BS_CheckCurrentValue_Direction(float current);

/*================== Function Implementations =============================*/

#endif /* BATTERYSYSTEM_CFG_H_ */
