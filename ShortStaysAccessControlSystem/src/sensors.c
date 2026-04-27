#include "sensors.h"


volatile uint8_t ToF_flag = 0;


/*
    The ToF will rely solely on the GPIO interrupt,
    and the threshold for the sensor is setup via I2C read onto peripheral's registers.
    A driver for the sensor is included under 'src/external_src/drivers/'
*/

bool ToF_Init(){

    i2c_init();
    xshut_gpio_init();
    return vl53l0x_init();
}

void ToF_IRQHandler(void){
    ToF_flag = 1;
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    return;
}

bool ToF_disable(){
    bool status = vl53l0x_stop_continuous(); //need to disable continuous mode

    xshut_toggle(false); //sensor to standby

    return status;
}



bool ToF_enable(){
    bool status = vl53l0x_init();

    status =  vl53l0x_start_continuous();

    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN6);

    return status;
}


bool ToF_validate_interrupt(void)
{
    uint16_t range=0;
    bool status = vl53l0x_read_range_interrupt(&range);
    status = ToF_disable();

    return status;
}


/*
void _RFIDInit(){
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}
*/
