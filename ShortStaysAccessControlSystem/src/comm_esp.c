#include "comm_esp.h"


// --- GLOBAL VARIABLES (Declared extern in the header) ---
TempUser activeTempUsers[MAX_TEMP_USERS];
volatile bool timeSynced = false;
volatile uint32_t lastTimeSync = 0;

// --- LOCAL MODULE VARIABLES ---
char uartBuffer[UART_BUFFER_SIZE];
volatile uint8_t uartBufferIndex = 0;
char command_buffer[UART_BUFFER_SIZE];

// --- INITIALIZATION ---
void ESP_Comm_Init(void) {
    // Initialize users from flash
    ptrUserArray = (TempUser *) USER_ARRAY_START;
    memcpy(activeTempUsers, ptrUserArray, sizeof(activeTempUsers)); // memcpy() is used to copy data from flash into the RAM instance "activeTempUser"

    // Check if data restored from flash contains right value or no
    if(activeTempUsers[0].magicNumber != FLASH_MAGIC_NUMBER){   // if data restored from flash are WRONG (like the first time) i initialize them
        int i;
        for(i = 0; i < MAX_TEMP_USERS; i++) {
            // Initialize all temporary user slots as "free" and set the right magic number
            activeTempUsers[i].active = false;
            activeTempUsers[i].magicNumber = FLASH_MAGIC_NUMBER;
        }
       save_userArray();       //save on flash initialized array
    }

    // FIX: Configure PendSV priority to the lowest possible (0xFF).
    // This ensures the heavy Flash operations don't block critical hardware interrupts like UART or SysTick.
    NVIC_SetPriority(PendSV_IRQn, 0xFF);

    // Configure pins P3.2 (RX) and P3.3 (TX) for UART function
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    // Configure eUSCI_A2 hardware (115200 baud with SMCLK at 12 MHz)
    const eUSCI_UART_ConfigV1 uartConfig = {
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,                 // SMCLK Clock Source
        6,                                              // clockPrescalar (BRDIV)
        8,                                              // firstModReg (UCxBRF)
        0x20,                                           // secondModReg (UCxBRS)
        EUSCI_A_UART_NO_PARITY,                         // No Parity
        EUSCI_A_UART_LSB_FIRST,                         // LSB First
        EUSCI_A_UART_ONE_STOP_BIT,                      // One stop bit
        EUSCI_A_UART_MODE,                              // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,  // Oversampling enabled
        EUSCI_A_UART_8_BIT_LEN                          // Data length: 8 bits
    };
    UART_initModule(EUSCI_A2_BASE, &uartConfig);
    UART_enableModule(EUSCI_A2_BASE);
    // Enable Interrupt only for RECEPTION
    UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    Interrupt_enableInterrupt(INT_EUSCIA2);
    delay_ms(200);
}

// --- HIGH PRIORITY INTERRUPT HANDLING ---
void EUSCIA2_IRQHandler(void) {
    uint32_t status = UART_getEnabledInterruptStatus(EUSCI_A2_BASE);    // Get current interrupt status

    // Check if the receive interrupt triggered
    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT) {
        // Read the incoming character from the hardware buffer
        char rxChar = UART_receiveData(EUSCI_A2_BASE);
        // If a normal character arrives...
        if(rxChar != '\n' && rxChar != '\r') {
            // Save the character in the buffer if there is space
            if(uartBufferIndex < (UART_BUFFER_SIZE - 1)) {
                uartBuffer[uartBufferIndex++] = rxChar;
            }
        }
        // Otherwise, a newline arrived meaning the message is complete
        else if (rxChar =='\n') {
            // Ignore multiple or empty newlines
            if(uartBufferIndex > 0) {
                uartBuffer[uartBufferIndex] = '\0'; // Add string terminator
                strncpy(command_buffer, uartBuffer, UART_BUFFER_SIZE); // Copy payload to transit buffer
                uartBufferIndex = 0;    // Reset index for the next message

                // 2. FIX: Trigger PendSV (Deferred Interrupt Processing).
                // This sets the PendSV flag. The ARM core will automatically jump to PendSV_Handler asynchronously, preempting delay_ms() or while(1) loops.
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
            }
        }
    }
    UART_clearInterruptFlag(EUSCI_A2_BASE, status);// Clear the interrupt flag
}

// --- DEFERRED INTERRUPT HANDLER (LOWEST PRIORITY) ---
void PendSV_Handler(void) {
    // This handler automatically interrupts the main execution flow safely, without requiring any polling mechanism in the code.
    processUartMessage(command_buffer);
}


// --- SEND RESPONSES TO ESP32 ---
void sendUartMessage(const char* msg) {
    // Send each character of the string sequentially
    while(*msg != '\0') {
        UART_transmitData(EUSCI_A2_BASE, *msg);
        msg++;
    }
    // Add "\r\n" at the end (as expected by the ESP32 parser)
    UART_transmitData(EUSCI_A2_BASE, '\r');
    UART_transmitData(EUSCI_A2_BASE, '\n');
}

// --- HANDLERS FOR MESSAGES ---
static void handleGenTempPin(const char* payload) {
    if(payload == NULL || *payload == '\0') return; //No chatId present
    // Find the first free slot
    int slot = -1;
    int i;
    // FIX: Before searching for an empty slot, check if the user already has one!
    // This prevents the same user from exhausting the 10 available slots on the MSP.
    for(i = 0; i < MAX_TEMP_USERS; i++) {
        if(activeTempUsers[i].active && strcmp(activeTempUsers[i].chatId, payload) == 0) {
            slot = i; // Reuse the existing slot for this user
            break;
        }
    }

    // If no existing slot was found, find the first free slot
    if (slot == -1) {
        for(i = 0; i < MAX_TEMP_USERS; i++) {
            if(!activeTempUsers[i].active) {
                slot = i;
                break;
            }
        }
    }

    // If a slot is found, generate PIN and save user
    if(slot != -1) {
        // Mark the slot as active
        activeTempUsers[slot].active = true;

        // Copy the chatId safely and force termination
        strncpy(activeTempUsers[slot].chatId, payload, sizeof(activeTempUsers[slot].chatId) - 1); // Copy the chatId
        activeTempUsers[slot].chatId[sizeof(activeTempUsers[slot].chatId) - 1] = '\0';  // Force termination for safety

        // Variables for random PIN generation and duplicate checking
        int randPin;
        bool isDuplicate;
        char newPinStr[5];
        char adminPinStr[5];

        // Get the current admin PIN to ensure the generated temp PIN does not duplicate it
        snprintf(adminPinStr, sizeof(adminPinStr), "%d%d%d%d",
                 saved_pin_admin[0], saved_pin_admin[1],
                 saved_pin_admin[2], saved_pin_admin[3]);

        // Seed the random number generator with a combination of the current time and some noise from the ADC to improve randomness
        const uint16_t* adc_noise = get_results_buffer();
        srand(system_millis ^ adc_noise[0] ^ adc_noise[1] ^ rand());

        // Generate a random PIN and check for duplicates with...
        do {
            isDuplicate = false;

            // Generate a random 4-digit
            randPin = (rand() % 9000) + 1000;
            snprintf(newPinStr, sizeof(newPinStr), "%04d", randPin);

            // Check A: against the admin PIN
            if (strcmp(newPinStr, adminPinStr) == 0) {
                isDuplicate = true;
                continue; //
            }

            // Check B: against other active temporary user PINs
            int j;
            for (j = 0; j < MAX_TEMP_USERS; j++) {
                // We check only the active temp users and we skip the current slot since it's not fully initialized yet
                if (j != slot && activeTempUsers[j].active && strcmp(newPinStr, activeTempUsers[j].pin) == 0) {
                    isDuplicate = true;
                    break; // If a duplicate is found, we break the loop and generate a new PIN
                }
            }
        } while (isDuplicate);

        // Save the generated PIN in the activeTempUsers array
        strncpy(activeTempUsers[slot].pin, newPinStr, sizeof(activeTempUsers[slot].pin));

        // Save the activeTempUsers array in flash memory
        save_userArray();

        // Send response to ESP32
        char response[64];
        snprintf(response, sizeof(response), "TEMP_PIN:%s:%s", activeTempUsers[slot].chatId, activeTempUsers[slot].pin);
        sendUartMessage(response);
    }
}

static void handleVerifyPin(const char* payload) {
    // Check if the payload is not empty
    if(payload == NULL || *payload == '\0') return;

    // The payload here is in the format "pin:chatId". Now we separate them

    // Find the memory address of the first colon ':' character
    const char* separator = strchr(payload, ':');

    // If no colon is found, the format is invalid, so we exit safely
    if(separator == NULL) return;

    // Extract the PIN safely
    char pin[5];
    size_t pinLen = separator - payload;
    if(pinLen >= sizeof(pin)) {
        pinLen = sizeof(pin) - 1; //Truncate if too long
    }
    strncpy(pin, payload, pinLen);
    pin[pinLen] = '\0'; // Terminate string

    // Extract the chatId, that start after the separator
    const char* chatId = separator + 1;
    if(*chatId == '\0') return;

    // Check the Admin PIN
    bool found = false;

    // Convert the saved admin pin from the system into a string for comparison
    char adminPinStr[5];
    snprintf(adminPinStr, sizeof(adminPinStr), "%d%d%d%d", saved_pin_admin[0], saved_pin_admin[1], saved_pin_admin[2], saved_pin_admin[3]);

    // Now compare the extracted PIN with the saved admin PIN
    if(strcmp(pin, adminPinStr) == 0) {
        found = true;
    }

    // Send result to ESP32
    char response[64];
    if(found) {
        snprintf(response, sizeof(response), "PIN_OK:%s", chatId);
    } else {
        snprintf(response, sizeof(response), "PIN_WRONG:%s", chatId);
    }
    sendUartMessage(response);
}

static void handleRevokePin(const char* payload) {
    // Check if the payload is not empty
    if(payload == NULL || *payload == '\0') return;

    bool arrayChanged = false; // Flag to track if we need to save changes to flash after processing the command

    // Find the user with the matching chat ID and mark their temporary pin as inactive
    int i;
    for(i = 0; i < MAX_TEMP_USERS; i++) {

      // Compare the chat ID in the payload with the chat ID of the active temporary users
      if(activeTempUsers[i].active && strcmp(activeTempUsers[i].chatId, payload) == 0) {

          // User found, mark it as inactive
          activeTempUsers[i].active = false;
          arrayChanged = true; // Set flag to save changes in flash after processing the command
      }
    }

    // FIX: Save the updated array to flash to ensure the revocation survives a reboot
    if (arrayChanged) {
        save_userArray(); // Save the updated activeTempUsers array in flash memory if any change was made
    }

    // Send an ACK back to the ESP32 to confirm that the revoke command has been processed
    char response[64];
    snprintf(response, sizeof(response), "REVOKE_OK:%s", payload);
    sendUartMessage(response);
}

static void handleTimeSync(const char* payload) {
    // Check if the payload is not empty
    if(payload == NULL || *payload == '\0') return;

    // Extract Time and Date (HH:MM:SS:DD:MM:YYYY:DOW) from the payload string
    int hour = 0, minute = 0, second = 0, day = 1, month = 1, year = 2026, dow = 0;
    sscanf(payload, "%d:%d:%d:%d:%d:%d:%d", &hour, &minute, &second, &day, &month, &year, &dow);

    // Configure the MSP432 hardware Real-Time Clock (RTC_C)
    RTC_C_Calendar currentTime = {0};
    currentTime.seconds = second;
    currentTime.minutes = minute;
    currentTime.hours = hour;

    // Real date fields, perfectly synced for your teammate's logger!
    currentTime.dayOfWeek = dow;
    currentTime.dayOfmonth = day;
    currentTime.month = month;
    currentTime.year = year;

    // Initialize and start the hardware clock
    RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BINARY);
    RTC_C_startClock();

    // Update synchronization flags
    timeSynced = true;
    lastTimeSync = system_millis; // Record the timestamp of this sync for anti-drift tracking
}

// --- COMMAND DISPATCHER FOR RECEIVED MESSAGE ---
void processUartMessage(char* command) {
    //Define command prefixes as constant strings
    const char* cmdGen = "GEN_PIN:";
    const char* cmdVerify = "VERIFY_PIN:";
    const char* cmdRevoke = "REVOKE_PIN:";
    const char* cmdTimeSync = "TIME_SYNC:";

    //Check prefixes to identify which command was sent
    if(strncmp(command, cmdGen, strlen(cmdGen)) == 0) {
        handleGenTempPin(command + strlen(cmdGen));  //Pass the payload using pointer arithmetic based on the exact prefix length
    }
    else if(strncmp(command, cmdVerify, strlen(cmdVerify)) == 0) {
        handleVerifyPin(command + strlen(cmdVerify));
    }
    else if(strncmp(command, cmdRevoke, strlen(cmdRevoke)) == 0) {
        handleRevokePin(command + strlen(cmdRevoke));
    }
    else if(strncmp(command, cmdTimeSync, strlen(cmdTimeSync)) == 0) {
        handleTimeSync(command + strlen(cmdTimeSync));
    }
}

// --- GET REAL TIME CLOCK FROM ESP32 ---
void requestRealTime(void) {
    // Mandiamo "0" come finto ChatID per rispettare il parser dell'ESP32
    sendUartMessage("REQ_TIME:0");
}

