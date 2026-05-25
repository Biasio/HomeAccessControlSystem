#ifndef PUSH_BUTTON_H
#define PUSH_BUTTON_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include "sensors.h"
#include "timers.h"

//variable used to see if the button is already pressed
extern volatile uint8_t buttonA_pressed;
extern volatile uint8_t buttonB_pressed;

void _pushButtonsInit();

void ButtonA_IRQHandler(void);
void ButtonB_IRQHandler(void);

#endif
