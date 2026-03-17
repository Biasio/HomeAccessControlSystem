#include "fsm.h"

int saved_pin_user[4] = {1,1,1,1};
int saved_pin_admin[4] = {9,9,9,9};
int selected_pin_user[4] = {0,0,0,0};

int error_pin = 0; //variable to count the number of wrong pin, when is equal to 3, block access

const uint16_t* current_results;

int32_t displayX = 64;
int32_t displayY = 64;


// ------------------------------------------------------ //

void _hwInit(void){
    /* Halting WDT and disabling master interrupts */
    WDT_A_holdTimer();
    Interrupt_disableMaster();

    /* Set the core voltage level to VCORE1 */
    PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    _ClockSystemInit();

    //display
    _graphicsInit();

    //joystick
    _adcInit();
    _ADCtimerInit();

    //buttons
    _pushButtonsInit();

    // Presence sensor
    _PIRInit();
    _idleTimerInit();

    //PWM for the buzzer
    _PWMtimerInit();
    _buzzerInit();

}


void door_locked(void){
    //it is better to update every tot time
    //here we show the hour and minutes
    display_string("10 : 00");
}



void insert_pin(bool pin){ //MAYBE WE CAN ADD LMPO HERE

    //pin = 0 -> SELECTED PIN
    //pin = 1 -> SAVED PIN

    bool number_pin_aquired;

    int x = 35; //position of selected digit on screen

    int i;
    //acquire 4 digit
    for(i=0;i<4;i++){
        number_pin_aquired=0;
        do{

            if (standby) return;

            //get results from joystick
            current_results = get_results_buffer();

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

                    if(!pin){ //selected pin
                        selected_pin_user[i]=selected_val;
                        sprintf(string, "%d", selected_pin_user[i]);
                    } else{   //saved pin
                        saved_pin_user[i]=selected_val;
                        sprintf(string, "%d", saved_pin_user[i]);
                    }

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
                    i--;    //turn back at the previous digit
                    x-=20;

                    printf("\ni=%d \n",i);
                    printf("x=%d \n\n",x);

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
}


void open_door(void){
    // - show on display that door is opening
    // - turn on servo
    // - turn on LED
    // ? make a sound to signal that the code is correct

    display_door_open();
    buzzerPWMgen(&StarWars);
    int i;
    for(i=0;i<1000000;i++); //simulate opening of the door, IS BETTER TO USE A TIMER

}



void wait_RFID(void){
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
        current_results = get_results_buffer();

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
    display_menu_last_access_log();
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


void wait_reset_door(void){

}

// ------------------------------------------------ //


State_t cur_state = STATE_BOOT;

StateMachine_t fsm[] = {
     {STATE_BOOT, fn_BOOT},
     {STATE_DOOR_LOCKED, fn_DOOR_LOCKED},
     {STATE_INSERT_PIN, fn_INSERT_PIN},
     {STATE_OPEN_DOOR, fn_OPEN_DOOR},
     {STATE_WAIT_RFID, fn_WAIT_RFID},
     {STATE_ADMIN_MENU, fn_ADMIN_MENU},

     {STATE_LAST_ACCESS_LOG, fn_menu_lal},
     {STATE_SETUP_PIN, fn_menu_setup_pin},
     {STATE_SETUP_WIFI, fn_menu_wifi},
     {STATE_FACTORY_RESET, fn_menu_fact_reset},
     {STATE_UNLOCK_DOOR, fn_menu_unlock_door},
     {STATE_BLOCK_PIN, fn_menu_block_pin},

     {STATE_WRONG_PIN, fn_WRONG_PIN},
     {STATE_BLOCK_ACCESS, fn_BLOCK_ACCESS},
     {STATE_WAIT_RESET_DOOR, fn_WAIT_RESET_DOOR},
};


// -----------------------------------------------//
// Implementation of the state's functions

void fn_BOOT(void){
    printf("Boot \n");
    _hwInit();
    cur_state = STATE_DOOR_LOCKED;
}



void fn_DOOR_LOCKED(void){

    Timer_A_stop(TIMER_A2_BASE); //stop the idle timer

    door_locked();

    if (PIR_flag)
    {
        PIR_flag = 0;
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
        // if the PIR interrupt is disabled, enable it
        if (!(P2->IE & BIT0))
        {
            GPIO_enableInterrupt(GPIO_PORT_P3, GPIO_PIN0);
            PIR_flag = 0;
        }
        return; // keep this fsm state
    }

    // Disable PIR interrupt and change state
    GPIO_disableInterrupt(GPIO_PORT_P3, GPIO_PIN0);
    PIR_flag = 0; // safety measure if PIR has been retriggered meanwhile

    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE);

    cur_state = STATE_INSERT_PIN;
}



void fn_INSERT_PIN(void){

    display_string("INSERT PIN");
    draw_grid();
    insert_pin(0);

    bool user_pin_correct = 1;
    bool admin_pin_correct = 1;

    // --- 1. Check for USER PIN ---
    int i;
    for(i=0;i<4;i++){
        user_pin_correct = 1;
        if(selected_pin_user[i]!=saved_pin_user[i]){ //if pin_user is wrong
            user_pin_correct = 0;
            break;
        }
    }

    // --- 2. Check if the User PIN was Correct ---
    if(user_pin_correct){
        error_pin = 0; //pin correct, reset counter of errors
        cur_state = STATE_OPEN_DOOR;
    } else {
        // --- 3. If User PIN was wrong, Check for ADMIN PIN ---

        for(i=0;i<4;i++){
            if(selected_pin_user[i]!=saved_pin_admin[i]){ //if pin_admin is wrong
                admin_pin_correct = 0;
                break;
            }
        }
        // --- 4. Final State Decision ---
        if (admin_pin_correct) {
            error_pin = 0; //pin correct, reset counter of errors
            cur_state = STATE_WAIT_RFID;
        } else {
            cur_state = STATE_WRONG_PIN;
        }
    }
}


void fn_OPEN_DOOR(void){
    open_door();

    cur_state = STATE_DOOR_LOCKED;
}


void fn_WAIT_RFID(void){
    wait_RFID();

    //if RFID signal doesn't arrive after a certain amount of time, display a message and go back to INSERT_PIN

    cur_state = STATE_ADMIN_MENU;
}



void fn_ADMIN_MENU(void){
    int selected_function;
    selected_function = admin_menu();

    //decide to which state of admin menu go
    switch(selected_function){
    case 0:
        cur_state = STATE_LAST_ACCESS_LOG;
        break;
    case 1:
        cur_state = STATE_SETUP_PIN;
        break;
    case 2:
        cur_state = STATE_SETUP_WIFI;
        break;
    case 3:
        cur_state = STATE_FACTORY_RESET;
        break;
    case 4:
        cur_state = STATE_UNLOCK_DOOR;
        break;
    case 5:
        cur_state = STATE_BLOCK_PIN;
        break;
    case 6:
    default:
        cur_state = STATE_INSERT_PIN;
        break;
    }
}

//Functions of admin menu
// --------------------------------------------- //

void fn_menu_lal(void){
    menu_last_access_log();

    //show the time of last access (and also pin used)

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}


void fn_menu_setup_pin(void){
    bool pins_equal = 0;

    do{
        display_menu_setup_pin();
        insert_pin(1);

        // verify if pin_admin = pin_user
        int i;
        for(i=0;i<4;i++){
            if(saved_pin_user[i]==saved_pin_admin[i]){
                pins_equal = 1;
            }else pins_equal = 0;
        }
        if(pins_equal){
            display_string("PIN ADMIN = PIN USER");
            display_string("CHOOSE ANOTHER PIN");
        }
    } while(pins_equal);

    display_string("PIN CHANGED");

    cur_state = STATE_ADMIN_MENU;
}


void fn_menu_wifi(void){
    menu_setup_wifi();

    //connect to wifi

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}


void fn_menu_fact_reset(void){
    menu_factory_reset();

    //maybe can reset user pin

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}


void fn_menu_unlock_door(void){
    menu_unlock_door();

    //move servo (same function of when opening door)

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}


void fn_menu_block_pin(void){
    menu_block_pin();

    //block the insertion of the pin, it can be unlock only with RFID

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}



// --------------------------------------------- //

void fn_WRONG_PIN(void){
    printf("Wrong pin \n");
    wrong_pin(); //show an error message on display

    if(error_pin<3){
        cur_state = STATE_INSERT_PIN;
    }else if(error_pin==3){
        error_pin = 0; //pin wrong for 3 times, reset counter of errors
        cur_state = STATE_BLOCK_ACCESS;
    }
}



void fn_BLOCK_ACCESS(void){
    printf("Access blocked \n");
    block_access();
    cur_state = STATE_WAIT_RESET_DOOR;
}



void fn_WAIT_RESET_DOOR(void){
    printf("Wait door to be reset \n");
    wait_reset_door();
    cur_state = STATE_INSERT_PIN;
    //if telegram or RFID
    //  cur_state = STATE_INSERT_PIN;
}


// ---------------------------------------------//
// Function to run in the main
void FSM_Run(void){

    if (cur_state < NUM_STATES)
    {
        if (standby == 1)
        {
            standby = 0;
            cur_state = STATE_DOOR_LOCKED;
        }

        (*fsm[cur_state].state_function)();
    }
    else
    {
        // Gestione errore stato non valido
    }

    //PCM_gotoLPM0();
}
