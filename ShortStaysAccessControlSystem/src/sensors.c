#include "sensors.h"
#include "timers.h"


volatile uint8_t PIR_flag = 0;

/*
    The PIR will rely solely on the GPIO interrupt,
    and the threshold for the sensor is setup via I2C read onto peripheral's registers
*/

void _PIRInit(){

    /* Select I2C function for I2C_SCL(P6.5) & I2C_SDA(P6.4) */
    GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P6,
            GPIO_PIN5 | GPIO_PIN4,
            GPIO_PRIMARY_MODULE_FUNCTION);


    // I2C config for the presence sensor
    eUSCI_I2C_MasterConfig i2cConfig = {
        EUSCI_B_I2C_CLOCKSOURCE_SMCLK, // Use System Micro Controller Clock
        0,                       // SMCLK speed
        EUSCI_B_I2C_SET_DATA_RATE_100KBPS, // I2C speed
        0,                             // No byte counter threshold
        EUSCI_B_I2C_NO_AUTO_STOP       // Manual stop
    };

    i2cConfig.i2cClk = CS_getSMCLK();

    I2C_initMaster(EUSCI_B1_BASE, &i2cConfig);

    // Enable I2C Module to start operations
    I2C_enableModule(EUSCI_B1_BASE);

    PIR_change_addr(VL53L0X_ADDR);



    //registers config

    I2C_write_reg8(I2C_MODE, 0x00);   // Default I2C mode

    I2C_write_reg8(0xFF, 0x01); // sensor tuning param
    I2C_write_reg8(SYSRANGE_START, 0x00);   // SYSRANGE_START in continous mode

    I2C_write_reg16(SYSTEM_THRESH_LOW, 0x0064); //low thresh in mm = 100
    I2C_write_reg16(SYSTEM_THRESH_HIGH, 0xFFFF);

    // Configure GPIO as out‑of‑window interrupt, active when lower than THRESH_MIN
    I2C_write_reg8(SYSTEM_INTERRUPT_CONFIG_GPIO, 0x01);   // SYSTEM_INTERRUPT_GPIO_CONFIG
    I2C_write_reg8(GPIO_HV_MUX_ACTIVE_HIGH, 0x00); // set interrupt to be a negative signal

    // Clear any pending interrupt
    I2C_write_reg8(SYSTEM_INTERRUPT_CLEAR, 0x01);   // SYSTEM_INTERRUPT_CLEAR

    I2C_write_reg8(SYSRANGE_START, 0x01);   // SYSRANGE_START in continous mode
    I2C_write_reg8(0xFF, 0x00); // sensor tuning param
    I2C_write_reg8(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);

    // enable interrupt pin on P4.6
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_interruptEdgeSelect(GPIO_PORT_P4, GPIO_PIN6, GPIO_HIGH_TO_LOW_TRANSITION);

    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    PIR_enable();
    PIR_flag=0;

    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);
    delay_ms(10);

}

void PIR_disable(){
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
}

void PIR_enable(){
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
}

void PIR_IRQHandler(void)
{
    PIR_disable();
    PIR_flag = 1;
}


void PIR_change_addr(uint8_t new_addr){
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN1);

    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN1);
    delay_ms(10);

    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);
    delay_ms(10);

    I2C_setSlaveAddress(EUSCI_B1_BASE, 0x29);

    I2C_write_reg8(I2C_SLAVE_DEVICE_ADDRESS, new_addr & 0x7F);

    I2C_setSlaveAddress(EUSCI_B1_BASE, new_addr & 0x7F);
}


/*
void _RFIDInit(){
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}
*/


void I2C_write_reg8(uint8_t reg_addr, uint8_t data)
{

    /* Set to transmit mode */
    I2C_setMode(EUSCI_B1_BASE, EUSCI_B_I2C_TRANSMIT_MODE);

    /* Wait until bus is free */
    while (I2C_isBusBusy(EUSCI_B1_BASE));

    /* Start, send pointer, then data, then stop */
    I2C_masterSendMultiByteStart(EUSCI_B1_BASE, reg_addr);

    I2C_masterSendMultiByteFinish(EUSCI_B1_BASE, data);
}


void I2C_write_reg16(uint8_t reg_addr, uint16_t data)
{
    /* Set to transmit mode */
    I2C_setMode(EUSCI_B1_BASE, EUSCI_B_I2C_TRANSMIT_MODE);

    /* Wait until bus is free */
    while (I2C_isBusBusy(EUSCI_B1_BASE));

    /* Start, send pointer */
    I2C_masterSendMultiByteStart(EUSCI_B1_BASE, reg_addr);

    /* Send MSB (high byte) */
    I2C_masterSendMultiByteNext(EUSCI_B1_BASE, (uint8_t)(data >> 8));

    /* Send LSB (low byte) and stop */
    I2C_masterSendMultiByteFinish(EUSCI_B1_BASE, (uint8_t)(data & 0xFF));
}
