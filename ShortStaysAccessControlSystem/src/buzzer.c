#include "buzzer.h"


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

        if((VOLUME!=0) && (freq != 0)){
            freq = (CS_getSMCLK() / (freq << 4)) & (0xFFFF); //limit to 65535
            (&buzzerPWMconfig)->timerPeriod = freq;
            (&buzzerPWMconfig)->dutyCycle = (freq * VOLUME) >> 10 ;
            Timer_A_generatePWM(TIMER_A0_BASE, &buzzerPWMconfig);

            delay_ms(song->notes[i].time_ms);

            (&buzzerPWMconfig)->dutyCycle = 0;
            Timer_A_generatePWM(TIMER_A0_BASE, &buzzerPWMconfig);
            delay_ms(1); // silence for 1 ms
        }
        else
        {
            Timer_A_stop(TIMER_A0_BASE);
        }
    }

    Timer_A_stop(TIMER_A0_BASE);
    Timer_A_clearCaptureCompareInterrupt(TIMER_A0_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_4);

    return;
}



/* ########## SONGS ######## */

const note CorrectPin_Notes[] = {
                                 {NOTE_E6, 125},
                                     {NOTE_G6, 125},
                                     {NOTE_E7, 125},
                                     {NOTE_C7, 125},
                                     {NOTE_D7, 125},
                                     {NOTE_G7, 125}
};

const audio_data CorrectPin = {
                               CorrectPin_Notes,
                               (sizeof(CorrectPin_Notes) / sizeof(CorrectPin_Notes[0]))
};



const note WrongPin_Notes[] = {
                               {NOTE_DS3, 300},
                                   {NOTE_D3,  300},
                                   {NOTE_CS3, 300},
                                   {NOTE_C3,  800}
};

const audio_data WrongPin = {
                             WrongPin_Notes,
                             (sizeof(WrongPin_Notes) / sizeof(WrongPin_Notes[0]))
};



const note LockOut_Notes[] = {
                            {NOTE_B4, 160},
                            {NOTE_F5, 160},
                            {REST,    160},
                            {NOTE_F5, 160},
                            {NOTE_F5, 200},
                            {NOTE_E5, 200},
                            {NOTE_D5, 200},
                            {NOTE_C5, 550}
};

const audio_data LockOut = {
                            LockOut_Notes,
                             (sizeof(LockOut_Notes) / sizeof(LockOut_Notes[0]))
};


const note CorrectRFID_Notes[] = {
                                  {NOTE_B5, 80},
                                  {NOTE_E6, 400}
};

const audio_data CorrectRFID = {
                                CorrectRFID_Notes,
                                (sizeof(CorrectRFID_Notes) / sizeof(CorrectRFID_Notes[0]))
};
