#include "sensors.h"


volatile uint8_t ToF_flag = 0;
volatile bool ToF_ready = 0;

/*
    The ToF will rely solely on the GPIO interrupt,
    and the threshold for the sensor is setup via I2C read onto peripheral's registers.
    A driver for the sensor is included under 'src/external_src/drivers/'
*/

void ToF_Init(){
    /*
    i2c_init();
    interrupt_gpio_init();
    xshut_gpio_init();
    ToF_ready = vl53l0x_init();
    return;*/
}

void ToF_IRQHandler(void){
    /*
    ToF_flag = 1;
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    return;*/
}

bool ToF_disable(){
    /*
    uint16_t dummy=0;


    bool status = vl53l0x_read_range_interrupt(&dummy);
    status &= vl53l0x_stop_continuous(); //need to disable continuous mode

    ToF_ready=0;
    xshut_toggle(false); //sensor to standby

    return status;*/
}



bool ToF_enable(){
    /*
    bool status = vl53l0x_init();

    status &=  vl53l0x_start_continuous();

    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN6);

    return status;*/
}


bool ToF_validate_interrupt(void)
{
    /*
    bool status = 0;
    uint16_t range=0;
    if (ToF_flag == 1){
        status &= vl53l0x_read_range_interrupt(&range);
        status &= ToF_disable();
    }
    return status;*/
}


/*
void _RFIDInit(){
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}
*/
