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
    xshut_gpio_init();
    i2c_init();
    interrupt_gpio_init();
    return;
}




void ToF_IRQHandler(void){
    if(ToF_ready) ToF_flag = 1;
    PORT(VL53L0X_INT_PORT)->IE  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);
    PORT(VL53L0X_INT_PORT)->IFG  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);

    return;
}



void ToF_disable(){
    if (ToF_ready)
    {
        PORT(VL53L0X_INT_PORT)->IE  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);
        PORT(VL53L0X_INT_PORT)->IFG  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);

        uint16_t dummy=0;
        vl53l0x_read_range_interrupt(&dummy); //clears sensor flags
        vl53l0x_stop_continuous(); //disable continuous mode

        ToF_ready=0;
        //xshut_toggle(false); //sensor to standby
    }
    return;
}




void ToF_enable(){
    PORT(VL53L0X_INT_PORT)->IE  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);
    PORT(VL53L0X_INT_PORT)->IFG  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);

    ToF_ready = vl53l0x_init();
    ToF_ready &= vl53l0x_start_continuous();

    if (!ToF_ready){  //if init failed
        i2c_recover();
        vl53l0x_stop_continuous();
        xshut_toggle(false);

        static bool retries=RECOVER_TRIES;

        if((retries--)>0) ToF_enable();
        else {retries=RECOVER_TRIES; return;}
    }
    else
    {
        PORT(VL53L0X_INT_PORT)->IFG  &= ~ONE_HOT_BIT(VL53L0X_INT_PIN);
        PORT(VL53L0X_INT_PORT)->IE  |= ONE_HOT_BIT(VL53L0X_INT_PIN);
    }
    printf("end of T0F_enable: ToF_ready %d\n", ToF_ready);

    uint16_t dummy=0;
    vl53l0x_read_range_interrupt(&dummy);
    printf("end of T0F_enable measure: %" PRIu16 "\n", dummy);

    return;
}



// RFID MFRC522

volatile bool RFID_ready = 0;
const uint8_t RFID_saved[RFID_UID_LENGTH] = {233,52,245,162};



static void RFID_WriteRegister(uint8_t reg, uint8_t value) {
    /*
     * Bit 0 must always be 0.
     * Bit 7 (MSB) is the R/W flag: 0 for write, 1 for read.
     * */
    uint8_t addr = ((uint16_t) (reg << 1)) & 0x7E;
    uint32_t t_start =0;
    RFID_CS_Low(); // activate the CS

    // Wait for TX to complete (first byte)
    SPI_transmitData(EUSCI_B2_BASE, addr);
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT))
               && ((system_millis - t_start) < SPI_TIMEOUT));
        SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    // Wait for TX complete (data byte)
    SPI_transmitData(EUSCI_B2_BASE, value);
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT))
               && ((system_millis - t_start) < SPI_TIMEOUT));
        SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    while (SPI_isBusy(EUSCI_B2_BASE));
    RFID_CS_High();
}



static uint8_t RFID_ReadRegister(uint8_t reg){

    uint8_t addr = (((reg << 1) & 0x7E) | 0x80);  // MSB=1 for read
    uint8_t data = 0;
    uint32_t t_start;

    RFID_CS_Low();

    // Send address byte
    SPI_transmitData(EUSCI_B2_BASE, addr);
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT))
           && ((system_millis - t_start) < SPI_TIMEOUT));
    SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    // Read dummy byte (discard) – this clocks out the first byte
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT))
           && ((system_millis - t_start) < SPI_TIMEOUT));
    (void) SPI_receiveData(EUSCI_B2_BASE);
    SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);

    // Send 0x00 to clock out the actual register value
    SPI_transmitData(EUSCI_B2_BASE, 0x00);
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT))
           && ((system_millis - t_start) < SPI_TIMEOUT));
    SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_TRANSMIT_INTERRUPT);

    // Read the actual data
    t_start = system_millis;
    while (!(SPI_getInterruptStatus(EUSCI_B2_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT))
           && ((system_millis - t_start) < SPI_TIMEOUT));
    data = SPI_receiveData(EUSCI_B2_BASE);
    SPI_clearInterruptFlag(EUSCI_B2_BASE, EUSCI_B_SPI_RECEIVE_INTERRUPT);

    while (SPI_isBusy(EUSCI_B2_BASE));

    RFID_CS_High();
    return data;
}



static bool MFRC522_SoftReset(void) {
    RFID_WriteRegister(MFRC522_COMMAND_REG, MFRC522_CMD_SOFT_RESET);
    // Wait for the PowerDown bit to clear (bit 4 of CommandReg)
    uint32_t t_start = system_millis;
    while ((system_millis - t_start) < 200) {
        if (!(RFID_ReadRegister(MFRC522_COMMAND_REG) & 0x10)) //bit4 (fifth bit) must be 0 when ready
            return true;
    }
    return false;
}



static void MFRC522_AntennaOn(void) {
    uint8_t temp = RFID_ReadRegister(MFRC522_TX_CONTROL_REG);
    if (!(temp & 0x03)) {
        RFID_WriteRegister(MFRC522_TX_CONTROL_REG, temp | 0x03);
    }
}



static bool RFID_Transceive(uint8_t *txData, uint8_t txLen, uint8_t *rxData, uint8_t *rxLen) {
    // Flush FIFO (set bit 7 of FIFOLevelReg)
    RFID_WriteRegister(MFRC522_FIFO_LEVEL_REG, 0x80);

    // Write data to FIFO
    for (int i = 0; i < txLen; i++) {
        RFID_WriteRegister(MFRC522_FIFO_DATA_REG, txData[i]);
    }

    // Clear any pending interrupts
    RFID_WriteRegister(MFRC522_COM_IRQ_REG, 0x7F);

    // Execute Transceive command
    RFID_WriteRegister(MFRC522_COMMAND_REG, MFRC522_CMD_TRANSCEIVE);

    // Set StartSend bit (bit 7 of BitFramingReg)
    uint8_t framing = RFID_ReadRegister(MFRC522_BIT_FRAMING_REG);
    RFID_WriteRegister(MFRC522_BIT_FRAMING_REG, framing | 0x80);

    // Wait for completion (RxIRq, IdleIRq, or TimerIRq)
    uint32_t t_start = system_millis;
    uint8_t irq = 0;
    while ((system_millis - t_start) < 100) {  // 100 ms timeout
        irq = RFID_ReadRegister(0x04);  // ComIrqReg
        if (irq & 0x20) break;          // RxIRq – data received
        if (irq & 0x10) break;          // IdleIRq – command finished (may be set with no data)
        if (irq & 0x80) {               // TimerIRq – timeout
            return false;
        }
    }

    if ((system_millis - t_start) >= 100) {
        return false;
    }

    // Check for errors (ErrorReg, address 0x06)
    uint8_t error = RFID_ReadRegister(0x06);
    if (error & 0x01) { // ProtocolErr
        printf("Protocol error\n");
        return false;
    }
    if (error & 0x04) { // CollErr (only at 106 kBd)
        printf("Collision detected\n");
        return false;
    }
    if (error & 0x08) { // CRCErr
        printf("CRC error\n");
        return false;
    }

    // Read FIFO level
    *rxLen = RFID_ReadRegister(MFRC522_FIFO_LEVEL_REG) & 0x7F;
    if (*rxLen == 0) {
        return false;
    }

    for (int i = 0; i < *rxLen; i++) {
        rxData[i] = RFID_ReadRegister(MFRC522_FIFO_DATA_REG);
    }

    return true;
}



static inline void RFID_CS_Low(void) {

   // no bus conflict since every peripheral has its own module
    GPIO_setOutputLowOnPin(RFID_CS_PORT, RFID_CS_PIN);
}



static inline void RFID_CS_High(void) {

    // no bus conflict since every peripheral has its own module
    GPIO_setOutputHighOnPin(RFID_CS_PORT, RFID_CS_PIN);
}



bool RFID_Init(void) {
    // Configure SPI pins
    GPIO_disableInterrupt(RFID_SCK_PORT, RFID_SCK_PIN); //shared with the button!
    GPIO_clearInterruptFlag(RFID_SCK_PORT, RFID_SCK_PIN);
    GPIO_setAsPeripheralModuleFunctionOutputPin(RFID_SCK_PORT, RFID_SCK_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    GPIO_disableInterrupt(RFID_MOSI_PORT, RFID_MOSI_PIN);
    GPIO_clearInterruptFlag(RFID_MOSI_PORT, RFID_MOSI_PIN);
    GPIO_setAsPeripheralModuleFunctionOutputPin(RFID_MOSI_PORT, RFID_MOSI_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    GPIO_disableInterrupt(RFID_MISO_PORT, RFID_MISO_PIN);
    GPIO_clearInterruptFlag(RFID_MISO_PORT, RFID_MISO_PIN);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN7);
    GPIO_setAsPeripheralModuleFunctionInputPin(RFID_MISO_PORT, RFID_MISO_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    // CS pin as output, idle high
    GPIO_disableInterrupt(RFID_CS_PORT, RFID_CS_PIN);
    GPIO_clearInterruptFlag(RFID_CS_PORT, RFID_CS_PIN);
    GPIO_setAsOutputPin(RFID_CS_PORT, RFID_CS_PIN);
    RFID_CS_High();

    // Reset pin as output and drive low
    GPIO_setAsOutputPin(RFID_RST_PORT, RFID_RST_PIN);
    GPIO_setOutputLowOnPin(RFID_RST_PORT, RFID_RST_PIN);
    delay_ms(1); //required 100ns for the pull down to be valid

    uint32_t SMCLK_freq = CS_getSMCLK();
    // Configure SPI module (EUSCI_B2) but keep it disabled for now
    eUSCI_SPI_MasterConfig spiConfig = {
        EUSCI_B_SPI_CLOCKSOURCE_SMCLK,
        SMCLK_freq,                            // clockSourceFrequency
        4000000,                                  // desiredSpiClock (4 MHz), it must be lower than 10MHz
        EUSCI_B_SPI_MSB_FIRST,
        EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,
        EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW,
        EUSCI_B_SPI_3PIN
    };

    SPI_initMaster(EUSCI_B2_BASE, &spiConfig);
    // Module remains disabled until RFID_Enable()
    RFID_ready=1;

    return true;
}




bool RFID_Enable(void) {
    if(!RFID_ready){
        RFID_Init();
    }

    // Enable SPI module
    SPI_enableModule(EUSCI_B2_BASE);

    // Hardware reset sequence
    GPIO_setOutputHighOnPin(RFID_RST_PORT, RFID_RST_PIN);  // release
    delay_ms(50);                                          // wait for boot


    // Perform initialization sequence
    if (! MFRC522_SoftReset()) return (RFID_ready=0);

    // Verify version register
    uint8_t version = RFID_ReadRegister(MFRC522_VERSION_REG);
    printf("version: %" PRIu8 "\n", version);

    if ((version != 0x91) && (version != 0x92)) {
        RFID_Init(); //try to reinitialize the peripheral
        version = RFID_ReadRegister(MFRC522_VERSION_REG);
        if ((version != 0x91) && (version != 0x92)) return (RFID_ready=0);
    }

    // 3. Configure the timer
    RFID_WriteRegister(MFRC522_T_MODE_REG, 0x8D);
    RFID_WriteRegister(MFRC522_T_PRESCALER_REG, 0x3E);
    RFID_WriteRegister(MFRC522_T_RELOAD_H_REG, 0x00);
    RFID_WriteRegister(MFRC522_T_RELOAD_L_REG, 0x12);

    // 4. Configure receiver gain (RFCfgReg) – Section 9.3.3.6
    // Max gain (48dB) for better range:
    RFID_WriteRegister(MFRC522_RF_CFG_REG, 0x70);      // RxGain[2:0] = 111b (48dB)

    // 5. Configure modulation width (ModWidthReg) – Section 9.3.3.4
    // Default 0x26 is usually fine
    RFID_WriteRegister(MFRC522_MOD_WIDTH_REG, 0x26);

    // 6. Configure TxASK (Force 100% ASK) – you have this
    RFID_WriteRegister(MFRC522_TX_ASK_REG, 0x40);

    // 7. Configure ModeReg (CRC, etc.) – Section 9.3.2.2
    // Default after reset is 0x3F (MSBFirst=1, TxWaitRF=1, CRCPreset=11b)
    RFID_WriteRegister(MFRC522_MODE_REG, 0x3F);   // Or 0x3D as you had (CRC 0x6363)

    // 8. Turn antenna on (you have this)
    MFRC522_AntennaOn();

    // 9. Clear any pending interrupts
    RFID_WriteRegister(MFRC522_COM_IRQ_REG, 0x7F);

    return true;
}




bool RFID_Disable(void) {
    // Ensure CS is high (disabled)
    RFID_CS_High();

    // Disable SPI module
    SPI_disableModule(EUSCI_B2_BASE);

    delay_ms(100); //ensure SPI is dead

    GPIO_setAsOutputPin(RFID_RST_PORT, RFID_RST_PIN);
    GPIO_setOutputHighOnPin(RFID_RST_PORT, RFID_RST_PIN);

    //revert GPIO to normal functions
    GPIO_setAsInputPinWithPullUpResistor(RFID_SCK_PORT, RFID_SCK_PIN);
    GPIO_setAsInputPinWithPullUpResistor(RFID_MOSI_PORT, RFID_MOSI_PIN);
    _graphicsInit();
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
    printf("RFID detected: ");
    for (int i = 0; i < 4; i++) {
        uid[i] = buffer[i];
        printf("%" PRIu8 " ", uid[i]);
    }
    *uidLength = 4;
    printf("\n");

    return true;
}
