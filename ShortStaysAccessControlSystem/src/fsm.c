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
     {STATE_MENU_OPEN, fn_menu_open},
     {STATE_MENU_CLOSE, fn_menu_close},
     {STATE_MENU_DATABASE_WIPE, fn_wipe_database},

     {STATE_WRONG_PIN, fn_WRONG_PIN},
     {STATE_WAIT_RESET_DOOR, fn_WAIT_RESET_DOOR},
     {STATE_AOD, fn_AOD}
};


void fn_BOOT(void){
    _hwInit();
    cur_state = STATE_DOOR_LOCKED;
}


void fn_DOOR_LOCKED(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;
    Timer_A_clearTimer(TIMER_A2_BASE);

    door_lock();

    cur_state= STATE_AOD;

    return;
}



void fn_INSERT_PIN(void){
    uint32_t t_start = 0;
    Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    display_string("INSERT PIN");
    delay_ms(200);
    draw_grid();

    switch (insert_pin())
    {
        case 1: // USER pin detected
            error_pin = 0;

            t_start = system_millis;
            Graphics_clearDisplay(&g_sContext);
            GrContextFontSet(&g_sContext, &g_sFontCmss16);
            Graphics_setForegroundColor(&g_sContext, ClrGreen);
            Graphics_drawStringCentered(&g_sContext, (int8_t *) "PIN Correct",
                                                AUTO_STRING_LENGTH,
                                                64, 60,
                                                OPAQUE_TEXT);
            Graphics_setForegroundColor(&g_sContext, ClrBlack);
                        Graphics_drawStringCentered(&g_sContext, (int8_t *) "Welcome user!",
                                                            AUTO_STRING_LENGTH,
                                                            64, 80,
                                                            OPAQUE_TEXT);
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
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    uint32_t t_start = system_millis;
    display_door_open();

    open_door();

    delay_ms(5000);

    display_door_closed();

    close_door();

    cur_state = STATE_DOOR_LOCKED;
}


void fn_WAIT_RFID(void){

    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;
    reset_flags(); //clear input actions

    if (wait_RFID()) {
        cur_state = STATE_ADMIN_MENU;
    }
    else
    {
        cur_state = STATE_INSERT_PIN;
    }
    return;
}



void fn_ADMIN_MENU(void){
    //stops the idle timer so the admin can work without interruption
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    int selected_function;
    selected_function = admin_menu();


    //decide to which state of admin menu go
    switch(selected_function){
        case -2:
            cur_state = STATE_INSERT_PIN;
            break;
        case LAST_ACCESS_LOG:
                db_page=1;                              //when you enter the db, the first page is shown
                cur_state = STATE_LAST_ACCESS_LOG;
                break;
        case OPEN_DOOR:
            cur_state = STATE_MENU_OPEN;
            break;
        case CLOSE_DOOR:
            cur_state = STATE_MENU_CLOSE;
            break;
        case DATABASE_WIPE:
                cur_state = STATE_MENU_DATABASE_WIPE;
                break;
        default: //remain in the same sttae
            cur_state = STATE_ADMIN_MENU;
            break;
        }
}


void fn_WRONG_PIN(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    wrong_pin(); //show an error message on display

    if(error_pin<MAX_PIN_TRIES)
    {
        uint32_t start_t=system_millis;
        buzzerPWMgen(&WrongPin);
        while((system_millis-start_t)<2000);
        cur_state = STATE_INSERT_PIN;
    }
    else if(error_pin==MAX_PIN_TRIES)
    {
        error_pin = 0; //pin wrong for max tries, reset counter of errors
        uint32_t start_t=system_millis;
        buzzerPWMgen(&LockOut);
        while((system_millis-start_t)<2000);
        cur_state = STATE_WAIT_RESET_DOOR;
    }
}



void fn_WAIT_RESET_DOOR(void){
    /*NOTE: this function never goes to AOD (sleep) */
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    printf("Access blocked \n");

    myDb.userAccessBlocked = true;      //in flash is stored that user access is blocked
    save_database();

    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_clearDisplay(&g_sContext);
        GrContextFontSet(&g_sContext, &g_sFontCmss16);
        Graphics_setForegroundColor(&g_sContext, ClrRed);
                Graphics_drawStringCentered(&g_sContext, (int8_t *) "ACCESS BLOCKED",
                                            AUTO_STRING_LENGTH, 64, 40, OPAQUE_TEXT);
        Graphics_setForegroundColor(&g_sContext, ClrBlack);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) "USE RFID TAG",
                                    AUTO_STRING_LENGTH, 64, 60, OPAQUE_TEXT);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) "TO UNLOCK",
                                            AUTO_STRING_LENGTH, 64, 80, OPAQUE_TEXT);

    if(block_RFID()) {
        myDb.userAccessBlocked = false;             //to store in flash that user access is no longer blocked
        save_database();
        cur_state=STATE_INSERT_PIN;
    }
}



void fn_AOD(void){

    // Stop the idle timer since we are already in the sleep/AOD state
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    //variable to let clock redraw in case the minute hasn't passed and coming from a different state like door locked
    static uint_fast8_t redraw =1;

    static uint32_t last_sync_req = 0;

    // Check if the user access is locked, if yes go to rfid validation
    if(myDb.userAccessBlocked){
        ToF_disable(); // Disable ToF interrupt and change state
        redraw = 1;

        cur_state = STATE_WAIT_RESET_DOOR;

        return;
    }

    // Check for any user interaction (buttons, joystick, or ToF sensor)
    if (check_for_inputs()) {
        ToF_disable(); // Disable ToF interrupt and change state
        redraw = 1;

        cur_state = STATE_INSERT_PIN;

        return;
    }

    // no input was received
    // if the ToF isn't scanning, enable it
    if(!ToF_ready) {
        ToF_enable();
    }

    // 1. Update clock display (if time synced)
    if (timeSynced) {
        // Fetch the current real time directly from the hardware RTC module since it's has been synced once
        RTC_C_Calendar now = RTC_C_getCalendarTime();

        static uint_fast8_t minutes=0xFF; //set to 255 so it's not reachable by RTC_C_getCalendarTime()

        if(now.minutes != minutes || redraw){ //sync only if a different minute is available
            // Refresh the clock
            Graphics_setForegroundColor(&g_sContext, ClrBlack);
            minutes = now.minutes;
            display_clock(now.hours, now.minutes);

            redraw = 0;
        }
    }
    else
    {
        if(redraw) {display_string("-- : --"); redraw = 0;}

        // Sync request
        if((system_millis - last_sync_req) > 4000){
            last_sync_req = system_millis;
            requestRealTime();   // send "REQ_TIME:0" to ESP32
        }
    }

    // if meanwhile an input is received, skip sleep and rerun fn_AOD
    if (ToF_flag || buttonA_pressed || buttonB_pressed) return;

    ReconfigInterruptsForSleep(true);

    PCM_gotoLPM0(); // is a blocking call: the CPU halts execution at this instruction and only resumes when an interrupt fires.

    // If the wake was due to the 30s timer (standby=1), we know it's exactly 30000 ms.
    // For other wakes, use elapsed_ms to adjust system_millis.
    if (standby) // Woke by TA2 interrupt: exactly 30 seconds
    {
        system_millis += 30000;
    }
    else // Woke by button or ToF: add to the sys millis the actual elapsed time
    {
        // Convert to milliseconds: each tick = (1000 / (ACLK/divider)) ms
        // ACLK is approximately 9400 Hz, divider = 64 -> 146.875 Hz -> 6.8085 ms/tick
        uint32_t elapsed_ms = (uint32_t) Timer_A_getCounterValue(TIMER_A2_BASE) * 68 / 10;
        system_millis += elapsed_ms;
    }

    ReconfigInterruptsForSleep(false);
    standby=0; //triggered by 30s timer because it shares IRQ with the idle timer

    return;

}



//Functions of admin menu
// --------------------------------------------- //

void fn_menu_lal(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

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
           printf("return_number_count = %d \n",return_number_count());
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



void fn_menu_open(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    //add check databse and open door:
    display_door_open();
    open_door();

    cur_state = STATE_ADMIN_MENU;
}


void fn_menu_close(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    //add check databse and close door:
    display_door_closed();
    close_door();

    cur_state = STATE_ADMIN_MENU;
}

void fn_wipe_database(void){
    Timer_A_stopTimer(TIMER_A2_BASE);
    standby = 0;

    //add wipe database

    cur_state = STATE_ADMIN_MENU;
}

// ---------------------------------------------//
// Function to run in the main
void FSM_Run(void){

    // Anti-drift: If TIME_SYNC_INTERVAL_MS ms (now 2h) have elapsed since the last sync, request the time to ESP32
    if (timeSynced) {
        if (system_millis - lastTimeSync > TIME_SYNC_INTERVAL_MS) {
            lastTimeSync = system_millis;
            requestRealTime();
        }
    }

    // --- FOREGROUND FSM ---
    if (cur_state < NUM_STATES)
    {
        static State_t prev_state= STATE_BOOT; // initializes only once

        if (standby == 1)
        {
            standby = 0;
            cur_state = STATE_DOOR_LOCKED;
        }

        if (cur_state != prev_state) { //if state has changed
            if(prev_state != STATE_AOD) reset_flags(); //clear input actions if not coming from AOD
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
