#include "sensors.h"

// ToF sensor VL53L0X

volatile uint8_t ToF_flag = 0;
volatile bool ToF_ready = 0;

/*
    The ToF will rely solely on the GPIO interrupt,
    and the threshold for the sensor is setup via I2C read onto peripheral's registers.
    A driver for the sensor is included under 'src/external_src/drivers/'
*/

void ToF_Init(){
    i2c_init();
    xshut_gpio_init();
    ToF_ready = vl53l0x_init();
    if(ToF_ready) interrupt_gpio_init();
    printf("end of T0F_Init: %d", ToF_ready);
    return;
}

void ToF_IRQHandler(void){
    ToF_flag = 1;
    GPIO_disableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
    return;
}

bool ToF_disable(){
    if (!ToF_ready) return false;

    uint16_t dummy=0;
    bool status = vl53l0x_read_range_interrupt(&dummy);
    status &= vl53l0x_stop_continuous(); //need to disable continuous mode

    ToF_ready=0;
    xshut_toggle(false); //sensor to standby

    return status;
}



bool ToF_enable(){

    ToF_ready &= vl53l0x_init();
    ToF_ready &= vl53l0x_start_continuous();

    if (!ToF_ready){
        vl53l0x_stop_continuous();
        xshut_toggle(false);
        i2c_recover();
    }
    else
    {
        GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN6);
        GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN6);
    }
    printf("end of T0F_enable: %d", ToF_ready);
    return ToF_ready;
}


bool ToF_validate_interrupt(void)
{
    bool status = false;
    uint16_t range=0;

    if (ToF_flag == 1){
        status &= vl53l0x_read_range_interrupt(&range);
        status &= ToF_disable();
    }
    return status;
}



// RFID MFRC522

volatile bool RFID_ready = 0;
const uint8_t RFID_saved[RFID_UID_LENGTH] = {1,1,1,1,1};



static void RFID_WriteRegister(uint8_t reg, uint8_t value) {
    /*
     * Bit 0 must always be 0.
     * Bit 7 (MSB) is the R/W flag: 0 for write, 1 for read.
     * */
    uint8_t addr = (reg << 1) & 0x7E;


    RFID_CS_Low(); // activate the CS
    SPI_transmitData(RFID_EUSCI, addr);
    // Wait for TX to complete (first byte)
    uint32_t t_start = system_millis;
    while (!(SPI_getInterruptStatus(RFID_EUSCI, RFID_EUSCI_RX_INT)) && (t_start - system_millis < SPI_TIMEOUT)); //wait for ACK
    SPI_receiveData(RFID_EUSCI); // Dummy read to clear the RX flag

    SPI_transmitData(RFID_EUSCI, value);
    // Wait for TX complete (data byte)
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(RFID_EUSCI, RFID_EUSCI_RX_INT)) && (t_start - system_millis < SPI_TIMEOUT)); //Wait for ACK
    SPI_receiveData(RFID_EUSCI); // Dummy read to clear the RX flag
    RFID_CS_High();
}

static uint8_t RFID_ReadRegister(uint8_t reg) {
    /*
     * Bit 0 must always be 0.
     * Bit 7 (MSB) is the R/W flag: 0 for write, 1 for read.
     * */
    uint8_t addr = ((reg << 1) & 0x7E) | 0x80;

        RFID_CS_Low();
        SPI_transmitData(RFID_EUSCI, addr);
        // Wait for transfer to complete and discard dummy RX data
        uint32_t t_start = system_millis;
        while (!(SPI_getInterruptStatus(RFID_EUSCI, RFID_EUSCI_RX_INT)) && (t_start - system_millis < SPI_TIMEOUT));
        SPI_receiveData(RFID_EUSCI);

        // Send dummy write to clock in the valid response
        SPI_transmitData(RFID_EUSCI, 0xFF);
        t_start = system_millis;
        while (!(SPI_getInterruptStatus(RFID_EUSCI, RFID_EUSCI_RX_INT)) && (t_start - system_millis < SPI_TIMEOUT));
        uint8_t data = SPI_receiveData(RFID_EUSCI);
        RFID_CS_High();
        return data;
}

static void MFRC522_SoftReset(void) {
    RFID_WriteRegister(MFRC522_COMMAND_REG, MFRC522_CMD_SOFT_RESET);
    // Wait for the PowerDown bit to clear (bit 4 of CommandReg)
    uint32_t t_start = system_millis;
    while ((system_millis - t_start) < 50) {
        if (!(RFID_ReadRegister(MFRC522_COMMAND_REG) & 0x10))
            break;
    }
}

static void MFRC522_AntennaOn(void) {
    uint8_t temp = RFID_ReadRegister(MFRC522_TX_CONTROL_REG);
    if (!(temp & 0x03)) {
        RFID_WriteRegister(MFRC522_TX_CONTROL_REG, temp | 0x03);
    }
}

static bool RFID_Transceive(uint8_t *txData, uint8_t txLen, uint8_t *rxData, uint8_t *rxLen) {
    // Flush FIFO
    RFID_WriteRegister(MFRC522_FIFO_LEVEL_REG, 0x80);

    // Write data to FIFO
    for (int i = 0; i < txLen; i++) {
        RFID_WriteRegister(MFRC522_FIFO_DATA_REG, txData[i]);
    }

    // Execute Transceive command
    RFID_WriteRegister(MFRC522_COMMAND_REG, MFRC522_CMD_TRANSCEIVE);

    // Start transmission (Set bit 7 of BitFramingReg without wiping lower bits)
    uint8_t framing = RFID_ReadRegister(MFRC522_BIT_FRAMING_REG);
    RFID_WriteRegister(MFRC522_BIT_FRAMING_REG, framing | 0x80);

    // Wait for RxIRq (bit 5) or Timeout (bit 7)
    uint32_t t_start = system_millis;
    while ((system_millis - t_start) < 50) {
        uint8_t irq = RFID_ReadRegister(0x04); // ComIrqReg
        if (irq & 0x20) break;                 // Success
        if (irq & 0x80) return false;          // Timeout
    }

    // Read response from FIFO
    *rxLen = RFID_ReadRegister(MFRC522_FIFO_LEVEL_REG) & 0x7F;
    if (*rxLen == 0) return false;

    for (int i = 0; i < *rxLen; i++) {
        rxData[i] = RFID_ReadRegister(MFRC522_FIFO_DATA_REG);
    }

    return true;
}


void RFID_CS_Low(void) {
   // no bus conflict since every peripheral has its own module
    GPIO_setOutputLowOnPin(RFID_CS_PORT, RFID_CS_PIN);

}

void RFID_CS_High(void) {
    // no bus conflict since every peripheral has its own module
    GPIO_setOutputHighOnPin(RFID_CS_PORT, RFID_CS_PIN);
}


bool RFID_Init(void) {
    // Configure SPI pins
    GPIO_disableInterrupt(RFID_SCK_PORT, RFID_SCK_PIN);
    GPIO_clearInterruptFlag(RFID_SCK_PORT, RFID_SCK_PIN);
    GPIO_disableInterrupt(RFID_MOSI_PORT, RFID_MOSI_PIN);
    GPIO_clearInterruptFlag(RFID_MOSI_PORT, RFID_MOSI_PIN);
    GPIO_disableInterrupt(RFID_MISO_PORT, RFID_MISO_PIN);
    GPIO_clearInterruptFlag(RFID_MISO_PORT, RFID_MISO_PIN);
    GPIO_setAsPeripheralModuleFunctionOutputPin(RFID_SCK_PORT, RFID_SCK_PIN, GPIO_SECONDARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(RFID_MOSI_PORT, RFID_MOSI_PIN, GPIO_TERTIARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(RFID_MISO_PORT, RFID_MISO_PIN, GPIO_TERTIARY_MODULE_FUNCTION);

    // CS pin as output, idle high
    GPIO_disableInterrupt(RFID_CS_PORT, RFID_CS_PIN);
    GPIO_clearInterruptFlag(RFID_CS_PORT, RFID_CS_PIN);
    GPIO_setAsOutputPin(RFID_CS_PORT, RFID_CS_PIN);
    RFID_CS_High();

    // Reset pin as output and drive low
    GPIO_setAsOutputPin(RFID_RST_PORT, RFID_RST_PIN);
    GPIO_setOutputLowOnPin(RFID_RST_PORT, RFID_RST_PIN);

    // Configure SPI module (EUSCI_B2) but keep it disabled for now
    eUSCI_SPI_MasterConfig spiConfig = {
        EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
        CS_getSMCLK(),                            // clockSourceFrequency
        4000000,                                  // desiredSpiClock (4 MHz)
        EUSCI_B_SPI_MSB_FIRST,
        EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
        EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
        EUSCI_B_SPI_3PIN
    };
    SPI_initMaster(RFID_EUSCI, &spiConfig);
    // Module remains disabled until RFID_Enable()
    RFID_ready=1;

    return true;
}

bool RFID_Enable(void) {
    if(!RFID_ready) RFID_Init();

    // Hardware reset sequence
    GPIO_setOutputHighOnPin(RFID_RST_PORT, RFID_RST_PIN);  // release reset
    delay_ms(10);                                          // stabilization
    GPIO_setOutputLowOnPin(RFID_RST_PORT, RFID_RST_PIN);   // assert reset
    delay_ms(10);
    GPIO_setOutputHighOnPin(RFID_RST_PORT, RFID_RST_PIN);  // release
    delay_ms(50);                                          // wait for boot

    // Enable SPI module
    SPI_enableModule(RFID_EUSCI);

    // Perform initialization sequence
    MFRC522_SoftReset();

    // Verify version register (expected 0x92 for MFRC522)
    uint8_t version = RFID_ReadRegister(MFRC522_VERSION_REG);
    if (version != 0x92 && version != 0x91) {
        return false;
    }

    // Timer settings: TModeReg = 0x80 (internal 13.56 MHz/2), TPrescalerReg = 0xA9, TReload = 0x10 * 0xE8
    RFID_WriteRegister(MFRC522_T_MODE_REG, 0x80);
    RFID_WriteRegister(MFRC522_T_PRESCALER_REG, 0xA9);
    RFID_WriteRegister(MFRC522_T_RELOAD_H_REG, 0x03);
    RFID_WriteRegister(MFRC522_T_RELOAD_L_REG, 0xE8);

    // Force 100% ASK modulation, set modulation width to 0x26
    RFID_WriteRegister(MFRC522_TX_ASK_REG, 0x40);
    RFID_WriteRegister(MFRC522_MODE_REG, 0x3D);  // CRC initial value 0x6363

    // Turn on antenna
    MFRC522_AntennaOn();

    return true;
}

bool RFID_Disable(void) {
    // Software power down MFRC522
    RFID_WriteRegister(MFRC522_COMMAND_REG, MFRC522_CMD_IDLE);
    uint8_t temp = RFID_ReadRegister(MFRC522_CONTROL_REG);
    RFID_WriteRegister(MFRC522_CONTROL_REG, temp | 0x10); // set PowerDown bit

    // Disable SPI module
    SPI_disableModule(RFID_EUSCI);

    // Ensure CS is high
    RFID_CS_High();

    _pushButtonsInit(); //revert the gpio's function as button

    RFID_ready=0;

    return true;
}

bool RFID_ReadTag(uint8_t *uid, uint8_t *uidLength) {
    uint8_t buffer[5];
    uint8_t len;

    // --- 1. REQA (Is a card present?) ---
    RFID_WriteRegister(MFRC522_BIT_FRAMING_REG, 0x07); // 7 valid bits for REQA
    buffer[0] = 0x26;                                  // REQA command

    if (!RFID_Transceive(buffer, 1, buffer, &len) || len < 2) {
        return false; // No card or bad response
    }

    // --- 2. Anticollision (Read the UID) ---
    RFID_WriteRegister(MFRC522_BIT_FRAMING_REG, 0x00); // 8 valid bits
    buffer[0] = 0x93;                                  // Anticollision command
    buffer[1] = 0x20;                                  // NVB (Number of Valid Bits)

    if (!RFID_Transceive(buffer, 2, buffer, &len) || len < 5) {
        return false; // Failed to read UID
    }

    // --- 3. Verify Checksum (BCC) ---
    if (buffer[4] != (buffer[0] ^ buffer[1] ^ buffer[2] ^ buffer[3])) {
        return false; // Corrupted read
    }

    // --- 4. Success: Copy UID to user variables ---
    for (int i = 0; i < 4; i++) {
        uid[i] = buffer[i];
    }
    *uidLength = 4;

    return true;
}
