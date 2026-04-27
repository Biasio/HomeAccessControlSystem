#ifndef SENSOR_ToF_H
#define SENSOR_ToF_H

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "vl53l0x.h"
#include "timers.h"


extern volatile uint8_t ToF_flag;


bool ToF_Init(void);

void ToF_IRQHandler(void);

bool ToF_disable(void);

bool ToF_enable(void);

bool ToF_validate_interrupt(void);


#endif
