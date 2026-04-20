#ifndef SENSOR_PIR_H
#define SENSOR_PIR_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include "vl53l0x.h"

extern volatile uint8_t PIR_flag;




void _PIRInit();


void PIR_disable();
void PIR_enable();
void PIR_change_addr(uint8_t new_addr);

void PIR_IRQHandler(void);


void I2C_write_reg8(uint8_t reg_addr, uint8_t data);
void I2C_write_reg16(uint8_t reg_addr, uint16_t data);


#define VL53L0X_ADDR 0x29

#define SYSRANGE_START                              0x00
#define SYSTEM_SEQUENCE_CONFIG                      0x01
#define SYSTEM_INTERMEASUREMENT_PERIOD              0x04
#define SYSTEM_RANGE_CONFIG                         0x09
#define SYSTEM_INTERRUPT_CONFIG_GPIO                0x0A
#define SYSTEM_INTERRUPT_CLEAR                      0x0B
#define SYSTEM_THRESH_HIGH                          0x0C
#define SYSTEM_THRESH_LOW                           0x0E
#define RESULT_INTERRUPT_STATUS                     0x13
#define RESULT_RANGE_STATUS                         0x14
#define CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       0x20
#define PRE_RANGE_CONFIG_MIN_SNR                    0x27
#define ALGO_PART_TO_PART_RANGE_OFFSET_MM           0x28
#define ALGO_PHASECAL_LIM                           0x30
#define ALGO_PHASECAL_CONFIG_TIMEOUT                0x30
#define GLOBAL_CONFIG_VCSEL_WIDTH                   0x32
#define HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       0x33
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define MSRC_CONFIG_TIMEOUT_MACROP                  0x46
#define FINAL_RANGE_CONFIG_VALID_PHASE_LOW          0x47
#define FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         0x48
#define PRE_RANGE_CONFIG_VCSEL_PERIOD               0x50
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x51
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          0x52
#define HISTOGRAM_CONFIG_READOUT_CTRL               0x55
#define PRE_RANGE_CONFIG_VALID_PHASE_LOW            0x56
#define PRE_RANGE_CONFIG_VALID_PHASE_HIGH           0x57
#define MSRC_CONFIG_CONTROL                         0x60
#define PRE_RANGE_CONFIG_SIGMA_THRESH_HI            0x61
#define PRE_RANGE_CONFIG_SIGMA_THRESH_LO            0x62
#define PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          0x64
#define FINAL_RANGE_CONFIG_MIN_SNR                  0x67
#define FINAL_RANGE_CONFIG_VCSEL_PERIOD             0x70
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        0x71
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        0x72
#define POWER_MANAGEMENT_GO1_POWER_FORCE            0x80
#define SYSTEM_HISTOGRAM_BIN                        0x81
#define GPIO_HV_MUX_ACTIVE_HIGH                     0x84
#define I2C_MODE                                    0x88
#define I2C_SLAVE_DEVICE_ADDRESS                    0x8A
#define VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           0x89
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_0            0xB0
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_1            0xB1
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_2            0xB2
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_3            0xB3
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_4            0xB4
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_5            0xB5
#define GLOBAL_CONFIG_REF_EN_START_SELECT           0xB6
#define RESULT_PEAK_SIGNAL_RATE_REF                 0xB6
#define RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       0xBC
#define RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        0xC0
#define IDENTIFICATION_MODEL_ID                     0xC0
#define IDENTIFICATION_REVISION_ID                  0xC2
#define RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       0xD0
#define RESULT_CORE_RANGING_TOTAL_EVENTS_REF        0xD4
#define OSC_CALIBRATE_VAL                           0xF8
#define SOFT_RESET_GO2_SOFT_RESET_N                 0xBF
#define DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         0x4E
#define DYNAMIC_SPAD_REF_EN_START_OFFSET            0x4F

#endif
