#ifndef IRQ_HANDLERS_H
#define IRQ_HANDLERS_H

#include <stdint.h>

#include "sensors.h"
#include "push_button.h"
#include "fsm.h"
#include "timers.h"

extern volatile uint8_t standby;
extern volatile bool move_rectangle;
extern volatile uint32_t system_millis;

extern uint16_t *resultsBuffer_ptr;

void SysTick_Handler(void);
void PORT5_IRQHandler(void);
void PORT3_IRQHandler(void);

void TA1_0_IRQHandler(void);
void TA2_0_IRQHandler(void);
void TA3_N_IRQHandler(void);
void ADC14_IRQHandler(void);

#endif
