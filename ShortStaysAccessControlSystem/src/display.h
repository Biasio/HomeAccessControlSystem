#ifndef DISPLAY_H
#define DISPLAY_H

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "Crystalfontz128x128_ST7735.h"
#include <stdio.h>
#include <inttypes.h>
#include "timers.h"


#define RECTANGLE_SHIFT_X_ON_GRID 30
#define RECTANGLE_SHIFT_Y_ON_GRID 22
#define RECTANGLE_SHIFT_ON_MENU 30

typedef struct {
    int x;
    int y;
} Point;


typedef struct {
    int pos_x1;
    int pos_x2;
    int pos_y1;
    int pos_y2;
}Rectangle;


//used to access the array GRID_POINTS easily
typedef enum {
    NUM1, NUM2, NUM3, NUM4, NUM5, NUM6, NUM7, NUM8, NUM9, VOID_LEFT, NUM0, VOID_RIGHT, NUM_POINTS
} PointIndex;

//functions inside admin menu
typedef enum {
    LAST_ACCESS_LOG,
    //SETUP_PIN, no longer used
    WIFI_SETUP,
    FACTORY_RESET,
    UNLOCK_DOOR,
    BLOCK_PIN,
    NUM_FUNCTION
} enum_menu_functions;


//variable used to define the display
Graphics_Context g_sContext;

// Initialize graphic context
void _graphicsInit();

// Draw grid of numbers
void draw_grid(void);

// Draw admin menu
void draw_admin_menu(bool screen_number);

int number_selected(void);
int display_function_selected(void);
int db_page_selected(uint16_t x, uint16_t y, int numElements, int currentPage);              //i give it the number of elements in the db, it returns the page selected by the joystick

void draw_rectangle(bool new_pos, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void move_rectangle_on_display( uint16_t x, uint16_t y, bool grid_on);

void display_menu_last_access_log(void);
void display_menu_setup_pin(void);
void display_menu_setup_wifi(void);
void display_menu_factory_reset(void);
void display_menu_unlock_door(void);
void display_menu_block_pin(void);

void display_clock(int hour, int minute);
void display_door_open(void);
void display_wait_RFID(void);
void display_wrong_pin(int error_pin);
void display_block_access(void);

void display_string(const char* string);

#endif
