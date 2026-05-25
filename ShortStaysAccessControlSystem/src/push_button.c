#include "push_button.h"

volatile uint8_t buttonA_pressed = 0;
volatile uint8_t buttonB_pressed = 0;

int timer_int_counter_button = 0;


void _pushButtonsInit(){
        //Set pushBUTTONS
        //S1 = 4.33 = P5.1
        //S2 = 4.32 = P3.5

        GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN1);
        GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN5);

        GPIO_interruptEdgeSelect(GPIO_PORT_P5, GPIO_PIN1, GPIO_HIGH_TO_LOW_TRANSITION);
        GPIO_interruptEdgeSelect(GPIO_PORT_P3, GPIO_PIN5, GPIO_HIGH_TO_LOW_TRANSITION);

        //enable interrupts on buttons
        GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN1);
        GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN5);

        Interrupt_enableInterrupt(INT_PORT5);
        Interrupt_enableInterrupt(INT_PORT3);

        Interrupt_enableMaster(); //there must be just one of this instruction

        _debounceTimerInit();
}


//S1 = FORWARD BUTTON
void ButtonA_IRQHandler(void)
{
    //printf("Button pressed - Debounce started\n");

    // Disable interrupts of PORT5 until timer has passed
    GPIO_disableInterrupt(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN1);


    TIMER_RESTART(TIMER_A1_BASE, TIMER_A_UP_MODE);
}


//S2 = BACKWARD BUTTON
void ButtonB_IRQHandler(void)
{
    GPIO_disableInterrupt(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN5);

    TIMER_RESTART(TIMER_A1_BASE, TIMER_A_UP_MODE);
}
