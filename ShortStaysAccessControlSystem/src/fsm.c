#include "fsm.h"


int db_page = 1;


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
     {STATE_UNLOCK_DOOR, fn_menu_unlock_door},
     {STATE_RFID_REGISTER, fn_rfid_register},

     {STATE_WRONG_PIN, fn_WRONG_PIN},
     {STATE_BLOCK_ACCESS, fn_BLOCK_ACCESS},
     {STATE_WAIT_RESET_DOOR, fn_WAIT_RESET_DOOR},
     {STATE_AOD, fn_AOD},
     {STATE_SYNC_TIME, fn_SYNC_TIME}
};


void fn_BOOT(void){
    _hwInit();
    cur_state = STATE_SYNC_TIME;
}

void fn_SYNC_TIME(void){
    Timer_A_stop(TIMER_A2_BASE);
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

    // SUCCESS CASE and FALLBACK CASE: The UART Interrupt has received a valid time packet
    // and initialized the hardware RTC.
    // If 10 seconds pass without a response from the ESP32,
    // exit the sync state to prevent the system from hanging.
    if (timeSynced || ((system_millis - entry_time) > 10000)) {
        req_sent = false;
        entry_time = 0; // Reset entry time for future re-synchronizations

        cur_state = STATE_AOD;  // Transition directly to Always On Display
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
    Timer_A_stop(TIMER_A2_BASE);

    static bool already_displayed = 0;

    if(already_displayed == 0){
        door_lock();
        already_displayed = 1;
    }

    // Here the door is assumend locked since the previous one will be
    // executed at least once per state transition to fn_DOOR_LOCKED

    if (check_for_inputs()) // check if there is activity
    {
        already_displayed = 0;
        cur_state = STATE_INSERT_PIN;
    }
    else if (standby) // if no activity and the idle timer has fired go to aod
    {
        standby = 0;
        already_displayed = 0;
        cur_state= STATE_AOD;
    }

    return;
}



void fn_INSERT_PIN(void){
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("INSERT PIN");
    draw_grid();

    switch (insert_pin())
    {
        case 1: // USER pin detected
            error_pin = 0;

            uint32_t t_start = system_millis;
            display_string("Welcome user!");
            buzzerPWMgen(&CorrectPin);
            while(system_millis - t_start < 1500);

            cur_state = STATE_OPEN_DOOR;

            break;

        case 2: // ADMIN pin detected
            error_pin = 0;

            t_start = system_millis;
            display_string("Welcome admin!");
            buzzerPWMgen(&CorrectPin);
            while(system_millis - t_start < 1500);

            cur_state = STATE_WAIT_RFID;

            break;

        default:
            cur_state = STATE_WRONG_PIN;

            break;
    }

    return;
}


void fn_OPEN_DOOR(void){
    Timer_A_stop(TIMER_A2_BASE);

    uint32_t t_start = system_millis;
    display_door_open();
    buzzerPWMgen(&CorrectPin);
    while(system_millis - t_start < 1500);

    open_door();

    cur_state = STATE_DOOR_LOCKED;
}


void fn_WAIT_RFID(void){

    Timer_A_stop(TIMER_A2_BASE);
    reset_flags(); //clear input actions

    if (wait_RFID()) {
        buzzerPWMgen(&CorrectRFID);
        cur_state = STATE_ADMIN_MENU;
    }
    else
    {
        cur_state = STATE_INSERT_PIN;
        //cur_state = STATE_ADMIN_MENU;
    }
    return;
}



void fn_ADMIN_MENU(void){
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);

    int selected_function;
    selected_function = admin_menu();

    //decide to which state of admin menu go
    switch(selected_function){
    case LAST_ACCESS_LOG:
            db_page=1;                              //when you enter the db, the first page is shown
            cur_state = STATE_LAST_ACCESS_LOG;
            break;
    case UNLOCK_DOOR:
        cur_state = STATE_UNLOCK_DOOR;
        break;
    case RFID_REGISTER:
        cur_state = STATE_RFID_REGISTER;
        break;
    default:
        cur_state = STATE_INSERT_PIN;
        break;
    }

    //stops the idle timer so the admin can work slowly
    Timer_A_stop(TIMER_A2_BASE);
}


void fn_WRONG_PIN(void){
    Timer_A_stop(TIMER_A2_BASE);

    wrong_pin(); //show an error message on display

    if(error_pin<MAX_PIN_TRIES){
        uint32_t start_t=system_millis;
        buzzerPWMgen(&WrongPin);
        while((system_millis-start_t)<3000);
        cur_state = STATE_INSERT_PIN;
    }else if(error_pin==MAX_PIN_TRIES){
        error_pin = 0; //pin wrong for max tries, reset counter of errors
        uint32_t start_t=system_millis;
        buzzerPWMgen(&LockOut);
        while((system_millis-start_t)<3000);
        cur_state = STATE_BLOCK_ACCESS;
    }
}



void fn_BLOCK_ACCESS(void){
    Timer_A_stop(TIMER_A2_BASE);

    printf("Access blocked \n");
    block_access();
    cur_state = STATE_WAIT_RESET_DOOR;
}



void fn_WAIT_RESET_DOOR(void){
    /*NOTE: this function never goes to AOD (sleep) */
    Timer_A_stop(TIMER_A2_BASE);

    printf("Wait door to be reset \n");
    if(wait_RFID()) cur_state=STATE_INSERT_PIN;
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
        unsynced_drawn = false;

        ToF_disable(); // Disable ToF interrupt and change state

        cur_state = STATE_INSERT_PIN;
    }
    else // no input was received
    {
        // if the ToF isn't scanning, enable it
        if(!ToF_ready) ToF_enable();
        // The VL53L0X interrupt on P4.6 will wake the CPU automatically
        // TODO: set a 30 sec interrupt to wake the cpu and increment the clock.
        // TODO: disable unnecessary interrupts so cpu isn't woke up
        PCM_gotoLPM0(); // is a blocking call: the CPU halts execution at this instruction and only resumes when an interrupt fires.
    }
}



//Functions of admin menu
// --------------------------------------------- //

void fn_menu_lal(void){
    //int dp_page;                  //this is defined in the top of the file!
    const uint16_t* current_results;
    bool menu_lal_active = 1;
    int tmp;
    menu_last_access_log(db_page);         //to display the first page of db

    while(menu_lal_active){     //until i press buttonB, i stay in lal database and i can use the joystick

        //get results from joystick
        current_results = get_results_buffer();
        //if timer of joystick finished, give number of page selected
        if(data_aquired()){
           tmp = db_page_selected(current_results[0], current_results[1], calc_num_pages_db(return_number_count()), db_page);

           if(tmp != db_page){  //i re-screen all infos only if i change page
               db_page = tmp;
               menu_last_access_log(db_page);
           }
        }
        if(buttonB_pressed){
                buttonB_pressed=0;
                menu_lal_active=0;
        }
    }
    cur_state = STATE_ADMIN_MENU;

}



void fn_menu_unlock_door(void){
    menu_unlock_door();

    //move servo (same function of when opening door)

    if(buttonB_pressed){
        buttonB_pressed=0;
        cur_state = STATE_ADMIN_MENU;
    }
}


void fn_rfid_register(void){

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
        static State_t prev_state=0; // initializes only once

        if (standby == 1)
        {
            cur_state = STATE_DOOR_LOCKED;
        }

        if (cur_state != prev_state) { //if state has changed
            reset_flags(); //clear input actions
            Timer_A_clearTimer(TIMER_A2_BASE); //clear timer

            //update the prev_state
            prev_state=cur_state;
        }

        //executes the state function
        (*fsm[cur_state].state_function)();
    }
    else
    {
        // Handle invalid state
    }
}
