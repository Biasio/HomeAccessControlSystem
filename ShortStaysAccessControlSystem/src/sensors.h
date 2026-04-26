#ifndef SENSOR_ToF_H
#define SENSOR_ToF_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include "vl53l0x.h"

extern volatile uint8_t ToF_flag;




void _ToFInit(void);

void ToF_disable(void);

void ToF_enable(void);

void ToF_IRQHandler(void);


#endif
