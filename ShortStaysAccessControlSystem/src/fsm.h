#ifndef FSM_H
#define FSM_H

#include "msp.h"
#include <stdio.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include "fsm_helpers.h"
#include "database.h"

#define MOVE_ON_MENU 0  //rectangle on display move on admin menu
#define MOVE_ON_GRID 1  //rectangle on display move on grid numbers

/*
 * This header file defines the states of the system, and related data types
*/

typedef enum{
    STATE_BOOT,
    STATE_DOOR_LOCKED,
    STATE_INSERT_PIN,
    STATE_OPEN_DOOR,
    STATE_WAIT_RFID, //after the admin insert the pin, wait for the RFID to access menu
                     //we should consider the case in which the RFID doesn't arrive (maybe a timer)
    STATE_ADMIN_MENU, //there must be other states for each functionality

    STATE_LAST_ACCESS_LOG,
    STATE_UNLOCK_DOOR,
    STATE_RFID_REGISTER,

    STATE_WRONG_PIN,
    STATE_BLOCK_ACCESS,
    STATE_WAIT_RESET_DOOR, //after this return to STATE_DOOR_LOCKED o STATE_INSERT_PIN
    STATE_AOD,
    NUM_STATES
}State_t;


typedef struct{
    State_t state;
    void (*state_function)(void);
} StateMachine_t;


void fn_BOOT(void);
void fn_DOOR_LOCKED(void);

void fn_DRAW_GRID(void);

void fn_INSERT_PIN(void);
void fn_OPEN_DOOR(void);
void fn_WAIT_RFID(void);
void fn_ADMIN_MENU(void);
void fn_WRONG_PIN(void);
void fn_BLOCK_ACCESS(void);
void fn_WAIT_RESET_DOOR(void);
void fn_AOD(void);

void fn_menu_lal(void);
void fn_menu_unlock_door(void);
void fn_rfid_register(void);

// Function to run in the loop
void FSM_Run(void);


#endif
