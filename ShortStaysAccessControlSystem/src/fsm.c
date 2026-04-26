#include "fsm.h"


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

    bool pin_correct=0;

    pin_correct = insert_pin();

    // --- 2. Check if the User PIN was Correct ---
    if(pin_correct == 1) {
        error_pin = 0;
        cur_state = STATE_OPEN_DOOR;
    }
    else if (pin_correct == 2)
    {
            error_pin = 0;
            cur_state = STATE_WAIT_RFID;
    }
    else
    {
        cur_state = STATE_WRONG_PIN; // Increment error_pin in the next state
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
    // Static variable to track the last drawn minute.
    // Initialized to -1 so it instantly draws the clock the first time it enters AOD.
    static int lastMinute = -1;

    static bool unsynced_drawn = false; // Flag to ensure the placeholder is drawn only once per sleep cycle

    // Stop the idle timer since we are already in the sleep/AOD state
    Timer_A_stop(TIMER_A2_BASE);



    if (timeSynced) {
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
    else {
        // Check if the placeholder hasn't been drawn yet in the current AOD cycle
        if (!unsynced_drawn) {
            // Reset the minute tracker to force the display to update immediately, without waiting for the next minute
            lastMinute = -1;

            // Draw the unsynced placeholder on the screen
            display_string("-- : --");

            // Set the flag to true so the FSM skips this drawing block in future loop iterations
            unsynced_drawn = true;
        }
    }

    // Check for any user interaction (buttons, joystick, or ToF sensor)
    if (check_for_inputs()) {
        cur_state = STATE_INSERT_PIN;
        unsynced_drawn = false;
    }

    // The VL53L0X interrupt on P4.6 will wake the CPU automatically
    go_to_idle();
    PCM_gotoLPM0(); // driverlib call — blocks until any interrupt fires
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

    // Anti-drift: If TIME_SYNC_INTERVAL_MS ms (now 2h) have elapsed since the last sync, request the time to ESP32
    if (timeSynced) {
        if (system_millis - lastTimeSync > TIME_SYNC_INTERVAL_MS) {
            lastTimeSync = system_millis;
            requestRealTime();
        }
    }
    // Auto-Recovery: if the system is not SYNCED, try again every minute
    else {
        static uint32_t last_retry = 0;
        if (system_millis - last_retry > 60000) {
            last_retry = system_millis;
            requestRealTime();
        }
    }

    // --- FOREGROUND FSM ---
    if (cur_state < NUM_STATES)
    {
        if (standby == 1)
        {
            cur_state = STATE_DOOR_LOCKED;
        }

        (*fsm[cur_state].state_function)();
    }
    else
    {
        // Handle invalid state
    }
}
