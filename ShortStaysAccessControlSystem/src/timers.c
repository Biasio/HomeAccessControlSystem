#include "timers.h"

void _ClockSystemInit(){
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);  //Master Clock Sources CPU and peripherals
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_2); //Subsystem Master Clock Sources peripherals
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_4); //Low-speed subsystem master clock Sources peripherals
    CS_initClockSignal(CS_BCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_4); // Low-speed backup domain clock Sources LPM peripherals
    CS_initClockSignal(CS_ACLK, CS_VLOCLK_SELECT, CS_CLOCK_DIVIDER_1);
}

static const Timer_A_UpModeConfig idleTimerConfig =
{
    TIMER_A_CLOCKSOURCE_ACLK,       // VLO ~9.4kHz
    TIMER_A_CLOCKSOURCE_DIVIDER_64, // ~146 Hz
    1460,                                   // CCR0 Value (1460 counts = 10s) CCR Value = f_{CLK} x desired_time
    TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Overflow ISR
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE      // Enable interrupt for CCR0
};

void _idleTimerInit()
{
    Timer_A_configureUpMode(TIMER_A2_BASE, &idleTimerConfig);
    Interrupt_enableInterrupt(INT_TA2_0);
    standby = 0;
}



static const Timer_A_UpModeConfig debounceTimerConfig =
{
    TIMER_A_CLOCKSOURCE_ACLK,       // VLO ~9.4kHz
    TIMER_A_CLOCKSOURCE_DIVIDER_64, // ~146 Hz = ~6ms
    4,                                   // CCR0 Value (4 counts = ~24ms) CCR Value = f_{CLK} x desired_time
    TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Overflow ISR
    TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE      // Enable interrupt for CCR0
};

void _debounceTimerInit()
{
    Timer_A_configureUpMode(TIMER_A1_BASE, &debounceTimerConfig);
    Interrupt_enableInterrupt(INT_TA1_0);
}



//timer used to slow down the adc conversion from the joystick
static const Timer_A_ContinuousModeConfig continuousModeConfig =
{
    TIMER_A_CLOCKSOURCE_SMCLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_16,
    TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
    TIMER_A_DO_CLEAR                    // Clear Counter
};

void _ADCtimerInit(){

    /* Configuring Continuous Mode */
    Timer_A_configureContinuousMode(TIMER_A3_BASE, &continuousModeConfig);

    Interrupt_enableInterrupt(INT_TA3_N);

    /* Starting the Timer_A3 in continuous mode */
    Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE);
}

void _SysTickInit(){
    SysTick_enableModule();
    SysTick_setPeriod((CS_getMCLK()/1000)-1); //1ms period
    SysTick_enableInterrupt();
    system_millis = 0;
}


void delay_ms(uint32_t msec) {
    uint32_t start_time = system_millis;
    while ((system_millis - start_time) < msec);
}
