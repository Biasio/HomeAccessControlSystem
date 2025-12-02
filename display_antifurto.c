#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>


typedef struct {
    int x;
    int y;
} Point;

const Point GRID_POINTS[] = {
     // P1 | P2 | P3
     { 10, 10 },
     { 50, 10 },
     { 90, 10 },
     // P4 | P5 | P6
     { 10, 50 },
     { 50, 50 },
     { 90, 50 },
     // P7 | P8 | P9
     { 10, 90 },
     { 50, 90 },
     { 90, 90 }
};

typedef enum {
    NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9, NUM_POINTS
} PointIndex;

int pos_x1 = 6;
int pos_x2 = 39;
int pos_y1 = 6;
int pos_y2 = 39;

bool move_rectangle;
bool button_pressed = 0;

Graphics_Context g_sContext;

static uint16_t resultsBuffer[2];

const Timer_A_ContinuousModeConfig continuousModeConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_64,      // SMCLK/1 = 32.768khz
        TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
        TIMER_A_DO_CLEAR                    // Clear Counter
};

void _timerInit(){
    /* Configuring Continuous Mode */
    Timer_A_configureContinuousMode(TIMER_A0_BASE, &continuousModeConfig);

    Interrupt_enableInterrupt(INT_TA0_N);

    /* Starting the Timer_A0 in continuous mode */
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_CONTINUOUS_MODE);
}

void _adcInit(){
    /* Configures Pin 6.0 and 4.4 as ADC input */
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
        GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

        /* Initializing ADC (ADCOSC/64/8) */
        ADC14_enableModule();
        ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

        /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
             * with internal 2.5v reference */
        ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
        ADC14_configureConversionMemory(ADC_MEM0,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

        ADC14_configureConversionMemory(ADC_MEM1,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

        /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
         *  is complete and enabling conversions */
        ADC14_enableInterrupt(ADC_INT1);

        /* Enabling Interrupts */
        Interrupt_enableInterrupt(INT_ADC14);
        Interrupt_enableMaster();

        /* Setting up the sample timer to automatically step through the sequence
         * convert.
         */
        ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

        /* Triggering the start of the sample */
        ADC14_enableConversion();
        ADC14_toggleConversionTrigger();
}

void _graphicsInit()
{
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    Graphics_setBackgroundColor(&g_sContext, ClrWhite);
    GrContextFontSet(&g_sContext, &g_sFontCmss36);
    Graphics_clearDisplay(&g_sContext);
}

void _hwInit()
{
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    _graphicsInit();
    _adcInit();
    _timerInit();
}

void draw_grid(void);
void draw_rectangle(bool new_pos);
bool update_rectangle_pos(uint64_t status);

int main(void){

    _hwInit();
    draw_grid();

    while(1){
        PCM_gotoLPM0();
    }

}





void draw_grid(void)
{
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    int i;
    for(i=2; i<=128; i+=41){
        Graphics_drawLineH(&g_sContext, 2, 125, i);
        Graphics_drawLineV(&g_sContext, i, 2, 125);
    }

    char string[1];
    int x,y;
    i=1;
    for(y=21; y<=106; y+=41){
        for(x=24; x<=106; x+=41){
            sprintf(string, "%d", i++);
            Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                        AUTO_STRING_LENGTH,
                                        x, y,
                                        OPAQUE_TEXT);
        }
    }
    Graphics_setForegroundColor(&g_sContext, ClrRed);
    const Graphics_Rectangle rectangle = {pos_x1, pos_y1, pos_x2, pos_y2};
    Graphics_drawRectangle(&g_sContext, &rectangle);

}

void TA0_N_IRQHandler(void)
{
    /* clear the timer pending interrupt flag */
    Timer_A_clearInterruptFlag(TIMER_A0_BASE);

    move_rectangle=1;
}

bool update_rectangle_pos(uint64_t status)
{
    return move_rectangle && (status & ADC_INT1);
}

void draw_rectangle(bool new_pos)
{
    if(!new_pos) Graphics_setForegroundColor(&g_sContext, ClrWhite);
    else Graphics_setForegroundColor(&g_sContext, ClrRed);

    const Graphics_Rectangle rectangle = {pos_x1, pos_y1, pos_x2, pos_y2};
    Graphics_drawRectangle(&g_sContext, &rectangle);
}


void ADC14_IRQHandler(void)
{
    uint64_t status;

    const int RIGHT = 16000;
    const int LEFT = 300;
    const int UP = 16000;
    const int DOWN = 100;

    status = ADC14_getEnabledInterruptStatus();
    ADC14_clearInterruptFlag(status);

    if(update_rectangle_pos(status))
    {
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);

        if(resultsBuffer[0]>RIGHT) {
            if(pos_x1<80)
            {
                draw_rectangle(0); //cancel old position

                pos_x1 += 41;
                pos_x2 += 41;

                draw_rectangle(1); //draw new rectangle
            }
        }
        if(resultsBuffer[0]<LEFT) {
            if(pos_x1>40)
            {
                draw_rectangle(0); //cancel old position

                pos_x1 -= 41;
                pos_x2 -= 41;

                draw_rectangle(1); //draw new rectangle
            }
        }
        if(resultsBuffer[1]>UP) {
            if(pos_y1>40)
            {
                draw_rectangle(0); //cancel old position

                pos_y1 -= 41;
                pos_y2 -= 41;

                draw_rectangle(1); //draw new rectangle
            }
        }
        if(resultsBuffer[1]<DOWN) {
            if(pos_y1<80)
            {
                draw_rectangle(0); //cancel old position

                pos_y1 += 41;
                pos_y2 += 41;

                draw_rectangle(1); //draw new rectangle
            }
        }
    }
    move_rectangle=0;


    /* Determine if JoyStick button is pressed */
    if (P4IN & GPIO_PIN1){
        button_pressed=0;
    }

    if (!(P4IN & GPIO_PIN1)) {

        if(!(button_pressed))
        {
            button_pressed=1;

            const Graphics_Rectangle rect = {pos_x1, pos_y1, pos_x2, pos_y2};

            int i;
            for (i = NUM1; i < NUM_POINTS; i++)
            {
                // Verifica se il punto corrente (GRID_POINTS[i]) è all'interno del rettangolo 'rect'
                if (Graphics_isPointWithinRectangle(
                        &rect,
                        GRID_POINTS[i].x,
                        GRID_POINTS[i].y))
                {
                    // Se il punto è all'interno, stampa il suo numero (i + 1)
                    printf("Number: %d\n", i + 1);

                    Graphics_setForegroundColor(&g_sContext, ClrRed);
                    Graphics_fillRectangle(&g_sContext, &rect);
                    int j;
                    for(j=0;j<100000;j++);
                    Graphics_setForegroundColor(&g_sContext, ClrWhite);
                    Graphics_fillRectangle(&g_sContext, &rect);


                    char string[2];

                    Graphics_setForegroundColor(&g_sContext, ClrBlack);

                    sprintf(string, "%d", i+1);
                    Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                                AUTO_STRING_LENGTH,
                                                pos_x1+18, pos_y1+15,
                                                OPAQUE_TEXT);


                    Graphics_setForegroundColor(&g_sContext, ClrRed);
                    Graphics_drawRectangle(&g_sContext, &rect);

                    // Se vuoi fermarti alla prima corrispondenza:
                     break;
                }
            }
        }
    }
}
