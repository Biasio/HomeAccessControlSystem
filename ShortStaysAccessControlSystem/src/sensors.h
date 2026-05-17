#ifndef SENSOR_ToF_H
#define SENSOR_ToF_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "vl53l0x.h"
#include "timers.h"

////////////////// VL53L0X /////////////////////////
extern volatile uint8_t ToF_flag;
extern volatile bool ToF_ready; // 1=yes 0=no

void ToF_Init(void);
void ToF_IRQHandler(void);
bool ToF_disable(void);
bool ToF_enable(void);
bool ToF_validate_interrupt(void);


//////////////////// RFID /////////////////////////

/* *the display uses SPI on UCB0
 * *the esp uses the UCA2 with UART
 * *the VL53L0X uses the UCB1 with I2C
 * *the RFID uses the UCB2 with SPI
 * */

extern volatile bool RFID_ready; // 1=yes 0=no

#define SPI_TIMEOUT 100
#define RFID_UID_LENGTH  4          // length of the RFID UID
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

#define RFID_EUSCI_TX_INT     EUSCI_B_SPI_TRANSMIT_INTERRUPT
#define RFID_EUSCI_RX_INT     EUSCI_B_SPI_RECEIVE_INTERRUPT

// --- MFRC522 USED REGISTER ADDRESSES ---
    //starts and stops command execution
    #define MFRC522_COMMAND_REG     0x01
    //enable and disable interrupt request control bits
    #define MFRC522_COM_I_EN_REG    0x02

    //input and output of 64 byte FIFO buffer
    #define MFRC522_FIFO_DATA_REG   0x09
    //number of bytes stored in the FIFO buffer
    #define MFRC522_FIFO_LEVEL_REG  0x0A

    //miscellaneous control registers
    #define MFRC522_CONTROL_REG     0x0C
    //adjustments for bit-oriented frames
    #define MFRC522_BIT_FRAMING_REG 0x0D
    //defines general modes for transmitting and receiving
    #define MFRC522_MODE_REG        0x11

    //controls the logical behavior of the antenna driver pins TX1 and TX2
    #define MFRC522_TX_CONTROL_REG  0x14
    //selects the internal sources for the antenna driver
    #define MFRC522_TX_ASK_REG      0x15
    //configures the receiver gain
    #define MFRC522_RF_CFG_REG      0x26

    //defines settings for the internal timer
    #define MFRC522_T_MODE_REG      0x2A
    #define MFRC522_T_PRESCALER_REG 0x2B
    //defines the 16-bit timer reload value
    #define MFRC522_T_RELOAD_H_REG  0x2C
    #define MFRC522_T_RELOAD_L_REG  0x2D

    //shows the software version
    #define MFRC522_VERSION_REG     0x37

    //defines the width of the Miller modulation as multiples of the carrier
    //frequency (ModWidth + 1 / fclk)
    // the maximum value is half the bit period
    #define MFRC522_MOD_WIDTH_REG       0x24

    //Interrupt request bits.
    #define MFRC522_COM_IRQ_REG         0x04

// --- MFRC522 COMMANDS ---
    //no action, cancels current command execution
    #define MFRC522_CMD_IDLE        0x00
    //resets the MFRC522
    #define MFRC522_CMD_SOFT_RESET  0x0F
    //transmits data from the FIFO buffer
    #define MFRC522_CMD_TRANSMIT    0X04
    //transmits data from FIFO buffer to antenna and automatically
    // activates the receiver after transmission
    #define MFRC522_CMD_TRANSCEIVE  0x0C
    //activates the receiver circuits
    #define MFRC522_CMD_RECEIVE     0x08
    //performs the MIFARE standard authentication as a reader
    #define MFRC522_CMD_MF_AUTHENT  0x0E

#endif
