#ifndef FSM_HELPERS_H
#define FSM_HELPERS_H

#include <stdint.h>
#include <stdbool.h>

#include "joystick.h"
#include "joystick.h"
#include "display.h"
#include "push_button.h"
#include "sensors.h"
#include "timers.h"
#include "irqHandlers.h"
#include "buzzer.h"
#include "comm_esp.h"
#include "database.h"


extern uint16_t error_pin;
extern uint8_t saved_pin_admin[4];




void _hwInit(void);

void reset_flags(void);

uint8_t insert_pin(void);
void open_door(void);
void close_door(void);
bool wait_RFID(void);
int admin_menu(void);

void menu_setup_pin(void);
void menu_setup_wifi(void);
void menu_unlock_door(void);
void menu_block_pin(void);

void wrong_pin(void);
void last_pin(void);
void block_access(void);

bool check_for_inputs();
void door_lock();

void menu_factory_reset(void);
void menu_last_access_log(int db_page);

#endif
