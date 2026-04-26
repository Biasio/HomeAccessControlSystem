#include "sensors.h"
#include "timers.h"
#include "vl53l0x.h"

volatile uint8_t ToF_flag = 0;


/*
    The ToF will rely solely on the GPIO interrupt,
    and the threshold for the sensor is setup via I2C read onto peripheral's registers.
    A driver for the sensor is included under 'src/external_src/drivers/'
*/

void _ToFInit(){

    i2c_init();

    bool success = vl53l0x_init();

}


void ToF_disable(){
    xshut_toggle(false);
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
}


void ToF_enable(){
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
}


void ToF_IRQHandler(void)
{
    ToF_disable();
    ToF_flag = 1;
}


/*
void _RFIDInit(){
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}
*/
