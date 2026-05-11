#include "display.h"

void highlight_selected_menu_item(void);

//define first position for the rectangle used to select the numbers
Rectangle sel_rectangle_on_grid = {22, 48, 34, 52};

//define first position of rectangle for selection in admin menu
Rectangle sel_rectangle_on_admin_menu = {2, 126, 37, 67};

bool move_on_menu = 0; //used to decide the color of the rectangle in base of the type of screen (grid or menu)
bool first_screen = 1; //in the admin menu you are in the first screen (first 3 entries)


//these points are used to select the numbers on the grid
//at each point corresponds a number
Point GRID_POINTS[] = {
     // P1 | P2 | P3
     { 35, 43 }, { 65, 43 }, { 95, 43 },
     // P4 | P5 | P6
     { 35, 65 }, { 65, 65 }, { 95, 65 },
     // P7 | P8 | P9
     { 35, 87 }, { 65, 87 }, { 95, 87 },
     // VOID | P0 | VOID
     { 35, 109 }, { 65, 109 }, { 95, 109 }
};

// Points used to select a function in admin menu
const Point MENU_POINTS[] = {
     // P1 | P2 | P3
     { 50, 50 },
     { 50, 80 },
     { 50, 110 },
};

// Array that stores the functions used
char* function_strings[] = {
    "LAST ACCESS LOG",
    //"SETUP PIN", no longer used
    //"SETUP WIFI",
    //"FACTORY RESET",
    "UNLOCK DOOR",
    "RFID_REGISTER"
    //"BLOCK PIN INSERT"
};

// Initialize display
void _graphicsInit()
{
    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    Graphics_setBackgroundColor(&g_sContext, ClrWhite);
    GrContextFontSet(&g_sContext, &g_sFontCmss36);
    Graphics_clearDisplay(&g_sContext);
}

// Draw grid of numbers
void draw_grid(void)
{
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss18);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);

    // Initial position to draw the lines
    int start_x = 20;
    int end_x = 110;
    int start_y = 32;
    int end_y = 120;

    int i;

    //the lines of the grid are shifted of the same as when the rectangle is shifted
    for(i=start_y; i<=end_y; i+=RECTANGLE_SHIFT_Y_ON_GRID){
        Graphics_drawLineH(&g_sContext, start_x, end_x, i);
    }

    for(i=start_x; i<=end_x; i+=RECTANGLE_SHIFT_X_ON_GRID){
        Graphics_drawLineV(&g_sContext, i, start_y, end_y);
    }

    // Draw the numbers on the grid
    char string[2];
    int x, y;
    i = 1;

    for(y=start_y+9; y<=end_y-11; y+=RECTANGLE_SHIFT_Y_ON_GRID){
        for(x=start_x+15; x<=end_x-15; x+=RECTANGLE_SHIFT_X_ON_GRID){
            if (i <= 9) {
                sprintf(string, "%d", i);
            } else if (i == 11) {
                sprintf(string, "0");
            } else {
                string[0] = '\0';
            }
            GRID_POINTS[i-1].x = x;
            GRID_POINTS[i-1].y = y;
            ++i;

            Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                        AUTO_STRING_LENGTH,
                                        x, y,
                                        OPAQUE_TEXT);
        }
    }
    //draw rectangle in his initial position (NUMBER 1)
    draw_rectangle(1, sel_rectangle_on_grid.pos_x1, sel_rectangle_on_grid.pos_y1, sel_rectangle_on_grid.pos_x2, sel_rectangle_on_grid.pos_y2);


    //show on the upper part of the display the number selected
    //initially they are empty
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);

    Graphics_drawLineH(&g_sContext, 30, 39, 25);
    Graphics_drawLineH(&g_sContext, 50, 59, 25);
    Graphics_drawLineH(&g_sContext, 70, 79, 25);
    Graphics_drawLineH(&g_sContext, 90, 99, 25);

}

// Draw the admin menu
void draw_admin_menu(bool screen_number){

    Graphics_clearDisplay(&g_sContext);

    int32_t x_string = 64;
    int32_t y_string = 20;
    int32_t y_line = 37;
    int32_t start_v_line = y_line;
    int32_t start_x_line = 2;
    int32_t end_x_line = 126;

    int start, end;

    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    GrContextFontSet(&g_sContext, &g_sFontCmss12);
     //in case of more than two pages, use an int to count the number of pages
     if(screen_number){ //first page
         first_screen=1;
         start = 0;
         end = 3;

         Graphics_drawStringCentered(&g_sContext, (int8_t *) "Page 1/1",
                                        AUTO_STRING_LENGTH,
                                        23, 5,
                                        OPAQUE_TEXT);
     }

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "ADMIN MENU",
                                        AUTO_STRING_LENGTH,
                                        x_string, y_string,
                                        OPAQUE_TEXT);
    y_string+=30;

    Graphics_setForegroundColor(&g_sContext, ClrBlack);



    int i;
    for(i=start; i<end; i++){
        Graphics_drawStringCentered(&g_sContext, (int8_t *) function_strings[i],
                                    AUTO_STRING_LENGTH,
                                    x_string, y_string,
                                    OPAQUE_TEXT);
        Graphics_drawLineH(&g_sContext, start_x_line, end_x_line, y_line);
        y_string+=30;
        y_line+=30;
    }

    Graphics_drawLineH(&g_sContext, start_x_line, end_x_line, y_line);

    Graphics_drawLineV(&g_sContext, start_x_line, start_v_line, y_line);
    Graphics_drawLineV(&g_sContext, end_x_line, start_v_line, y_line);

    sel_rectangle_on_admin_menu.pos_x1 = start_x_line;
    sel_rectangle_on_admin_menu.pos_y1 = start_v_line;
    sel_rectangle_on_admin_menu.pos_x2 = end_x_line;
    sel_rectangle_on_admin_menu.pos_y2 = start_v_line+30;

    draw_rectangle(1, sel_rectangle_on_admin_menu.pos_x1, sel_rectangle_on_admin_menu.pos_y1, sel_rectangle_on_admin_menu.pos_x2, sel_rectangle_on_admin_menu.pos_y2);

}

/******************************************************************************
 * FUNCTION: Draws a selection rectangle in a new position or erases an old one.
 * DETAILS: This function abstracts the process of drawing a rectangle
 * (using ClrRed) or erasing a rectangle (using ClrBlack/ClrWhite)
 * depending on the state of the static/global variable
 * 'move_on_menu' and the function's 'new_pos' flag.
 *
 *   The choice of erasure color (Black or White) is context-dependent,
 * implying the function handles two different visual states:
 * 1. Menu Mode (Erased with Black)
 * 2. Grid Mode (Erased with White)
 *
 * INPUT:
 *  - new_pos:  Boolean flag. If true (1), the rectangle is drawn
 * in the new position (highlighted/Red). If false (0),
 * the rectangle is drawn over the old position using
 * the background color (erased).
 *  - x1         X-coordinate of the top-left corner.
 *  - y1         Y-coordinate of the top-left corner.
 *  - x2         X-coordinate of the bottom-right corner.
 *  - y2         Y-coordinate of the bottom-right corner.
 *
 ******************************************************************************/

void draw_rectangle(bool new_pos, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
    //if move_on_menu=1 and new_pos=0 cancel the old rectangle on menu
    //if move_on_menu=0 and new_pos=0 cancel the old rectangle on grid
    //if new_pos=1 draw the rectangle in the updated position
    if(move_on_menu & !new_pos) Graphics_setForegroundColor(&g_sContext, ClrBlack);
    else if(!move_on_menu & !new_pos) Graphics_setForegroundColor(&g_sContext, ClrWhite);
    else Graphics_setForegroundColor(&g_sContext, ClrRed);

    const Graphics_Rectangle rectangle = {x1, y1, x2, y2};
    Graphics_drawRectangle(&g_sContext, &rectangle);
}


void move_rectangle_right(Rectangle* rectangle, int16_t shift){

        draw_rectangle(0, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);

        rectangle->pos_x1 += shift;
        rectangle->pos_x2 += shift;

        draw_rectangle(1, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);
}

void move_rectangle_left(Rectangle* rectangle, int16_t shift){

        draw_rectangle(0, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);

        rectangle->pos_x1 -= shift;
        rectangle->pos_x2 -= shift;

        draw_rectangle(1, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);
}

void move_rectangle_up(Rectangle* rectangle, int16_t shift){

        draw_rectangle(0, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);

        rectangle->pos_y1 -= shift;
        rectangle->pos_y2 -= shift;

        draw_rectangle(1, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);
}

void move_rectangle_down(Rectangle* rectangle, int16_t shift){

        draw_rectangle(0, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);

        rectangle->pos_y1 += shift;
        rectangle->pos_y2 += shift;

        draw_rectangle(1, rectangle->pos_x1, rectangle->pos_y1, rectangle->pos_x2, rectangle->pos_y2);
}


/*******************************************************************************
 * @brief       Handles all display and selection rectangle movements based on
 * joystick input for both Grid and Menu interfaces.
 *
 * @details     Reads raw joystick X and Y values to determine user intent.
 * It first checks the 'grid_on' flag to branch into Grid Mode
 * (move_on_menu = 0) or Menu Mode (move_on_menu = 1).
 * * In Menu Mode, it handles three actions:
 * 1. Initial Text Coloring: Highlights the currently selected menu item.
 * 2. Vertical Scrolling: Clears old text and moves the rectangle up/down.
 * 3. Horizontal Paging: Changes the 'first_screen' flag and redraws the menu.
 *
 * @param[in]   x          Raw X-axis value from the joystick.
 * @param[in]   y          Raw Y-axis value from the joystick.
 * @param[in]   grid_on    Flag: true for Grid Mode, false for Menu Mode.
 *
 * @return      void
 *
 * @note        Relies on the global flag 'first_screen' and several global/static
 * rectangle structs and string arrays (e.g., 'function_strings').
 ******************************************************************************/

void move_rectangle_on_display( uint16_t x, uint16_t y, bool grid_on) {

    const int RIGHT = 12000;
    const int LEFT = 4000;
    const int UP = 12000;
    const int DOWN = 4000;

    //move rectangle on grid
    if(grid_on)
    {
       move_on_menu = 0;

       const int GRID_UPPER_LIMIT_X = 80;
       const int GRID_LOWER_LIMIT_X = 40;
       const int GRID_UPPER_LIMIT_Y = 90;
       const int GRID_LOWER_LIMIT_Y = 40;

       if(x>RIGHT) {
           if(sel_rectangle_on_grid.pos_x1 < GRID_UPPER_LIMIT_X) {
               move_rectangle_right(&sel_rectangle_on_grid, RECTANGLE_SHIFT_X_ON_GRID);
               Timer_A_clearTimer(TIMER_A2_BASE);
           }
       }
       else if(x<LEFT)
       {
           if(sel_rectangle_on_grid.pos_x1 > GRID_LOWER_LIMIT_X) {
               move_rectangle_left(&sel_rectangle_on_grid, RECTANGLE_SHIFT_X_ON_GRID);
               Timer_A_clearTimer(TIMER_A2_BASE);
           }
       }
       else if(y>UP)
       {
           if(sel_rectangle_on_grid.pos_y1 > GRID_LOWER_LIMIT_Y){
               move_rectangle_up(&sel_rectangle_on_grid, RECTANGLE_SHIFT_Y_ON_GRID);
               Timer_A_clearTimer(TIMER_A2_BASE);
           }
       }
       else if(y<DOWN)
       {
           if(sel_rectangle_on_grid.pos_y1 < GRID_UPPER_LIMIT_Y) {
               move_rectangle_down(&sel_rectangle_on_grid, RECTANGLE_SHIFT_Y_ON_GRID);
               Timer_A_clearTimer(TIMER_A2_BASE);
           }
       }
    }
    else //move rectangle on menu
    {
       move_on_menu = 1;

       const int UPPER_LIMIT = 80;
       const int LOWER_LIMIT = 40;

       int32_t x_string = 64;
       int32_t y_string = 50;
       int8_t start = 0;
       int8_t end = 2;
       int i;
       int8_t string_offset = (first_screen) ? 0 : 3;

       //up and down to scroll in the menu
       if(y>UP) {
           Timer_A_clearTimer(TIMER_A2_BASE);
           // 1. Clear ALL currently drawn menu strings (Draw them in black)
           for(i=start; i<=end; i++){
               int8_t string_index = i + string_offset;
               Graphics_setForegroundColor(&g_sContext, ClrBlack);
               Graphics_drawStringCentered(&g_sContext, (int8_t *) function_strings[string_index],
                                             AUTO_STRING_LENGTH,
                                             x_string, y_string,
                                             OPAQUE_TEXT);
               y_string+=30;
           }
           // 2. Move the selection rectangle UP
          if(sel_rectangle_on_admin_menu.pos_y1>LOWER_LIMIT) {
            move_rectangle_up(&sel_rectangle_on_admin_menu, RECTANGLE_SHIFT_ON_MENU);
          }
       }
       if(y<DOWN) {
           Timer_A_clearTimer(TIMER_A2_BASE);
           // 1. Clear ALL currently drawn menu strings (Draw them in black)
           for(i=start; i<=end; i++){
               int8_t string_index = i + string_offset;
               Graphics_setForegroundColor(&g_sContext, ClrBlack);
               Graphics_drawStringCentered(&g_sContext, (int8_t *) function_strings[string_index],
                                             AUTO_STRING_LENGTH,
                                             x_string, y_string,
                                             OPAQUE_TEXT);
               y_string+=30;
           }
           // 2. Move the selection rectangle DOWN
           if(sel_rectangle_on_admin_menu.pos_y1<UPPER_LIMIT) {
              move_rectangle_down(&sel_rectangle_on_admin_menu, RECTANGLE_SHIFT_ON_MENU);
          }
       }

      // --- FINAL STEP: Draw the NEWLY selected item in RED --- //
      // This must run after any movement (scroll or page change)

      highlight_selected_menu_item();
    }
}
int db_page_selected(uint16_t x, uint16_t y, int numPages, int currentPage){ //this function returns the page selected with joystick
    const int RIGHT = 12000;
    const int LEFT = 4000;

    // --- Horizontal Paging Logic --- //
     if(x>RIGHT) {
         if(currentPage < numPages){
             return (currentPage + 1);
         }
     }
     if(x<LEFT) {
         if(currentPage>1){
            return (currentPage - 1);
         }
    }
     return currentPage;
}

// Helper function to draw the selected menu item in red
void highlight_selected_menu_item(void) {
    const Graphics_Rectangle rect = {sel_rectangle_on_admin_menu.pos_x1,
                                     sel_rectangle_on_admin_menu.pos_y1,
                                     sel_rectangle_on_admin_menu.pos_x2,
                                     sel_rectangle_on_admin_menu.pos_y2};

    int32_t x_string = 64;
    int32_t y_string; // Will be set in the switch block

    int8_t start = 0;
    int8_t end = 2;

    // Determine the string offset once
    int8_t string_offset = (first_screen) ? 0 : 3;

    int i;
    for (i = start; i <= end; i++) {
        int8_t menu_index = i;
        int8_t function_string_index = i + string_offset;

        // 1. Check if the current point is within the rectangle 'rect'
        if (Graphics_isPointWithinRectangle(&rect, MENU_POINTS[menu_index].x, MENU_POINTS[menu_index].y)) {

            // 2. Set the Y position based on the menu_index (0, 1, or 2)
            switch(menu_index) {
            case 0:
                y_string = 50;
                break;
            case 1:
                y_string = 80;
                break;
            case 2:
                y_string = 110;
                break;
            }

            // 3. Draw the selected string in RED
            Graphics_setForegroundColor(&g_sContext, ClrRed);
            Graphics_drawStringCentered(&g_sContext, (int8_t *) function_strings[function_string_index],
                                         AUTO_STRING_LENGTH,
                                         x_string, y_string,
                                         OPAQUE_TEXT);
            // Since we found the selected item, we can stop the loop
            return;
        }
    }
}

/*******************************************************************************
 * @brief       Detects which grid point is currently selected by the grid
 * selection rectangle and provides visual feedback.
 *
 * @details     The function iterates through a predefined array of coordinates,
 * GRID_POINTS, checking if the current position of the global selection
 * rectangle ('sel_rectangle_on_grid') contains any of the points.
 * * If a selected point is found, the function executes a temporary visual
 * "flash" (Red -> White fill) over the selected grid cell for feedback.
 * Finally, it re-draws the cell's content (the number) and the red selection
 * rectangle before returning the selected number.
 *
 * @return      int            Returns the 1-based index (i + 1) of the
 * selected point if a match is found. Returns 0 if no grid point is
 * within the selection rectangle.
 *
 * @note        Uses a busy-wait loop (for j=0; j<100000; j++) to create a
 * delay for the visual "flash." This is a blocking, non-optimal approach
 * for embedded systems and should ideally be replaced with a timer-based delay.
 ******************************************************************************/

int number_selected(void){
    const Graphics_Rectangle rect = {sel_rectangle_on_grid.pos_x1,
                                     sel_rectangle_on_grid.pos_y1,
                                     sel_rectangle_on_grid.pos_x2,
                                     sel_rectangle_on_grid.pos_y2};

    int i;
    // Iterate through the array of fixed grid point coordinates (GRID_POINTS)
    for (i = NUM1; i < NUM_POINTS; i++)
    {
        // Check if the current grid point's coordinates (x, y) fall within
        // the selection rectangle's boundaries.
        if (Graphics_isPointWithinRectangle(
                &rect,
                GRID_POINTS[i].x,
                GRID_POINTS[i].y))
        {
            if (i == VOID_LEFT || i == VOID_RIGHT) return -1;

            // --- Feedback Flash Sequence Start ---

            GrContextFontSet(&g_sContext, &g_sFontCmss18);
            Graphics_setForegroundColor(&g_sContext, ClrRed);
            Graphics_fillRectangle(&g_sContext, &rect);

            delay_ms(100);
            Graphics_setForegroundColor(&g_sContext, ClrWhite);
            Graphics_fillRectangle(&g_sContext, &rect);

            // --- Feedback Flash Sequence End ---

            char string[2];
            int val=i;

            Graphics_setForegroundColor(&g_sContext, ClrBlack);

            (val <= NUM9) ? (++val) : (val=0);
            sprintf(string, "%d", val);

            Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                        AUTO_STRING_LENGTH,
                                        GRID_POINTS[i].x,
                                        GRID_POINTS[i].y,
                                        OPAQUE_TEXT);


            Graphics_setForegroundColor(&g_sContext, ClrRed);
            Graphics_drawRectangle(&g_sContext, &rect);

            // Return the selected number immediately
            return val;
        }
    }
    return -1;
}

/*******************************************************************************
 * @brief       Determines and returns the function identifier corresponding to
 * the currently selected menu item.
 *
 * @details     This function evaluates the position of the selection rectangle
 * ('sel_rectangle_on_admin_menu') against a set of fixed menu points
 * ('MENU_POINTS'). The function's logic is conditional, depending on the state
 * of the global 'first_screen' flag, which allows it to map three menu
 * positions to one of two sets of three distinct function identifiers.
 * * * The function iterates through the three visible menu points (indices 0 to 2).
 * * When a match is found, the corresponding function identifier (e.g.,
 * LAST_ACCESS_LOG, FACTORY_RESET) is assigned to 'selected_function'
 * based on the current screen context.
 *
 * @param       void
 *
 * @return      int            Returns the integer value of the selected function's
 * identifier (e.g., LAST_ACCESS_LOG). The actual identifier values are
 * expected to be defined as global macros or enums.
 *
 ******************************************************************************/
int display_function_selected(void){
    const Graphics_Rectangle rect = {sel_rectangle_on_admin_menu.pos_x1,
                                     sel_rectangle_on_admin_menu.pos_y1,
                                     sel_rectangle_on_admin_menu.pos_x2,
                                     sel_rectangle_on_admin_menu.pos_y2};

    int selected_function = 0; // Initialize to 0 (or an appropriate 'NO_SELECTION' default)

    if(first_screen){ // --- FIRST SCREEN (Functions 0-2) ---
        int i;
            for (i = 0; i <= 2; i++) //only 3 points for screen
            {
                // Check if current point (MENU_POINTS[i]) is inside the rectangle rect
                if (Graphics_isPointWithinRectangle(
                        &rect,
                        MENU_POINTS[i].x,
                        MENU_POINTS[i].y))
                {

                    switch(i){
                    case 0:
                        printf("Last access log \n");
                        selected_function = LAST_ACCESS_LOG;
                        break;

                    case 1:
                        printf("Unlock_door \n");
                        selected_function = UNLOCK_DOOR;
                        break;
                    case 2:
                        printf("RFID_REGISTER\n");
                        selected_function = RFID_REGISTER;
                        break;
                    default:
                        printf("Nothing \n");
                    }
                    // Break the loop once the selected function is found
                    return selected_function;
                }
            }

    }

    // Return the initial value (0) if no menu point was selected on either screen
    return selected_function;
}



void display_menu_last_access_log(void){
    int32_t x_string = 64;
    int32_t y_string = 20;

    Graphics_clearDisplay(&g_sContext);

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "LAST ACCESS LOG",
                                AUTO_STRING_LENGTH,
                                x_string, y_string,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
}

void display_menu_setup_pin(void){
    display_string("SETUP PIN");
    draw_grid();
}

void display_menu_setup_wifi(void){
    int32_t x_string = 64;
    int32_t y_string = 20;

    Graphics_clearDisplay(&g_sContext);

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "SETUP WIFI",
                                AUTO_STRING_LENGTH,
                                x_string, y_string,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
}

void display_menu_factory_reset(void){
    int32_t x_string = 64;
    int32_t y_string = 20;

    Graphics_clearDisplay(&g_sContext);

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "FACTORY RESET",
                                AUTO_STRING_LENGTH,
                                x_string, y_string,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
}

void display_menu_unlock_door(void){
    int32_t x_string = 64;
    int32_t y_string = 20;

    Graphics_clearDisplay(&g_sContext);

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "UNLOCK DOOR",
                                AUTO_STRING_LENGTH,
                                x_string, y_string,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
}

void display_menu_block_pin(void){
    int32_t x_string = 64;
    int32_t y_string = 20;

    Graphics_clearDisplay(&g_sContext);

    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_setForegroundColor(&g_sContext, ClrRed);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) "BLOCK PIN",
                                AUTO_STRING_LENGTH,
                                x_string, y_string,
                                OPAQUE_TEXT);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
}


// -------------------------------------- //


void display_door_open(void){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) "CODE CORRECT",
                                    AUTO_STRING_LENGTH,
                                    64, 54,
                                    OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) "OPENING DOOR",
                                        AUTO_STRING_LENGTH,
                                        64, 74,
                                        OPAQUE_TEXT);
}

void display_wait_RFID(void){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) "PLEASE, USE RFID",
                                    AUTO_STRING_LENGTH,
                                    64, 64,
                                    OPAQUE_TEXT);
}

void display_wrong_pin(int error_pin){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) "WRONG PIN",
                                    AUTO_STRING_LENGTH,
                                    64, 64,
                                    OPAQUE_TEXT);
    char string[20];
    sprintf(string, "ERROR %" PRIu16 "/%d", error_pin, MAX_PIN_TRIES);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                        AUTO_STRING_LENGTH,
                                        64, 84,
                                        OPAQUE_TEXT);
}

void display_block_access(void){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) "ACCESS BLOCKED",
                                    AUTO_STRING_LENGTH,
                                    64, 64,
                                    OPAQUE_TEXT);
}

void display_clock(int hour, int minute){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);

    char string[20];
    sprintf(string, "%02d : %02d", hour, minute);

    Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                    AUTO_STRING_LENGTH,
                                    64, 64,
                                    OPAQUE_TEXT);
}

/*
 * Displays in the center of the screen the passed string
 */
void display_string(const char* string){
    Graphics_clearDisplay(&g_sContext);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) string,
                                    AUTO_STRING_LENGTH,
                                    64, 64,
                                    OPAQUE_TEXT);
    delay_ms(100);
}

