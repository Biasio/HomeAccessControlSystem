#include "irqHandlers.h"


volatile uint8_t standby = 0;

//variable used to move the rectangle in the display after the joystick moved
volatile bool move_rectangle = 0;


void PORT5_IRQHandler(void)
{
    uint_fast16_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P5);

    if(status & GPIO_PIN1){
        ButtonA_IRQHandler();
        return;
    }

    GPIO_clearInterruptFlag(GPIO_PORT_P5, status);
}



void PORT3_IRQHandler(void)
{

    uint_fast16_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P3);

    if(status & GPIO_PIN5)
    {
        ButtonB_IRQHandler();
        return;
    }
    else if(status & GPIO_PIN0)
    {
        PIR_IRQHandler();
        return;
    }

    GPIO_clearInterruptFlag(GPIO_PORT_P3, status);

}


// Debouncer timer
void TA1_0_IRQHandler(void)
{
    // Stop the timer
    Timer_A_stop(TIMER_A1_BASE);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

    // Check state of pin
    if (GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN1) == GPIO_INPUT_PIN_LOW) {
        buttonA_pressed = 1; // Valid pressure
        TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE); //Restart idle timer
    }
    else if(GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN5) == GPIO_INPUT_PIN_LOW) {
        buttonB_pressed = 1; // Valid pressure
        TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE); //Restart idle timer
    }

    // Clear possible pending interrupts flags set during debounce period
    GPIO_clearInterruptFlag(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN5);

    // Enable interrupt for push buttons
    GPIO_enableInterrupt(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN5);
}


void TA2_0_IRQHandler(void)
{
    Timer_A_stop(TIMER_A2_BASE);
    Timer_A_clearTimer(TIMER_A2_BASE); // Reset counter at 0
    Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

    standby = 1;

    // Enable interrupt for PIR sensor
    PIR_flag = 0;
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN0);
}



// timer to make joystick updates
void TA3_N_IRQHandler(void)
{
    /* clear the timer pending interrupt flag */
    Timer_A_clearInterruptFlag(TIMER_A3_BASE);
    ADC14_toggleConversionTrigger();
}



void ADC14_IRQHandler(void)
{
    uint64_t status;

    status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if(status & ADC_INT1) //timer has finished, can update position on display
    {
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);
        move_rectangle=1;
        TIMER_RESTART(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE); //Retsrat idle timer
    }
}
