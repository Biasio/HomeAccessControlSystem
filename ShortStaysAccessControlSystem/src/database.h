#ifndef DATABASE_H_
#define DATABASE_H_

#include <stdio.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <string.h>
#include <stdlib.h>
#include "display.h"

/*bisogna prima fare un erase. posso scrivere nelle parte finale della flash main oopure nella flash info
 Guarda su moodle i due manuali: il datasheet e il reference manual a pag 458*/


#define MAX_NUMBER_LOG_SAVED 10
#define DATABASE_START 0x0003F000     //start address of database
//int dbAddress = DATABASE_START;

char buffer[40];

typedef enum {
    USER, ADMIN, DENIED
} dbStates;



typedef struct {                           //struct of a single "log event"
  dbStates logState;
  int used_pin[4];
  char dateHour[20];
}AccessData;

typedef struct {                               //this is the structure that will be saved in flash
  AccessData dbArray[MAX_NUMBER_LOG_SAVED];
  int count;
  int head;
}LogDB;


static LogDB myDb;                 //the instance of logDB which contains the array
LogDB *ptr1;                 //pointer to myDb
LogDB *ptr2;




//FUNCTIONS

void add_log(dbStates s, const char* dH, const int* pin);
void database_init();                   //to initialize all database stuffs (like to copy the array from flash to ram when turning on)
void serial_print_db();
void display_db(int page);

int return_number_count();
int calc_num_pages_db(int numElements);

void write_date(const AccessData* d);           //to overwrite the array "buffer" with infos about date
void write_pin(const AccessData* d);            //to overwrite the array "buffer" with infos about the pin used
void write_accessType(const AccessData* d);     //to overwrite the array "buffer" with infos about the access type
void write_number(int page);                    //to overwrite the array "buffer" with infos about the number to show in database

















#endif /* SRC_DATABASE_H_ */

