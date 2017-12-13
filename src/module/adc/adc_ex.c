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
 * @file    adc_ex.c
 * @author  foxBMS Team
 * @date    23.10.2017 (date of creation)
 * @ingroup DRIVERS
 * @prefix  ADC
 *
 * @brief   Driver for the analog to digital converter
 *
 * This external ADC module provides support for analog/digital conversion.
 * It must be initialized during startup.
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
#include "adc_ex.h"

#include "database.h"
#include "adc.h"
#include "mcu.h"

/*================== Macros and Definitions ===============================*/
#define ADC_CONVERT             0
#define ADC_WAITFORCONVERSION   1
#define ADC_STOREDATA           2

/* Macros for converting voltage -> temperature */
#define ADC_VBAT_VOLTAGE_DIVIDER    4.0
#define ADC_FULL_RANGE              4095.0        // 12bit adc converter --> range [0, ..., 4095]
#define ADC_VREF_EXT                2.5
#define ADC_V25                     0.76
#define ADC_AVG_SLOPE               2.5

/*================== Constant and Variable Definitions ====================*/
static ADC_ChannelConfTypeDef adc_cfg = {
    .Channel = ADC_CHANNEL_VBAT,
    .Rank = 1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES,
    .Offset = 0,
};

static DATA_BLOCK_ADC_s adc_tab = {
    .vbat = 0,
    .vbat_previous_timestamp = 0,
    .vbat_timestamp = 0,
    .temperature = 0.0,
    .temperature_previous_timestamp = 0,
    .temperature_timestamp = 0,
    .state_vbat = 0,
    .state_temperature = 0,
};

static uint8_t adc_conversion_state = ADC_CONVERT;

/*================== Function Prototypes ==================================*/

/*================== Function Implementations =============================*/

void ADC_Ctrl(void) {
    static uint8_t vbat = 0;

    adc_cfg.Rank = 1;
    adc_cfg.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    adc_cfg.Offset = 0;

    /* Alternate ADC conversion between temperature sensor and battery voltage */
    if (adc_conversion_state == ADC_CONVERT) {
        if (vbat == 0) {
            vbat = 1;
            adc_cfg.Channel = ADC_CHANNEL_VBAT;
        } else if (vbat == 1) {
            vbat = 0;
            adc_cfg.Channel = ADC_CHANNEL_TEMPSENSOR;
        }
    }

    if (adc_conversion_state == ADC_CONVERT) {
        /* Configure ADC Channel */
        HAL_ADC_ConfigChannel(&adc_devices[0], &adc_cfg);
        /* Enable conversion */
        ADC_Convert(&adc_devices[0]);
        adc_conversion_state = ADC_WAITFORCONVERSION;
    } else if (adc_conversion_state == ADC_STOREDATA) {
        /* Store data in database */
        DATA_StoreDataBlock(&adc_tab, DATA_BLOCK_ID_ADC);
        adc_conversion_state = ADC_CONVERT;
    }
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle) {
    uint32_t raw_voltage = 0;
    float scaled_voltage = 0.0;
    float scaled_temperature = 0.0;

    /* Disable interrupt and stop ADC conversion */
    HAL_ADC_Stop_IT(AdcHandle);
    raw_voltage = HAL_ADC_GetValue(AdcHandle);

    if (adc_cfg.Channel == ADC_CHANNEL_VBAT) {
        scaled_voltage = ((float)(raw_voltage)*(ADC_VREF_EXT)*ADC_VBAT_VOLTAGE_DIVIDER)/(ADC_FULL_RANGE);
        adc_tab.vbat = scaled_voltage;
        adc_tab.vbat_previous_timestamp = adc_tab.vbat_timestamp;
        adc_tab.vbat_timestamp = MCU_GetTimeStamp();
        adc_tab.state_vbat++;
    }

    if (adc_cfg.Channel == ADC_CHANNEL_TEMPSENSOR) {
        scaled_voltage = ((float)(raw_voltage)*(1000.0*(float)(ADC_VREF_EXT)))/(ADC_FULL_RANGE);
        scaled_temperature = (scaled_voltage - ADC_V25)/(1000.0*ADC_AVG_SLOPE) + 25.0;
        adc_tab.temperature = scaled_temperature;
        adc_tab.temperature_previous_timestamp = adc_tab.temperature_timestamp;
        adc_tab.temperature_timestamp = MCU_GetTimeStamp();
        adc_tab.state_temperature++;
    }

    adc_conversion_state = ADC_STOREDATA;
}

