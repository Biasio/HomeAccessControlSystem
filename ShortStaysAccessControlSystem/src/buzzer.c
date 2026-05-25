#include "buzzer.h"


const note StarWars_Notes[] = {
        {NOTE_AS4, 250}, {NOTE_AS4, 250}, {NOTE_AS4, 250},
        {NOTE_F5, 1000}, {NOTE_C6, 1000},
        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F6, 1000}, {NOTE_C6, 500},
        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F6, 1000}, {NOTE_C6, 500},
        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_AS5, 250}, {NOTE_G5, 1000}, {NOTE_C5, 250}, {NOTE_C5, 250}, {NOTE_C5, 250},
        {NOTE_F5, 1000}, {NOTE_C6, 1000},
        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F6, 1000}, {NOTE_C6, 500},

        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F6, 1000}, {NOTE_C6, 500},
        {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_AS5, 250}, {NOTE_G5, 1000}, {NOTE_C5, 375}, {NOTE_C5, 125},
        {NOTE_D5, 750},  {NOTE_D5, 250},  {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F5, 250},
        {NOTE_F5, 250},  {NOTE_G5, 250},  {NOTE_A5, 250},  {NOTE_G5, 500}, {NOTE_D5, 250}, {NOTE_E5, 500}, {NOTE_C5, 375}, {NOTE_C5, 125},
        {NOTE_D5, 750},  {NOTE_D5, 250},  {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F5, 250},

        {NOTE_C6, 375},  {NOTE_G5, 125},  {NOTE_G5, 1000}, {REST, 250},    {NOTE_C5, 250},
        {NOTE_D5, 750},  {NOTE_D5, 250},  {NOTE_AS5, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_F5, 250},
        {NOTE_F5, 250},  {NOTE_G5, 250},  {NOTE_A5, 250},  {NOTE_G5, 500}, {NOTE_D5, 250}, {NOTE_E5, 500}, {NOTE_C6, 375}, {NOTE_C6, 125},
        {NOTE_F6, 500},  {NOTE_DS6, 250}, {NOTE_CS6, 500}, {NOTE_C6, 250}, {NOTE_AS5, 500}, {NOTE_GS5, 250}, {NOTE_G5, 500}, {NOTE_F5, 250},
        {NOTE_C6, 2000}
};

const audio_data StarWars = {
    StarWars_Notes,
    (sizeof(StarWars_Notes) / sizeof(StarWars_Notes[0]))
};



volatile Timer_A_PWMConfig buzzerPWMconfig =
{
    TIMER_A_CLOCKSOURCE_SMCLK,
    TIMER_A_CLOCKSOURCE_DIVIDER_16,
    100,    //PWM timer period count
    TIMER_A_CAPTURECOMPARE_REGISTER_4,
    TIMER_A_OUTPUTMODE_RESET_SET,
    50 //PWM Duty cycle count
};

void _buzzerInit(){
    Timer_A_stop(TIMER_A0_BASE);
    Interrupt_disableInterrupt(INT_TA0_N);
    Interrupt_disableInterrupt(INT_TA0_0);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2,GPIO_PIN7,GPIO_PRIMARY_MODULE_FUNCTION);
}



void buzzerPWMgen(const audio_data* song){

    uint32_t i,freq = 0;

    for(i=0; i < song->length; ++i){
        freq = song->notes[i].pitch;

        freq == 0 ? (freq = 0) : (freq = CS_getSMCLK() / (freq*16));

        if (freq > 65535) freq = 65535; // check if freq is greather than 16 bit uint

        (&buzzerPWMconfig)->timerPeriod = freq;
        (&buzzerPWMconfig)->dutyCycle = freq / 50;

        Timer_A_generatePWM(TIMER_A0_BASE, &buzzerPWMconfig);

        delay_ms(song->notes[i].time_ms);

        (&buzzerPWMconfig)->dutyCycle = 0;
        Timer_A_generatePWM(TIMER_A0_BASE, &buzzerPWMconfig);
        delay_ms(1); // silence for 1 ms
    }

    Timer_A_stop(TIMER_A0_BASE);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4);

    return;
}


