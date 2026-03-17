#ifndef SENSOR_PIR_H
#define SENSOR_PIR_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>

extern volatile uint8_t PIR_flag;

void _PIRInit();

void PIR_IRQHandler(void);

#endif
