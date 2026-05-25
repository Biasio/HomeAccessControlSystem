#include "fsm_helpers.h"


uint8_t saved_pin_admin[4] = {9,9,9,9};
uint16_t error_pin = 0; //variable to count the number of wrong pin, when is equal to 3, block access
dbStates dbstate;

// -----------------------------------------------//
// Implementation of the state's functions


// ------------------------------------------------------ //

void _hwInit(void){
    WDT_A_holdTimer();
    Interrupt_enableMaster();

    /* Initializes Clock & FLASH System */
    _ClockSystemInit();
    _SysTickInit();

    //joystick
    _adcInit();
    _ADCtimerInit();

    //disable the CS gpio for RFID It might be
    // left low after a reset without power interruptionn
    GPIO_setAsOutputPin(RFID_CS_PORT, RFID_CS_PIN);
    RFID_CS_High();

    //buttons
    _pushButtonsInit();

    // Presence sensor
    ToF_Init();
    _idleTimerInit();

    //PWM for the buzzer
    _buzzerInit();

    // Initialize UART communication with ESP32
    ESP_Comm_Init();

    //to restore data from flash
    database_init();

    //display
    _graphicsInit();

    //motor Pins
    motor_init();

    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE); //start the idle timer
}


uint8_t insert_pin(){

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

    // Check for USER PIN
    int k;
    uint8_t pin_correct=0;

    for(k=0; k < MAX_TEMP_USERS; ++k) {
        if(activeTempUsers[k].active && strcmp(typed_pin_str, activeTempUsers[k].pin) == 0) {
            pin_correct = 1;
            printf("%c \n",activeTempUsers[k].pin[0]);                  //only to debug

            //to save the access data in db:
            dbstate = USER;
            add_log(dbstate, get_date_hour(), selected_pin_user);
            save_database();

            break; //break if correct
        }
    }


    if(pin_correct != 1){
    // Check for Admin PIN
        for(i=0; i<4; i++) {
            if(selected_pin_user[i] != saved_pin_admin[i]) {
                pin_correct = 0;
                //to save the access data in db:
                dbstate = DENIED;
                add_log(dbstate, get_date_hour(), selected_pin_user);
                save_database();

                break;
            }
            else
            {
                pin_correct = 2;

                if(i==3){           //this way i save admin access on db only if all 4 digits are right. it saves when the last digit is checked
                    dbstate = ADMIN;
                    add_log(dbstate, get_date_hour(), selected_pin_user);
                    save_database();
                }
            }
        }
    }
    return pin_correct;
}


void open_door(void){
    // - turn on motor
    printf("open door \n");
    moveMotor(360);
}
void close_door(void){
    printf("closing door \n");
    moveMotor(-360);
}



bool wait_RFID(void){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("PLEASE, USE RFID");

    uint32_t start_t = system_millis;
    while ((system_millis - start_t) < 4000){
        if(buttonB_pressed){ //if back button is pressed, go back
            buttonB_pressed=0;
            RFID_Disable();
            return false; //exit if a button is pressed
        }
    }

    if(!RFID_Enable()){
        goto FALSE;
    }

    uint8_t read_uid[10] = {0};
    uint8_t uidLength=10;
    bool read_status = 0;

    while(1){ //polling loop
        if(buttonA_pressed || buttonB_pressed){
            buttonA_pressed=0;
            buttonB_pressed=0;
        }

        if(RFID_ready) read_status = RFID_ReadTag(read_uid, &uidLength);

        if (read_status && (uidLength <= RFID_UID_LENGTH))
        {
            printf("Read RFID:");
            for(int i=uidLength; i>0;) printf("%" PRIu8, read_uid[--i]);

            //check for valid RFID
            while((uidLength--) > 0)
            {
                if(read_uid[uidLength]!=RFID_saved[uidLength]){
                    read_status = 0;
                    break;
                }
            }
            if(read_status) //rfid data is valid
            {

                RFID_Disable(); //restore button functionality and disables SPI
                _graphicsInit();
                display_string("valid RFID");
                delay_ms(2000);
                return true;
            }
        }
        else
        {
            goto FALSE;
        }
    }
    return true;

    /* Reset logic before exiting */
    FALSE:
    RFID_Disable();
    _graphicsInit();
    display_string("ERROR");
    delay_ms(2000);
    return false; //exit if a button is pressed
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

void menu_setup_wifi(void){
    display_menu_setup_wifi();
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

    ++error_pin;

    uint32_t t_start = system_millis;
    display_wrong_pin(error_pin);
    if (error_pin<MAX_PIN_TRIES) buzzerPWMgen(&WrongPin);
    else buzzerPWMgen(&LockOut);
    while(t_start-system_millis < 3000);

}


void block_access(void){
    display_block_access();
    delay_ms(3000);
}

void door_lock(){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("DOOR LOCKED");
    delay_ms(2000);
}



bool check_for_inputs(){
    if (buttonA_pressed || buttonB_pressed)
    {
        buttonB_pressed=0;
        buttonA_pressed=0;
        Timer_A_stop(TIMER_A2_BASE);

        return 1; //signal that an input was detected
    }
    else if (ToF_flag)
    {
        ToF_flag = 0; // safety measure if ToF has been re-triggered meanwhile

        if (ToF_validate_interrupt()){
            Timer_A_stop(TIMER_A2_BASE);
            ToF_disable(); // Disable ToF interrupt and change state

            return 1;
        }
    }
    return 0; // no interrupts were detected or ToF wasn't valid
}


void menu_last_access_log(int db_page){
    display_menu_last_access_log();     //it displays only the title
//    serial_print_db();

    display_db(db_page);                //to display only the correct page of the database
}

void menu_factory_reset(void){
    display_menu_factory_reset();
}

char* get_date_hour(){
    char buffer[15];
    int hour = 0, minute = 0, day = 1, month = 1;
    if(timeSynced){             //so only if the esp has sinced and sent the current time
        hour = RTC_C_getCalendarTime().hours;
        minute = RTC_C_getCalendarTime().minutes;
        day = RTC_C_getCalendarTime().dayOfmonth;
        month = RTC_C_getCalendarTime().month;
        sprintf(buffer, "%02d/%02d %02d:%02d", day, month, hour, minute);   //"%02d" specifies i want to stamp 2 character in any case, and to add a 0 if necessary (ex 7 -> 07)
    } else {
        sprintf(buffer, "--/-- --:--");         //if time data not available, print this
    }
    return buffer;
}

