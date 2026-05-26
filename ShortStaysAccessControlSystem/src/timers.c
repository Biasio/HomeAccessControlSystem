#include "timers.h"

void _ClockSystemInit() {
    // Increase core voltage to safely support 48 MHz operation
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    // Add 1 wait state to Flash banks for reliable 48 MHz reads
    FlashCtl_setWaitState(FLASH_BANK0, 1);
    FlashCtl_setWaitState(FLASH_BANK1, 1);

    // Configure pins P4.2 and P4.3 for the external 48 MHz crystal (HFXT)
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN3 | GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    // Define external crystal frequencies (LFXT = 32.768 kHz, HFXT = 48 MHz)
    CS_setExternalClockSourceFrequency(32768, 48000000);

    // Start the HFXT oscillator and wait for it to stabilize
    CS_startHFXT(false);

    // MCLK = 48 MHz (CPU and Display)
    CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // HSMCLK = 24 MHz (High-speed peripherals)
    CS_initClockSignal(CS_HSMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_2);

    // SMCLK = 12 MHz (Provides perfectly accurate baud rates for UART, Timers, I2C)
    CS_initClockSignal(CS_SMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);

    // ACLK = ~9.4 kHz (Ultra-low power VLO for idle/debounce timers)
    CS_initClockSignal(CS_ACLK, CS_VLOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    // BCLK = 12 MHz (Backup clock domain)
    CS_initClockSignal(CS_BCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_4);
}



void _idleTimerInit()
{
    const Timer_A_UpModeConfig idleTimerConfig =
    {
        TIMER_A_CLOCKSOURCE_ACLK,       // VLO ~9.4kHz
        TIMER_A_CLOCKSOURCE_DIVIDER_64, // ~146 Hz
        1460,                                   // CCR0 Value (1460 counts = 10s) CCR Value = f_{CLK} x desired_time
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Overflow ISR
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE      // Enable interrupt for CCR0
    };


    Timer_A_stopTimer(TIMER_A2_BASE);
    Timer_A_configureUpMode(TIMER_A2_BASE, &idleTimerConfig);
    Timer_A_clearInterruptFlag(TIMER_A2_BASE);
    Interrupt_enableInterrupt(INT_TA2_0);

    standby = 0;
}


void AODClockTimerInit(){
    const Timer_A_UpModeConfig thirtySecTimerConfig =
    {
        TIMER_A_CLOCKSOURCE_ACLK,   //9.4 kHz
        TIMER_A_CLOCKSOURCE_DIVIDER_64,
        4406,                                   // 30 seconds
        TIMER_A_TAIE_INTERRUPT_DISABLE,
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE
    };

    Timer_A_stopTimer(TIMER_A2_BASE);
    Timer_A_configureUpMode(TIMER_A2_BASE, &thirtySecTimerConfig);
    Timer_A_clearInterruptFlag(TIMER_A2_BASE);
    Interrupt_enableInterrupt(INT_TA2_0);
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);

}


void _debounceTimerInit()
{
    const Timer_A_UpModeConfig debounceTimerConfig =
    {
        TIMER_A_CLOCKSOURCE_ACLK,       // VLO ~9.4kHz
        TIMER_A_CLOCKSOURCE_DIVIDER_64, // ~146 Hz = ~6ms
        4,                                   // CCR0 Value (4 counts = ~24ms) CCR Value = f_{CLK} x desired_time
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Overflow ISR
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE      // Enable interrupt for CCR0
    };

    Timer_A_stopTimer(TIMER_A1_BASE);
    Timer_A_configureUpMode(TIMER_A1_BASE, &debounceTimerConfig);
    Timer_A_clearInterruptFlag(TIMER_A1_BASE);
    Interrupt_enableInterrupt(INT_TA1_0);

}






void _ADCtimerInit(){

    //timer used to slow down the adc conversion from the joystick
    const Timer_A_ContinuousModeConfig continuousModeConfig =
    {
        TIMER_A_CLOCKSOURCE_SMCLK,
        TIMER_A_CLOCKSOURCE_DIVIDER_4,
        TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
        TIMER_A_DO_CLEAR                    // Clear Counter
    };

    Timer_A_stopTimer(TIMER_A3_BASE);

    /* Configuring Continuous Mode */
    Timer_A_configureContinuousMode(TIMER_A3_BASE, &continuousModeConfig);

    Timer_A_clearInterruptFlag(TIMER_A3_BASE);
    Interrupt_enableInterrupt(INT_TA3_N);

    /* Starting the Timer_A3 in continuous mode */
    Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE);
}


void _SysTickInit(){
    SysTick_setPeriod(48000); //1ms period
    HWREG(NVIC_ST_CURRENT) = 1;
    SysTick_enableModule();
    SysTick_enableInterrupt();
    system_millis = 0;
}


void delay_ms(uint32_t msec) {
    uint32_t start_time = system_millis;
    while ((system_millis - start_time) < msec);
}
