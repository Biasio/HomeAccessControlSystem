#ifndef FLASH_H_
#define FLASH_H_

#include "database.h"
#include "comm_esp.h"

// -- these arrays are saved in different sector, this way i can erase/write the flash fewer times and in a safer way
#define DATABASE_START 0x0003F000     //start address of database. i cannot save it in info memory because db occupies more than 128 Byte
#define USER_ARRAY_START 0x0003E000


void save_database();

void save_userArray();















#endif
