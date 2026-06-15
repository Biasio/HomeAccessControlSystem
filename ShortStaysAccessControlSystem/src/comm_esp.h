#ifndef ESP_COMM_H
#define ESP_COMM_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "flash.h"
#include "fsm_helpers.h" // External reference to the Admin PIN stored in fsm.c for Telegram login verification


// Include some libraries for random pin generation
#include "irqHandlers.h"
#include "joystick.h"

// --- CONSTANTS ---
#define MAX_TEMP_USERS      10          // Maximum number of simultaneous temporary users
#define UART_BUFFER_SIZE    64          // Maximum size of an incoming UART message
#define TIME_SYNC_INTERVAL_MS 7200000   // 2 hours in milliseconds
#define FLASH_MAGIC_NUMBER 0xABCD       // This number is used to check if data from flash have been restored properly

// --- USER DATA STRUCTURE ---
typedef struct {
    char chatId[20];    // Telegram ID
    char pin[5];        // 4-digit temporary PIN
    bool active;        // Flag to check if this array slot is occupied or free
    uint16_t magicNumber;       // Number used to check if data from flash are restored properly
} TempUser;

// --- SHARED GLOBAL VARIABLES ---
extern volatile bool pending_uart_command;       // Flag to notify a command received
extern char command_buffer[UART_BUFFER_SIZE];    // Command buffer
extern TempUser activeTempUsers[MAX_TEMP_USERS]; // Array containing all generated temporary users
extern volatile bool timeSynced;        // Flag to indicate whether the time has been synchronised
extern volatile uint32_t lastTimeSync;  // Timestamp of the last time synchronisation
TempUser *ptrUserArray;                 // Pointer used to save and restore data from flash

// --- FUNCTIONS  ---
void ESP_Comm_Init(void);               // Call in _hwInit() to configure P3.2/P3.3 pins and initialise UART at 115200 baud
void processUartMessage(char* command); //
void sendUartMessage(const char* msg);  // Helper function to send text responses back to the ESP32
void EUSCIA2_IRQHandler(void);          // UART Interrupt Service Routine (must also be declared in irqHandlers.h)
void PendSV_Handler(void);              // CMSIS standard Deferred Interrupt Handler

void requestRealTime(void);

#endif /* ESP_COMM_H */
