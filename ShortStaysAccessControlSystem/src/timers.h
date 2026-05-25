#ifndef TIMERS_H
#define TIMERS_H


#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "irqHandlers.h"


#define TIMER_RESTART(x,y) Timer_A_stop(x); Timer_A_clearTimer(x); Timer_A_startCounter(x, y)
#define TIMER_CLEAR_STOP(x) Timer_A_stop(x); Timer_A_clearTimer(x)
#define TIMER_CLEAR_START(x,y) Timer_A_clearTimer(x); Timer_A_startCounter(x, y)

void _ClockSystemInit();

void _idleTimerInit();

void _debounceTimerInit();

void _ADCtimerInit();

void _PWMtimerInit();

void _SysTickInit();

void delay_ms(uint32_t msec);

#endif
