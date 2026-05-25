#include "flash.h"


void save_database(){
    /* Setting our MCLK to 48MHz for faster programming*/
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    FlashCtl_setWaitState(FLASH_BANK0, 1);
    FlashCtl_setWaitState(FLASH_BANK1, 1);
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);

    //![FlashCtl Program]
    /* Unprotecting Info Bank 0, Sector 0  */
    MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);

    /* Trying to erase the sector. Within this function, the API will
        automatically try to erase the maximum number of tries.  */
    if(!MAP_FlashCtl_eraseSector(DATABASE_START)){
        printf("Error in erasing sector! \n");
    }


    /* Trying to program the memory. Within this function, the API will
        automatically try to program the maximum number of tries. */
    if(!MAP_FlashCtl_programMemory((LogDB*)&myDb, (void*) DATABASE_START, sizeof(LogDB))){      //qua al posto di ptr1 ho usato &myDb, e dopo boh ha iniziato a funzionare
        printf("Error in writing flash! \n");
    }

    /* Setting the sector back to protected  */
    MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);           //here is the last sector
    printf("saved in db \n");
}

void save_userArray(){
    /* Setting our MCLK to 48MHz for faster programming*/
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);
    FlashCtl_setWaitState(FLASH_BANK0, 1);
    FlashCtl_setWaitState(FLASH_BANK1, 1);
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);

    //![FlashCtl Program]
    /* Unprotecting Info Bank 0, Sector 0  */
    MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR30);     //here is the penultimo sector

    /* Trying to erase the sector. Within this function, the API will
        automatically try to erase the maximum number of tries.  */
    if(!MAP_FlashCtl_eraseSector(USER_ARRAY_START)){
        printf("Error in erasing sector! \n");
    }


    /* Trying to program the memory. Within this function, the API will
        automatically try to program the maximum number of tries. */
    if(!MAP_FlashCtl_programMemory((TempUser*)activeTempUsers, (void*) USER_ARRAY_START, sizeof(TempUser))){      //qua al posto di ptr1 ho usato &myDb, e dopo boh ha iniziato a funzionare
        printf("Error in writing flash! \n");
    }

    /* Setting the sector back to protected  */
    MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR30);
    printf("saved userArray \n");
}
