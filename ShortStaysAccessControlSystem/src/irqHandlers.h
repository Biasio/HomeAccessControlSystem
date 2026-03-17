#ifndef IRQ_HANDLERS_H
#define IRQ_HANDLERS_H

#include "push_button.h"
#include "sensorPIR.h"
#include "fsm.h"
#include <stdint.h>


extern volatile uint8_t standby;
extern volatile bool move_rectangle;

void PORT5_IRQHandler(void);
void PORT3_IRQHandler(void);

#endif
