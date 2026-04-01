#include "msp.h"
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// --- CONFIGURAZIONE ---
#define ADMIN_PIN "9999"
#define UART_BUFFER_SIZE 64

// --- VARIABILI GLOBALI ---
char uartBuffer[UART_BUFFER_SIZE];
volatile uint8_t uartBufferIndex = 0;
volatile bool newUartMessage = false;

// --- FUNZIONE PER INVIARE STRINGHE ---
void sendString(const char* msg) {
    while(*msg != '\0') {
        UART_transmitData(EUSCI_A2_BASE, *msg);
        msg++;
    }
    // L'ESP32 si aspetta \r\n alla fine del messaggio
    UART_transmitData(EUSCI_A2_BASE, '\r');
    UART_transmitData(EUSCI_A2_BASE, '\n');
}

int main(void)
{
    // 1. Ferma il Watchdog
    WDT_A_holdTimer();

    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // 4. Configura i pin UART P3.2 (RX) e P3.3 (TX)
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    // 5. Configura e avvia l'UART a 115200 Baud (con SMCLK a 12MHz)
    const eUSCI_UART_ConfigV1 uartConfig = {
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,
        6, 8, 0x20, // 115200 baud
        EUSCI_A_UART_NO_PARITY,
        EUSCI_A_UART_LSB_FIRST,
        EUSCI_A_UART_ONE_STOP_BIT,
        EUSCI_A_UART_MODE,
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,
        EUSCI_A_UART_8_BIT_LEN
    };
    UART_initModule(EUSCI_A2_BASE, &uartConfig);
    UART_enableModule(EUSCI_A2_BASE);

    // 6. Abilita Interrupt per l'UART RX
    UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    Interrupt_enableInterrupt(INT_EUSCIA2);
    Interrupt_enableMaster();

    // Invio un messaggio di "Ciao" iniziale all'ESP32 per testare il TX
    sendString("BATMAN_99");

    // 7. Loop Principale
    while(1)
    {
        if (newUartMessage) {
            newUartMessage = false; // Abbasso la bandierina

            const char* cmdVerify = "VERIFY_PIN:";

            // Se il messaggio inizia con "VERIFY_PIN:"
            if(strncmp(uartBuffer, cmdVerify, strlen(cmdVerify)) == 0) {

                // Estraggo la parte utile "9999:1234567"
                const char* payload = uartBuffer + strlen(cmdVerify);

                // Cerco i due punti che separano il PIN dalla ChatID
                const char* separator = strchr(payload, ':');

                if (separator != NULL) {
                    char pin[10] = {0};
                    size_t pinLen = separator - payload;

                    if(pinLen < sizeof(pin)) {
                        strncpy(pin, payload, pinLen);
                        pin[pinLen] = '\0';

                        const char* chatId = separator + 1;
                        char response[64];

                        // Controllo se il PIN è quello giusto
                        if (strcmp(pin, ADMIN_PIN) == 0) {
                            snprintf(response, sizeof(response), "PIN_OK:%s", chatId);
                        } else {
                            snprintf(response, sizeof(response), "PIN_WRONG:%s", chatId);
                        }

                        // Rispondo all'ESP32
                        sendString(response);
                    }
                }
            }

            // Pulisco il buffer per il prossimo messaggio
            uartBuffer[0] = '\0';
        }
    }
}

// --- GESTORE INTERRUPT UART ---
void EUSCIA2_IRQHandler(void) {
    uint32_t status = UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT) {
        char rxChar = UART_receiveData(EUSCI_A2_BASE);

        if(rxChar != '\n' && rxChar != '\r') {
            if(uartBufferIndex < (UART_BUFFER_SIZE - 1)) {
                uartBuffer[uartBufferIndex++] = rxChar;
            }
        }
        else {
            if(uartBufferIndex > 0) {
                uartBuffer[uartBufferIndex] = '\0';
                newUartMessage = true;
                uartBufferIndex = 0;
            }
        }
    }
    UART_clearInterruptFlag(EUSCI_A2_BASE, status);
}
