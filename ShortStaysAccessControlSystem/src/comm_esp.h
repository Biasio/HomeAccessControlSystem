#ifndef ESP_COMM_H
#define ESP_COMM_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- CONSTANTS ---
#define MAX_TEMP_USERS      10          // Maximum number of simultaneous temporary users
#define UART_BUFFER_SIZE    64          // Maximum size of an incoming UART message
#define TIME_SYNC_INTERVAL_MS 7200000   // 2 hours in milliseconds

// --- USER DATA STRUCTURE ---
typedef struct {
    char chatId[20];    // Telegram ID
    char pin[5];        // 4-digit temporary PIN
    bool active;        // Flag to check if this array slot is occupied or free
} TempUser;

// --- SHARED GLOBAL VARIABLES ---
extern volatile bool newUartMessage;    // This flag is set to 'true' by the UART Interrupt when a full message is received
extern TempUser activeTempUsers[MAX_TEMP_USERS];    // Array containing all generated temporary users
extern volatile bool timeSynced;        // Flag to indicate whether the time has been synchronized
extern volatile uint32_t lastTimeSync;  // Timestamp of the last time synchronization

// --- FUNCTIONS  ---
void ESP_Comm_Init(void);               // Call in _hwInit() to configure P3.2/P3.3 pins and initialize UART at 115200 baud
void processUartMessage(void);          // Call in FSM_Run() or check_for_inputs() when newUartMessage becomes true
void sendUartMessage(const char* msg);  // Helper function to send text responses back to the ESP32
void EUSCIA2_IRQHandler(void);          // UART Interrupt Service Routine (must also be declared in irqHandlers.h)

void requestRealTime(void);

#endif /* ESP_COMM_H */
