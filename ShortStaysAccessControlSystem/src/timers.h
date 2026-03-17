#ifndef TIMERS_H
#define TIMERS_H


#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "irqHandlers.h"


#define TIMER_RESTART(x,y) Timer_A_stop(x); Timer_A_clearTimer(x); Timer_A_startCounter(x, y)

void _idleTimerInit();

void _debounceTimerInit();

void _ADCtimerInit();

void _PWMtimerInit();

#endif
