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

    //_SysTickInit((const uint32_t) 48000);

    //joystick
    _adcInit();
    _ADCtimerInit();

    //disable the CS gpio for RFID It might be
    // left low after a reset without power interruption
    GPIO_setAsOutputPin(RFID_CS_PORT, RFID_CS_PIN);
    GPIO_setOutputHighOnPin(RFID_CS_PORT, RFID_CS_PIN);


    _SysTickInit();
    system_millis = 0;

    //buttons
    _pushButtonsInit();

    // Presence sensor
    ToF_Init();
    _idleTimerInit();

    //PWM for the buzzer
    _buzzerInit();

    // Initialize UART communication with ESP32
    ESP_Comm_Init();

    //flash
    database_init();

    //motor Pins
    motor_init();

    //display
    _graphicsInit();
}



void reset_flags(){
    move_rectangle=0;
    buttonA_pressed = 0;
    buttonB_pressed = 0;
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
    // - turn on servo
    printf("open door \n");
    if(!myDb.isDoorOpen){   //open door only if is closed
        moveMotor(360);
        myDb.isDoorOpen = true;              //I save on flash the actual position of the door
        save_database();
    }

}

void close_door(void){
    printf("closing door \n");
    if(myDb.isDoorOpen){    //close door only if is open
        moveMotor(-360);
        myDb.isDoorOpen = false;            //I save on flash the actual position of the door
        save_database();
    }

}


bool wait_RFID(void){
    // clear previous set flags
    buttonA_pressed=0;
    buttonB_pressed=0;

    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_clearDisplay(&g_sContext);
        GrContextFontSet(&g_sContext, &g_sFontCmss16);
        Graphics_setForegroundColor(&g_sContext, ClrBlack);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) "USE RFID TAG",
                                    AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);

        Graphics_setForegroundColor(&g_sContext, ClrGray);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) "Press A to cancel",
                                    AUTO_STRING_LENGTH, 64, 60, OPAQUE_TEXT);

    if(!RFID_Enable()){
        goto ERROR;
    }

    uint8_t read_uid[10] = {0};
    uint8_t uid_len=0;
    bool read_status = 0;
    bool tag_valid =0;

    //polling loop
    while (1) {
        // Check for button A
        if (buttonA_pressed) {buttonA_pressed = 0; goto CANCEL;}

        // Try to read a tag
        if (RFID_ReadTag(read_uid, &uid_len))
        {
            // If tag found, check if it matches the saved UID length first
            tag_valid = (uid_len == RFID_UID_LENGTH);
            if (tag_valid)
            {
                for (int i = 0; i < RFID_UID_LENGTH; i++)
                {
                    if (read_uid[i] != RFID_saved[i])
                    {
                        tag_valid = 0;
                        break;
                    }
                }
            }

            if (tag_valid)
            {
                // Valid tag, success
                RFID_Disable();

                uint32_t t_start = system_millis;
                Graphics_setForegroundColor(&g_sContext, ClrGreen);
                display_string("VALID RFID");
                buzzerPWMgen(&CorrectRFID);
                while(system_millis - t_start < 1500);

                return true;
            }
            else
            {
                // Invalid tag
                RFID_Disable();

                uint32_t t_start = system_millis;
                Graphics_setForegroundColor(&g_sContext, ClrRed);
                display_string("WRONG RFID");
                buzzerPWMgen(&WrongPin);
                while(system_millis - t_start < 1500);

                return false;
            }
        }
        else {
            // No tag – short delay to avoid busy‑looping and reduce power
            delay_ms(50);
        }
    }

    /* Reset logic on error before exiting */
    ERROR:
        RFID_Disable();
        display_string("ERROR");
        delay_ms(2000);
        return false;

    /* Reset logic on cancel before exiting */
    CANCEL:
        RFID_Disable();
        display_string("CANCELLED");
        delay_ms(1500);
        return false;
}




bool block_RFID(void){
    if(!RFID_Enable()){
        goto ERROR;
    }

    uint8_t read_uid[10] = {0};
    uint8_t uid_len=0;
    bool read_status = 0;
    bool tag_valid =0;

    //polling loop
    while (1) {
        // Try to read a tag
        if (RFID_ReadTag(read_uid, &uid_len))
        {
            // If tag found, check if it matches the saved UID length first
            tag_valid = (uid_len == RFID_UID_LENGTH);
            if (tag_valid)
            {
                for (int i = 0; i < RFID_UID_LENGTH; i++)
                {
                    if (read_uid[i] != RFID_saved[i])
                    {
                        tag_valid = 0;
                        break;
                    }
                }
            }

            if (tag_valid)
            {
                // Valid tag, success
                RFID_Disable();

                uint32_t t_start = system_millis;
                Graphics_setForegroundColor(&g_sContext, ClrGreen);
                display_string("VALID RFID");
                buzzerPWMgen(&CorrectRFID);
                while(system_millis - t_start < 1500);

                return true;
            }
            else
            {
                // Invalid tag, show feedback but continue scanning
                RFID_Disable();

                uint32_t t_start = system_millis;
                Graphics_setForegroundColor(&g_sContext, ClrRed);
                display_string("WRONG RFID");
                buzzerPWMgen(&WrongPin);
                while(system_millis - t_start < 1500);
                return false;
            }
        }
        else {
            // No tag – short delay to avoid busy‑looping and reduce power
            delay_ms(50);
        }
    }

    /* Reset logic on error before exiting */
    ERROR:
        RFID_Disable();
        display_string("ERROR");
        delay_ms(2000);
        return false;
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


void menu_unlock_door(void){
    display_menu_unlock_door();
}



void wrong_pin(void){
    // - turn on LED
    // - make a sound to signal that the code is incorrect
    ++error_pin;

    display_wrong_pin(error_pin);
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
    printf("check_for_inputs: ToF_ready:%d, ToF_flag:%d\n", ToF_ready, ToF_flag);
    if (buttonA_pressed || buttonB_pressed)
    {
        buttonB_pressed=0;
        buttonA_pressed=0;
        ToF_flag=0;
        return true; //signal that an input was detected
    }

    if (ToF_flag)
    {
        ToF_flag=0;
        uint16_t range=0;
        uint8_t error=0;
        bool valid = vl53l0x_read_range_interrupt(&range, &error);

        // Only treat as "presence detected" if range is genuinely within threshold
        valid &= ((range <= VL53L0X_LOW_THRESH) && (range > 0));

        if(!valid){ // if it wasn't valid or in threshold, re-enable interrupt (disabled in the ISR)
            PORT(VL53L0X_INT_PORT)->IE |= ONE_HOT_BIT(VL53L0X_INT_PIN);
        }
        else
        {
            printf("check_for_inputs range: %" PRIu16 ", error: %" PRIu8 "\n", range, error);
        }


        return valid;
    }

    return false; // no inputs were detected or ToF wasn't valid
}


void ReconfigInterruptsForSleep(bool enable){
    if(enable)
    {
        SysTick_disableInterrupt();
        Interrupt_disableInterrupt(INT_TA3_N);  //(joystick)
        AODClockTimerInit();                    //reconfigure the idle timer as a 30s timer
        PORT(VL53L0X_INT_PORT)->IE  |= ONE_HOT_BIT(VL53L0X_INT_PIN);

    }
    else
    {
        SysTick_enableInterrupt();
        Interrupt_enableInterrupt(INT_TA3_N);
        _idleTimerInit();                       // reconfigure TA2 as idle timer
    }

    return;
}



void menu_last_access_log(int db_page){
    display_menu_last_access_log();     //it display only the title


    display_db(db_page);                //to display only the correct page of the database
}

void menu_factory_reset(void){
    display_menu_factory_reset();
}


char* get_date_hour(){
    static char buffer[15];
    int hour = 0, minute = 0, day = 1, month = 1;
    if(timeSynced){             //only if the esp has sinced and sent the current time
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

