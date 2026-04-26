#include "fsm_helpers.h"
#include "joystick.h"
#include "display.h"
#include "push_button.h"
#include "sensors.h"
#include "timers.h"
#include "irqHandlers.h"
#include "buzzer.h"
#include "comm_esp.h"

uint8_t saved_pin_admin[4] = {9,9,9,9};
uint16_t error_pin = 0; //variable to count the number of wrong pin, when is equal to 3, block access

// -----------------------------------------------//
// Implementation of the state's functions


// ------------------------------------------------------ //

void _hwInit(void){
    WDT_A_holdTimer();
    Interrupt_enableMaster();

    /* Initializes Clock & FLASH System */
    _ClockSystemInit();
    _SysTickInit();

    //display
    _graphicsInit();

    //joystick
    _adcInit();
    _ADCtimerInit();

    //buttons
    _pushButtonsInit();

    // Presence sensor
    _ToFInit();
    _idleTimerInit();

    //PWM for the buzzer
    _buzzerInit();

    // Initialize UART communication with ESP32
    ESP_Comm_Init();
}


uint8_t insert_pin(){

    uint8_t pin_correct=0;
    int selected_pin_user[4] = {0,0,0,0};


    bool number_pin_aquired;

    int x = 35; //position of selected digit on screen

    int i;
    //acquire 4 digit
    for(i=0;i<4;i++){
        number_pin_aquired=0;
        do{
            if (standby) return 0; // Exit if system goes to sleep

            //get results from joystick
            const uint16_t *current_results = get_results_buffer();

            //if timer of joystick finished, move rectangle
            if(data_aquired()){
                move_rectangle_on_display(current_results[0], current_results[1], MOVE_ON_GRID);
            }

            //Button A (S1) used to select a digit
            if(buttonA_pressed){
                buttonA_pressed=0;
                int selected_val = number_selected();

                if (selected_val != -1)
                {
                    number_pin_aquired=1;
                    char string[2];
                    selected_pin_user[i]=selected_val;
                    snprintf(string, sizeof(string), "%d", selected_pin_user[i]);

                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    GrContextFontSet(&g_sContext, &g_sFontCmss18);
                    Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                               AUTO_STRING_LENGTH,
                                               x, 12,
                                               OPAQUE_TEXT);

                    x+=20;
                }
            }

            // BUTTON B to deselect a number (only if is not at the first digit)
            if(buttonB_pressed){
                buttonB_pressed=0;
                if(i>0){
                    i--;    // Go back to the previous digit
                    x-=20;

                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    Graphics_drawStringCentered(&g_sContext, " ",
                                                AUTO_STRING_LENGTH,
                                                x, 15,
                                                OPAQUE_TEXT);
                    Graphics_drawLineH(&g_sContext, x-5, x+4, 25);
                }

            }

        } while(number_pin_aquired==0); //when the digit is selected, exit from loop
    }


    // Convert the integer array from the display into a string
    char typed_pin_str[5];
    snprintf(typed_pin_str, sizeof(typed_pin_str), "%d%d%d%d",
             selected_pin_user[0],
             selected_pin_user[1],
             selected_pin_user[2],
             selected_pin_user[3]);

    // --- 1. Check for USER PIN ---
    int k;
    for(k=0; k < MAX_TEMP_USERS; ++k) {
        if(activeTempUsers[k].active && strcmp(typed_pin_str, activeTempUsers[k].pin) == 0) {
            pin_correct = 1;
            break;
        }
    }

    if(pin_correct != 1){
    // Fallback: Check for Admin PIN
        for(i=0; i<4; i++) {
            if(selected_pin_user[i] != saved_pin_admin[i]) {
                pin_correct = 2;
                break;
            }
        }
    }

    return pin_correct;
}


void open_door(void){
    // - show on display that door is opening
    // - turn on servo
    // - turn on LED
    // ? make a sound to signal that the code is correct

    display_door_open();
    //buzzerPWMgen(&StarWars);
    delay_ms(2500);

}



void wait_RFID(void){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("PLEASE, USE RFID");
    int i;
    for(i=0;i<1000000;i++); //IS BETTER TO USE A TIMER
}



int admin_menu(void){
    draw_admin_menu(1); // 1 = FIRST_SCREEN

    bool admin_menu_active = 1;

    while(admin_menu_active){
        if (standby) return -1;

        //get results from joystick
        const uint16_t *current_results = get_results_buffer();

        //if timer of joystick finished, move rectangle
        if(data_aquired()){
            move_rectangle_on_display(current_results[0], current_results[1], MOVE_ON_MENU);

            if(buttonA_pressed){
                buttonA_pressed=0;
                return display_function_selected();
            }
        }
        if(buttonB_pressed){    //turn back to grid (ADD A CONFIRMATION MENU ? )
            buttonB_pressed=0;
            admin_menu_active=0;
        }
    }
    return 6; //return to INSERT PIN
}



void menu_last_access_log(void){
}

void menu_setup_wifi(void){
    display_menu_setup_wifi();
}



void menu_factory_reset(void){
    display_menu_factory_reset();
}



void menu_unlock_door(void){
    display_menu_unlock_door();
}



void menu_block_pin(void){
    display_menu_block_pin();
}


void wrong_pin(void){
    // - turn on LED
    // - make a sound to signal that the code is incorrect

    error_pin++;

    display_wrong_pin(error_pin);
}


void block_access(void){
    display_block_access();

    int i;
    for(i=0;i<1000000;i++); //BETTER TO USE A TIMER
}

void door_lock(){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("DOOR LOCKED");
}


void wait_reset_door(void){

}

void go_to_idle(){
    display_string("Going to sleep...");
    delay_ms(1500);
}


bool check_for_inputs(){
    if (ToF_flag)
    {
        ToF_flag = 0;
        //I2C_write_reg8(SYSTEM_INTERRUPT_CLEAR, 0x01);
    }
    else if (buttonA_pressed)
    {
        buttonA_pressed=0;
    }
    else if (buttonB_pressed)
    {
        buttonB_pressed=0;
    }
    else // no input was received
    {
        if (!(P4->IE & BIT6)) // if the ToF interrupt isn't enabled
        {

            ToF_enable(); // enable the interrupt
            ToF_flag = 0; // safety measure if ToF has been retriggered meanwhile
        }
        return 0; // no interrupts were detected
    }

    ToF_disable(); // Disable ToF interrupt and change state
    ToF_flag = 0; // safety measure if ToF has been retriggered meanwhile

    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE); // restart the idle timer

    return 1; //signal that an input was detected
}



