#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include "timers.h"
#include "irqHandlers.h"




void _timerInit();
void _adcInit();

const uint16_t* get_results_buffer(void);
bool data_aquired(void);

#endif
