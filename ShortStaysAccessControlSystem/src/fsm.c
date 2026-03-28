#include "fsm.h"

// int saved_pin_user[4] = {1,1,1,1}; no longer used!
int saved_pin_admin[4] = {9,9,9,9};
int selected_pin_user[4] = {0,0,0,0};

int error_pin = 0; //variable to count the number of wrong pin, when is equal to 3, block access

const uint16_t* current_results;

int32_t displayX = 64;
int32_t displayY = 64;


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
     //{STATE_SETUP_PIN, fn_menu_setup_pin}, no longer used
     {STATE_SETUP_WIFI, fn_menu_wifi},
     {STATE_FACTORY_RESET, fn_menu_fact_reset},
     {STATE_UNLOCK_DOOR, fn_menu_unlock_door},
     {STATE_BLOCK_PIN, fn_menu_block_pin},

     {STATE_WRONG_PIN, fn_WRONG_PIN},
     {STATE_BLOCK_ACCESS, fn_BLOCK_ACCESS},
     {STATE_WAIT_RESET_DOOR, fn_WAIT_RESET_DOOR},
     {STATE_AOD, fn_AOD},
     {STATE_SYNC_TIME, fn_SYNC_TIME}
};


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
    _PIRInit();
    _idleTimerInit();

    //PWM for the buzzer
    _buzzerInit();

    // Initialize UART communication with ESP32
    ESP_Comm_Init();

}


void insert_pin(void){

    bool number_pin_aquired;

    int x = 35; //position of selected digit on screen

    int i;
    //acquire 4 digit
    for(i=0;i<4;i++){
        number_pin_aquired=0;
        do{
            if (standby) return; // Exit if system goes to sleep

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

void door_lock(){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("DOOR LOCKED");
}


void wait_reset_door(void){

}

void go_to_idle(){
    display_string("Going to sleep...");
    //delay_ms(1500);
}


bool check_for_inputs(){
    if (PIR_flag)
    {
        PIR_flag = 0;
        I2C_write_reg8(SYSTEM_INTERRUPT_CLEAR, 0x01);
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
        if (!(P4->IE & BIT6)) // if the PIR interrupt isn't enabled
        {

            PIR_enable(); // enable the interrupt
            PIR_flag = 0; // safety measure if PIR has been retriggered meanwhile
        }
        return 0; // no interrupts were detected
    }

    PIR_disable(); // Disable PIR interrupt and change state
    PIR_flag = 0; // safety measure if PIR has been retriggered meanwhile

    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE); // restart the idle timer

    return 1; //signal that an input was detected
}



void fn_BOOT(void){
    _hwInit();
    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE);
    cur_state = STATE_SYNC_TIME;
}

void fn_SYNC_TIME(void){
    // Static flag to ensure the time request is sent to the ESP32 only once
    static bool req_sent = false;

    // Track when we last sent the request
    static uint32_t last_req_time = 0;

    // Timestamp to manage the state's timeout and first-run logic
    static uint32_t entry_time = 0;

    // Initialization: runs only the first time the FSM enters this state
    if (entry_time == 0) {
        entry_time = system_millis;
        // Setup display color and show the loading string.
        Graphics_setForegroundColor(&g_sContext, ClrBlack);
        display_string("SYNCING TIME...");
    }

    // CRITICAL: Keep restarting the hardware idle timer (A2) to prevent
    // the system from going into standby/sleep while waiting for the ESP32.
    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE);

    // SUCCESS CASE: The UART Interrupt has received a valid time packet
    // and initialized the hardware RTC.
    if (timeSynced) {
        req_sent = false;
        entry_time = 0; // Reset entry time for future re-synchronizations
        cur_state = STATE_AOD;  // Transition directly to Always On Display
        return;
    }

    // FALLBACK CASE: If 10 seconds pass without a response from the ESP32,
    // exit the sync state to prevent the system from hanging.
    if ((system_millis - entry_time) > 10000) {
        req_sent = false;
        entry_time = 0; // Resetta
        cur_state = STATE_AOD; // Forza l'uscita, la tastiera tornerà a funzionare
        return;
    }

    // TRANSMISSION LOGIC:
    // Send the first request immediately, then retry every 3000ms (3 seconds) if no response.
    if (!req_sent || ((system_millis - last_req_time) > 3000)) {
        requestRealTime();  // Send the REQ_TIME command via UART

        // Update timing trackers
        last_req_time = system_millis;
        req_sent = true;
    }
}

void fn_DOOR_LOCKED(void){
    static bool already_displayed = 0;

    if(already_displayed == 0){
        door_lock();
        already_displayed = 1;
    }

    if (standby) {
        standby = 0;
        cur_state= STATE_AOD;
    }

    if (check_for_inputs()) {
        already_displayed = 0;
        cur_state = STATE_INSERT_PIN;
    }
}



void fn_INSERT_PIN(void){
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("INSERT PIN");
    draw_grid();
    insert_pin();

    bool admin_pin_correct = 1;
    bool temp_pin_correct = 0;

    // Convert the integer array from the display into a string
    char typed_pin_str[5];
    snprintf(typed_pin_str, sizeof(typed_pin_str), "%d%d%d%d",
             selected_pin_user[0],
             selected_pin_user[1],
             selected_pin_user[2],
             selected_pin_user[3]);

    // --- 1. Check for USER PIN ---
    int i;
    for(i=0; i < MAX_TEMP_USERS; i++) {
        if(activeTempUsers[i].active && strcmp(typed_pin_str, activeTempUsers[i].pin) == 0) {
            temp_pin_correct = 1;
            break;
        }
    }

    // --- 2. Check if the User PIN was Correct ---
    if(temp_pin_correct) {
        error_pin = 0;
        cur_state = STATE_OPEN_DOOR;
    } else {
        // Fallback: Check for Admin PIN
        for(i=0; i<4; i++) {
            if(selected_pin_user[i] != saved_pin_admin[i]) {
                admin_pin_correct = 0;
                break;
            }
        }
        if (admin_pin_correct) {
            error_pin = 0;
            cur_state = STATE_WAIT_RFID;
        } else {
            cur_state = STATE_WRONG_PIN; // Increment error_pin in the next state [cite: 601]
        }
    }
}


void fn_OPEN_DOOR(void){
    open_door();
    TIMER_RESTART(TIMER_A2_BASE, TIMER_A_UP_MODE);
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
    case LAST_ACCESS_LOG:
            cur_state = STATE_LAST_ACCESS_LOG;
            break;
    // case SETUP_PIN no longer used
    case WIFI_SETUP:
        cur_state = STATE_SETUP_WIFI;
        break;
    case FACTORY_RESET:
        cur_state = STATE_FACTORY_RESET;
        break;
    case UNLOCK_DOOR:
        cur_state = STATE_UNLOCK_DOOR;
        break;
    case BLOCK_PIN:
        cur_state = STATE_BLOCK_PIN;
        break;
    default:
        cur_state = STATE_INSERT_PIN;
        break;
    }
}



void fn_AOD(void){
    // Stop the idle timer since we are already in the sleep/AOD state
    Timer_A_stop(TIMER_A2_BASE);

    // Check for any user interaction (buttons, joystick, or PIR sensor)
    if (check_for_inputs()) {
        cur_state = STATE_INSERT_PIN;
    }

    // Static variable to track the last drawn minute.
    // Initialized to -1 so it instantly draws the clock the first time it enters AOD.
    static int lastMinute = -1;

    // Fetch the current real time directly from the hardware RTC module
    RTC_C_Calendar now = RTC_C_getCalendarTime();

    // Refresh the clock ONLY when the minute actually changes
    if(now.minutes != lastMinute){
        Graphics_setForegroundColor(&g_sContext, ClrBlack);

        // Update the display with the current hours and minutes
        display_clock(now.hours, now.minutes);

        // Update the tracker
        lastMinute = now.minutes;
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

/* no longer used
void fn_menu_setup_pin(void){
    bool pins_equal = 0;

    Graphics_setForegroundColor(&g_sContext, ClrBlack);
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
*/

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

    // --- BACKGROUD ---
    if (newUartMessage) {
        processUartMessage();
    }

    // If TIME_SYNC_INTERVAL_MS ms (now 2h) have elapsed since the last sync, request the time to ESP32
    if (timeSynced && (system_millis - lastTimeSync > TIME_SYNC_INTERVAL_MS)) {
        lastTimeSync = system_millis;
        requestRealTime();
    }

    // --- FOREGROUND FSM ---
    if (cur_state < NUM_STATES)
    {
        if (standby == 1)
        {
            cur_state = STATE_DOOR_LOCKED;
            go_to_idle();
        }

        (*fsm[cur_state].state_function)();
    }
    else
    {
        // Handle invalid state
    }
}
