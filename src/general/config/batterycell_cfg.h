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
 * @file    batterycell_cfg.h
 * @author  foxBMS Team
 * @date    14.03.2017 (date of creation)
 * @ingroup BATTERY_CELL_CONF
 * @prefix  BC
 *
 * @brief   Configuration of the battery cell (e.g., minimum and maximum cell voltage)
 *
 * This files contains basic macros of the battery cell in order to derive needed inputs
 * in other parts of the software. These macros are all depended on the hardware.
 *
 */

#ifndef BATTERYCELL_CFG_H_
#define BATTERYCELL_CFG_H_


/**
 * @ingroup CONFIG_BATTERYCELL
 * maximum temperature safe operating limit during discharge process
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * °C
 * \par Default:
 * 55
*/
#define BC_TEMPMAX_DISCHARGE 55

/**
 * @ingroup CONFIG_BATTERYCELL
 * minimum operating temperature safe operating limit during discharge process
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * °C
 * \par Default:
 * -20
*/
#define BC_TEMPMIN_DISCHARGE -20 //°C

/**
 * @ingroup CONFIG_BATTERYCELL
 * maximum operating temperature safe operating limit during charge process
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * °C
 * \par Default:
 * 45
*/
#define BC_TEMPMAX_CHARGE 45

/**
 * @ingroup CONFIG_BATTERYCELL
 * minimum operating temperature safe operating limit during charge process
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * °C
 * \par Default:
 * 0
*/
#define BC_TEMPMIN_CHARGE -20

/**
 * @ingroup CONFIG_BATTERYCELL
 * maximum cell voltage safe operating limit
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * mV
 * \par Default:
 * 2800
*/
#define BC_VOLTMAX 2800

/**
 * @ingroup CONFIG_BATTERYCELL
 * nominal cell voltage according to datasheet
 * \par Type:
 * int
 * \par Unit:
 * mV
 * \par Default:
 * 2500
*/
#define BC_VOLT_NOMINAL 2500
/**
 * @ingroup CONFIG_BATTERYCELL
 * minimum cell voltage safe operating limit
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * mV
 * \par Default:
 * 1700
*/
#define BC_VOLTMIN 1700

/**
 * @ingroup CONFIG_BATTERYCELL
 * maximum current safe operating limit
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * mA
 * \par Default:
 * 180000
*/
#define BC_CURRENTMAX_DISCHARGE 180000

/**
 * @ingroup CONFIG_BATTERYCELL
 * maximum current safe operating limit
 * When limit is violated, error state is requested and contactors will open.
 * \par Type:
 * int
 * \par Unit:
 * mA
 * \par Default:
 * 180000
*/
#define BC_CURRENTMAX_CHARGE 180000

/*
 * the cell capacity used for SOC calculation, in this case Ah counting
 * @var battery cell capacity
 * @type int
 * @validator 0<x
 * @unit mAh
 * @default 3500
 * @group
 */
#define BC_CAPACITY 3500

#endif /* BATTERYCELL_CFG_H_ */
