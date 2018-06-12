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
 * @file    database_cfg.h
 * @author  foxBMS Team
 * @date    18.08.2015 (date of creation)
 * @ingroup ENGINE_CONF
 * @prefix  DATA
 *
 * @brief   Database configuration header
 *
 * Provides interfaces to database configuration
 *
 */

#ifndef DATABASE_CFG_H_
#define DATABASE_CFG_H_

/*================== Includes =============================================*/
#include "batterysystem_cfg.h"

/*================== Macros and Definitions ===============================*/

/**
 * @brief maximum amount of data block
 *
 * this value is extendible but limitation is done due to RAM consumption and performance
 */
#define DATA_MAX_BLOCK_NR                20        /* max 20 Blocks currently supported*/

/**
 * @brief data block identification number
 */
typedef enum {
    DATA_BLOCK_1        = 0,
    DATA_BLOCK_2        = 1,
    DATA_BLOCK_3        = 2,
    DATA_BLOCK_4        = 3,
    DATA_BLOCK_5        = 4,
    DATA_BLOCK_6        = 5,
    DATA_BLOCK_7        = 6,
    DATA_BLOCK_8        = 7,
    DATA_BLOCK_9        = 8,
    DATA_BLOCK_10       = 9,
    DATA_BLOCK_11       = 10,
    DATA_BLOCK_12       = 11,
    DATA_BLOCK_13       = 12,
    DATA_BLOCK_14       = 13,
    DATA_BLOCK_15       = 14,
    DATA_BLOCK_16       = 15,
    DATA_BLOCK_17       = 16,
    DATA_BLOCK_18       = 17,
    DATA_BLOCK_19       = 18,
    DATA_BLOCK_20       = 19,
    DATA_BLOCK_MAX      = DATA_MAX_BLOCK_NR,
} DATA_BLOCK_ID_TYPE_e;


/**
 * @brief data block access types
 *
 * read or write access types
 */
typedef enum {
    WRITE_ACCESS    = 0,    /*!< write access to data block   */
    READ_ACCESS     = 1,    /*!< read access to data block   */
} DATA_BLOCK_ACCESS_TYPE_e;

/**
 * @brief data block consistency types
 *
 * recommendation: use single buffer for small data (e.g.,one variable) and less concurrent read and write accesses
 */
typedef enum {
    // Init-Sequence
    SINGLE_BUFFERING    = 1,    /*!< single buffering   */
    DOUBLE_BUFFERING    = 2,    /*!< double buffering   */
    // TRIPLEBUFFERING     = 3,    /* actually not supported*/
} DATA_BLOCK_CONSISTENCY_TYPE_e;

/**
 * configuration struct of database channel (data block)
 */
typedef struct {
    void *blockptr;
    uint16_t datalength;
    DATA_BLOCK_CONSISTENCY_TYPE_e buffertype;
} DATA_BASE_HEADER_s;

/**
 * configuration struct of database device
 */
typedef struct {
    uint8_t nr_of_blockheader;
    DATA_BASE_HEADER_s *blockheaderptr;
} DATA_BASE_HEADER_DEV_s;


/*================== Macros and Definitions [USER CONFIGURATION] =============*/
// FIXME comments doxygen, is comment necessary?
/*Macros and Definitions for User Configuration*/
#define     DATA_BLOCK_ID_CELLVOLTAGE                   DATA_BLOCK_1
#define     DATA_BLOCK_ID_CELLTEMPERATURE               DATA_BLOCK_2
#define     DATA_BLOCK_ID_SOX                           DATA_BLOCK_3
#define     DATA_BLOCK_ID_BALANCING_CONTROL_VALUES      DATA_BLOCK_4
#define     DATA_BLOCK_ID_BALANCING_FEEDBACK_VALUES     DATA_BLOCK_5
#define     DATA_BLOCK_ID_CURRENT                       DATA_BLOCK_6
#define     DATA_BLOCK_ID_ADC                           DATA_BLOCK_7
#define     DATA_BLOCK_ID_STATEREQUEST                  DATA_BLOCK_8
#define     DATA_BLOCK_ID_MINMAX                        DATA_BLOCK_9
#define     DATA_BLOCK_ID_ISOGUARD                      DATA_BLOCK_10
#define     DATA_BLOCK_ID_SLAVE_CONTROL                 DATA_BLOCK_11
#define     DATA_BLOCK_ID_OPEN_WIRE_CHECK               DATA_BLOCK_12
#define     DATA_BLOCK_ID_LTC_DEVICE_PARAMETER          DATA_BLOCK_13
#define     DATA_BLOCK_ID_LTC_ACCURACY                  DATA_BLOCK_14
#define     DATA_BLOCK_ID_ERRORSTATE                    DATA_BLOCK_15
#define     DATA_BLOCK_ID_MOV_MEAN                      DATA_BLOCK_16
#define     DATA_BLOCK_ID_CONTFEEDBACK                  DATA_BLOCK_17
#define     DATA_BLOCK_ID_ILCKFEEDBACK                  DATA_BLOCK_18
#define     DATA_BLOCK_ID_SYSTEMSTATE                   DATA_BLOCK_19

/**
 * data block struct of cell voltage
 */
typedef struct {
    uint16_t voltage[BS_NR_OF_BAT_CELLS];       /*!< unit: mV                                   */
    uint32_t valid_voltPECs[BS_NR_OF_MODULES];  /*!< bitmask if PEC was okay. 0->ok, 1->error   */
    uint32_t sumOfCells[BS_NR_OF_MODULES];      /*!< unit: mV                                   */
    uint8_t valid_socPECs[BS_NR_OF_MODULES];   /*!< 0 -> if PEC okay; 1 -> PEC error           */
    uint32_t previous_timestamp;                /*!< timestamp of last database entry           */
    uint32_t timestamp;                         /*!< timestamp of database entry                */
    uint8_t state;                              /*!< for future use                             */
} DATA_BLOCK_CELLVOLTAGE_s;

/**
 * data block struct of cell voltage
 */
typedef struct {
    uint8_t openwire[BS_NR_OF_BAT_CELLS];  /*!< 1 -> open wire, 0 -> everything ok */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry     */
    uint32_t timestamp;                 /*!< timestamp of database entry          */
    uint8_t state;                      /*!< for future use                       */
} DATA_BLOCK_OPENWIRE_s;

/**
 * data block struct of cell temperatures
 */
typedef struct {
    int16_t temperature[BS_NR_OF_TEMP_SENSORS];             /*!< unit: degree Celsius                       */
    uint16_t valid_temperaturePECs[BS_NR_OF_MODULES];  /*!< bitmask if PEC was okay. 0->ok, 1->error   */
    uint32_t previous_timestamp;                            /*!< timestamp of last database entry           */
    uint32_t timestamp;                                     /*!< timestamp of database entry                */
    uint8_t state;                                          /*!< for future use                             */
} DATA_BLOCK_CELLTEMPERATURE_s;

/**
 * data block struct of sox
 */
typedef struct {
    float soc_mean;                     /*!< 0.0 <= soc_mean <= 100.0           */
    float soc_min;                      /*!< 0.0 <= soc_min <= 100.0            */
    float soc_max;                      /*!< 0.0 <= soc_max <= 100.0            */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry   */
    uint32_t timestamp;                 /*!< timestamp of database entry        */
    uint8_t state;                      /*!<                                    */
    float sof_continuous_charge;        /*!<                                    */
    float sof_continuous_discharge;     /*!<                                    */
    float sof_peak_charge;              /*!<                                    */
    float sof_peak_discharge;           /*!<                                    */
} DATA_BLOCK_SOX_s;


/*  data structure declaration of DATA_BLOCK_BALANCING_CONTROL */
typedef struct {
    uint16_t value[BS_NR_OF_BAT_CELLS];    /*!< */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry           */
    uint32_t timestamp;                 /*!< timestamp of database entry                */
    uint8_t enable_balancing;           /*!< Switch for enabling/disabling balancing    */
    uint8_t threshold;                  /*!< balancing threshold in mV                  */
    uint8_t request;                     /*!< balancing request per CAN                 */
    uint8_t state;                      /*!< for future use                             */
} DATA_BLOCK_BALANCING_CONTROL_s;

/*  data structure declaration of DATA_BLOCK_USER_IO_CONTROL */
typedef struct {
    uint8_t io_value_out[BS_NR_OF_MODULES];   /*!< data to be written to the port expander    */
    uint8_t io_value_in[BS_NR_OF_MODULES];    /*!< data read from to the port expander        */
    uint8_t eeprom_value_write[BS_NR_OF_MODULES];   /*!< data to be written to the slave EEPROM    */
    uint8_t eeprom_value_read[BS_NR_OF_MODULES];    /*!< data read from to the slave EEPROM        */
    uint8_t external_sensor_temperature[BS_NR_OF_MODULES];    /*!< temperature from the external sensor on slave   */
    uint32_t eeprom_read_address_to_use;                 /*!< address to read from for  slave EEPROM        */
    uint32_t eeprom_read_address_last_used;                 /*!< last address used to read fromfor slave EEPROM        */
    uint32_t eeprom_write_address_to_use;                 /*!< address to write to for slave EEPROM        */
    uint32_t eeprom_write_address_last_used;                 /*!< last address used to write to for slave EEPROM        */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry           */
    uint32_t timestamp;                 /*!< timestamp of database entry                */
    uint8_t state;                      /*!< for future use                             */
} DATA_BLOCK_SLAVE_CONTROL_s;

/**
 * data block struct of cell balancing feedback
 */

typedef struct {
    uint16_t value[BS_NR_OF_MODULES];    /*!< unit: mV (opto-coupler output)     */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry   */
    uint32_t timestamp;                 /*!< timestamp of database entry        */
    uint8_t state;                      /*!< for future use                     */
} DATA_BLOCK_BALANCING_FEEDBACK_s;


/**
 * data block struct of user multiplexer values
 */

typedef struct {
    uint16_t value[8*2*BS_NR_OF_MODULES];              /*!< unit: mV (mux voltage input)       */
    uint32_t previous_timestamp;                    /*!< timestamp of last database entry   */
    uint32_t timestamp;                             /*!< timestamp of database entry        */
    uint8_t state;                                  /*!< for future use                     */
} DATA_BLOCK_USER_MUX_s;

/**
 * data block struct of current measurement
 */
typedef struct {
    float current;                                         /*!< unit: mA                */
    float voltage[BS_NR_OF_VOLTAGES_FROM_CURRENT_SENSOR];  /*!< unit: mV                */
    float temperature;                                     /*!< unit: 0.1°C             */
    float power;                                           /*!< unit: W                */
    float current_counter;                                 /*!< unit: A.s                */
    float energy_counter;                                  /*!< unit: W.h                */
    uint32_t previous_timestamp;                           /*!< timestamp of last current database entry   */
    uint32_t timestamp;                                    /*!< timestamp of current database entry        */
    uint8_t state_current;
    uint8_t state_voltage;
    uint8_t state_temperature;
    uint8_t state_power;
    uint8_t state_cc;
    uint8_t state_ec;
    uint8_t newCurrent;
    uint8_t newPower;
    uint32_t previous_timestamp_cc;                           /*!< timestamp of C-C database entry   */
    uint32_t timestamp_cc;                                    /*!< timestamp of C-C database entry        */
} DATA_BLOCK_CURRENT_s;


/**
 * data block struct of ADC
 */
typedef struct {
    float vbat;  // unit: to be defined
    uint32_t vbat_previous_timestamp;           /*!< timestamp of last database entry of vbat           */
    uint32_t vbat_timestamp;                    /*!< timestamp of database entry of vbat                */
    float temperature;                          /*!<                                                    */
    uint32_t temperature_previous_timestamp;    /*!< timestamp of last database entry of temperature    */
    uint32_t temperature_timestamp;             /*!< timestamp of database entry of temperature         */
    uint8_t state_vbat;                         /*!<                                                    */
    uint8_t state_temperature;                  /*!<                                                    */
} DATA_BLOCK_ADC_s;


/**
 * data block struct of can state request
 */

typedef struct {
    uint8_t state_request;
    uint8_t previous_state_request;
    uint8_t state_request_pending;
    uint32_t timestamp;             /*!< timestamp of database entry        */
    uint32_t previous_timestamp;    /*!< timestamp of last database entry   */
    uint8_t state;
} DATA_BLOCK_STATEREQUEST_s;

/**
 * data block struct of LTC minimum and maximum values
 */
typedef struct {
    uint32_t voltage_mean;
    uint16_t voltage_min;
    uint16_t voltage_module_number_min;
    uint16_t voltage_cell_number_min;
    uint16_t previous_voltage_min;
    uint16_t voltage_max;
    uint16_t voltage_module_number_max;
    uint16_t voltage_cell_number_max;
    uint16_t previous_voltage_max;
    float temperature_mean;
    int16_t temperature_min;
    uint16_t temperature_module_number_min;
    uint16_t temperature_sensor_number_min;
    int16_t temperature_max;
    uint16_t temperature_module_number_max;
    uint16_t temperature_sensor_number_max;
    uint8_t state;
    uint32_t timestamp;             /*!< timestamp of database entry                                        */
    uint32_t previous_timestamp;    /*!< timestamp of last database entry                                   */
} DATA_BLOCK_MINMAX_s;


/**
 * data block struct of isometer measurement
 */
typedef struct {
    uint8_t valid;                  /*!< 0 -> valid, 1 -> resistance unreliable                             */
    uint8_t state;                  /*!< 0 -> resistance/measurement OK , 1 -> resistance too low or error  */
    uint32_t resistance_kOhm;       /*!< insulation resistance measured in kOhm                             */
    uint32_t timestamp;             /*!< timestamp of database entry                                        */
    uint32_t previous_timestamp;    /*!< timestamp of last database entry                                   */
} DATA_BLOCK_ISOMETER_s;


/**
 * data block struct of ltc device parameter
 */
typedef struct {
    uint32_t sumOfCells[BS_NR_OF_MODULES];
    uint8_t valid_sumOfCells[BS_NR_OF_MODULES];         /*!< 0 -> valid, 1 -> unreliable                                        */
    uint16_t dieTemperature[BS_NR_OF_MODULES];          /* die temperature in degree celsius                                    */
    uint8_t valid_dieTemperature[BS_NR_OF_MODULES];     /*!< 0 -> valid, 1 -> unreliable                                        */
    uint32_t analogSupplyVolt[BS_NR_OF_MODULES];        /* voltage in [uV]                                                      */
    uint8_t valid_analogSupplyVolt[BS_NR_OF_MODULES];   /*!< 0 -> valid, 1 -> unreliable                                        */
    uint32_t digitalSupplyVolt[BS_NR_OF_MODULES];       /* voltage in [uV]                                                      */
    uint8_t valid_digitalSupplyVolt[BS_NR_OF_MODULES];  /*!< 0 -> valid, 1 -> unreliable                                        */
    uint32_t valid_cellvoltages[BS_NR_OF_MODULES];      /*!< 0 -> valid, 1 -> invalid, bit0 -> cell 0, bit1 -> cell 1 ...       */
    uint8_t valid_GPIOs[BS_NR_OF_MODULES];              /*!< 0 -> valid, 1 -> invalid, bit0 -> GPIO0, bit1 -> GPIO1 ...         */
    uint8_t valid_LTC[BS_NR_OF_MODULES];                /*!< 0 -> LTC working, 1 -> LTC defect                                  */
    uint32_t timestamp;                                 /*!< timestamp of database entry                                        */
    uint32_t previous_timestamp;                        /*!< timestamp of last database entry                                   */
} DATA_BLOCK_LTC_DEVICE_PARAMETER_s;


/**
 * data block struct of ltc adc accuracy measurement
 */
typedef struct {
    int adc1_deviation[BS_NR_OF_MODULES];               /* ADC1 deviation from 2nd reference */
    int adc2_deviation[BS_NR_OF_MODULES];               /* ADC2 deviation from 2nd reference */
    uint32_t timestamp;
    uint32_t previous_timestamp;
} DATA_BLOCK_LTC_ADC_ACCURACY_s;

/**
 * data block struct of error flags
 */
typedef struct {
    uint8_t general_error;                           /*!< 0 -> no error, 1 -> error         */
    uint8_t currentsensorresponding;                 /*!< 0 -> no error, 1 -> error         */
    uint8_t main_plus;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t main_minus;                              /*!< 0 -> no error, 1 -> error         */
    uint8_t precharge;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t charge_main_plus;                        /*!< 0 -> no error, 1 -> error         */
    uint8_t charge_main_minus;                       /*!< 0 -> no error, 1 -> error         */
    uint8_t charge_precharge;                        /*!< 0 -> no error, 1 -> error         */
    uint8_t interlock;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t over_current_charge;                     /*!< 0 -> no error, 1 -> error         */
    uint8_t over_current_discharge;                  /*!< 0 -> no error, 1 -> error         */
    uint8_t over_voltage;                            /*!< 0 -> no error, 1 -> error         */
    uint8_t under_voltage;                           /*!< 0 -> no error, 1 -> error         */
    uint8_t over_temperature_discharge;              /*!< 0 -> no error, 1 -> error         */
    uint8_t under_temperature_discharge;             /*!< 0 -> no error, 1 -> error         */
    uint8_t over_temperature_charge;                 /*!< 0 -> no error, 1 -> error         */
    uint8_t under_temperature_charge;                /*!< 0 -> no error, 1 -> error         */
    uint8_t crc_error;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t mux_error;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t spi_error;                               /*!< 0 -> no error, 1 -> error         */
    uint8_t can_timing;                              /*!< 0 -> no error, 1 -> error         */
    uint8_t can_timing_cc;                           /*!< 0 -> no error, 1 -> error         */
    uint8_t can_cc_used;                             /*!< 0 -> not present, 1 -> present    */
    uint32_t timestamp;                              /*!< timestamp of database entry       */
    uint32_t previous_timestamp;                     /*!< timestamp of last database entry  */
} DATA_BLOCK_ERRORSTATE_s;

typedef struct {
    float movMean_current_1s;         /*!< current moving mean over the last 1s               */
    float movMean_current_5s;         /*!< current moving mean over the last 5s               */
    float movMean_current_10s;        /*!< current moving mean over the last 10s              */
    float movMean_current_30s;        /*!< current moving mean over the last 30s              */
    float movMean_current_60s;        /*!< current moving mean over the last 60s              */
    float movMean_current_config;     /*!< current moving mean over the last configured time  */
    float movMean_power_1s;           /*!< power moving mean over the last 1s                 */
    float movMean_power_5s;           /*!< power moving mean over the last 5s                 */
    float movMean_power_10s;          /*!< power moving mean over the last 10s                */
    float movMean_power_30s;          /*!< power moving mean over the last 30s                */
    float movMean_power_60s;          /*!< power moving mean over the last 60s                */
    float movMean_power_config;       /*!< power moving mean over the last configured time    */
    uint32_t timestamp;                 /*!< timestamp of database entry                        */
    uint32_t previous_timestamp;        /*!< timestamp of last database entry                   */
} DATA_BLOCK_MOVING_MEAN_s;

/**
 * data block struct of contactor feedback
 */
typedef struct {
    uint16_t contactor_feedback;                     /*!< feedback of contactors, without interlock */
    uint32_t timestamp;                              /*!< timestamp of database entry       */
    uint32_t previous_timestamp;                     /*!< timestamp of last database entry  */
} DATA_BLOCK_CONTFEEDBACK_s;

/**
 * data block struct of interlock feedback
 */
typedef struct {
    uint8_t interlock_feedback;                     /*!< feedback of interlock, without contactors */
    uint32_t timestamp;                              /*!< timestamp of database entry       */
    uint32_t previous_timestamp;                     /*!< timestamp of last database entry  */
} DATA_BLOCK_ILCKFEEDBACK_s;

/**
 * data block struct of system state
 */
typedef struct {
    uint8_t bms_state;                             /*!< system state (e.g., standby, normal) */
    uint32_t timestamp;                              /*!< timestamp of database entry       */
    uint32_t previous_timestamp;                     /*!< timestamp of last database entry  */
} DATA_BLOCK_SYSTEMSTATE_s;

/*================== Constant and Variable Definitions ====================*/

/**
 * @brief device configuration of database
 *
 * all attributes of device configuration are listed here (pointer to channel list, number of channels)
 */
extern DATA_BASE_HEADER_DEV_s data_base_dev;

/*================== Function Prototypes ==================================*/

/*================== Function Implementations =============================*/

#endif /* DATABASE_CFG_H_ */
