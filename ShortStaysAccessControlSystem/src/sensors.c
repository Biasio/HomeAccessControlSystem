#include "sensors.h"


volatile uint8_t PIR_flag = 0;


void _PIRInit(){
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN0);
    GPIO_interruptEdgeSelect(GPIO_PORT_P3, GPIO_PIN0, GPIO_HIGH_TO_LOW_TRANSITION);

    //enable interrupts on buttons
    GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN0);

    Interrupt_enableInterrupt(INT_PORT3);
    PIR_flag = 0;
}


void PIR_IRQHandler(void)
{
    GPIO_clearInterruptFlag(GPIO_PORT_P3, GPIO_PIN0);
    PIR_flag = 1;
}

void _RFIDInit(){
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}

