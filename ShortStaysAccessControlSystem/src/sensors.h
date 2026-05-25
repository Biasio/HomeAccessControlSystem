#ifndef SENSOR_ToF_H
#define SENSOR_ToF_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "vl53l0x.h"
#include "timers.h"


extern volatile uint8_t ToF_flag;
extern volatile bool ToF_ready; // 1=yes 0=no

void ToF_Init(void);
void ToF_IRQHandler(void);
bool ToF_disable(void);
bool ToF_enable(void);
bool ToF_validate_interrupt(void);



/* *the display uses SPI on UCB0
 * *the esp uses the UCA2 with UART
 * *the VL53L0X uses the UCB1 with I2C
 * *the RFID uses the UCB2 with SPI
 * */

extern volatile bool RFID_ready; // 1=yes 0=no

#define SPI_TIMEOUT 100
#define RFID_UID_LENGTH  7          // length of the RFID UID
extern const uint8_t RFID_saved[RFID_UID_LENGTH];


void RFID_CS_Low(void);
void RFID_CS_High(void);
bool RFID_Init(void);                    // Configure SPI pins and module (module disabled initially)
bool RFID_Enable(void);                  // Enable SPI, HW init MFRC522 (soft reset, timer, antenna)
bool RFID_Disable(void);                 // Power down MFRC522, disable SPI module
bool RFID_ReadTag(uint8_t *uid, uint8_t *uidLength); // Polling single tag read (REQA -> anticollision)


#define RFID_SCK_PORT         GPIO_PORT_P3
#define RFID_SCK_PIN          GPIO_PIN5
#define RFID_MOSI_PORT        GPIO_PORT_P3
#define RFID_MOSI_PIN         GPIO_PIN6
#define RFID_MISO_PORT        GPIO_PORT_P3
#define RFID_MISO_PIN         GPIO_PIN7

/* CS active low */
#define RFID_CS_PORT          GPIO_PORT_P5
#define RFID_CS_PIN           GPIO_PIN2

/* If pulled low, hard power down triggers, when edge rising, reset is forced*/
#define RFID_RST_PORT         GPIO_PORT_P3
#define RFID_RST_PIN          GPIO_PIN0

#define RFID_EUSCI            EUSCI_B2_BASE
#define RFID_EUSCI_TX_INT     EUSCI_B_SPI_TRANSMIT_INTERRUPT
#define RFID_EUSCI_RX_INT     EUSCI_B_SPI_RECEIVE_INTERRUPT

// --- MFRC522 USED REGISTER ADDRESSES ---
#define MFRC522_COMMAND_REG     0x01
#define MFRC522_COM_I_EN_REG    0x02
#define MFRC522_FIFO_DATA_REG   0x09
#define MFRC522_FIFO_LEVEL_REG  0x0A
#define MFRC522_CONTROL_REG     0x0C
#define MFRC522_BIT_FRAMING_REG 0x0D
#define MFRC522_MODE_REG        0x11
#define MFRC522_TX_CONTROL_REG  0x14
#define MFRC522_TX_ASK_REG      0x15
#define MFRC522_RF_CFG_REG      0x26
#define MFRC522_T_MODE_REG      0x2A
#define MFRC522_T_PRESCALER_REG 0x2B
#define MFRC522_T_RELOAD_H_REG  0x2C
#define MFRC522_T_RELOAD_L_REG  0x2D
#define MFRC522_VERSION_REG     0x37

// --- MFRC522 COMMANDS ---
#define MFRC522_CMD_IDLE        0x00
#define MFRC522_CMD_SOFT_RESET  0x0F
#define MFRC522_CMD_TRANSCEIVE  0x0C
#define MFRC522_CMD_MF_AUTHENT  0x0E

#endif
