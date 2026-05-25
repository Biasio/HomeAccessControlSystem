#include "database.h"


//formato: dd/mm - hh:mm
//          USER ACCESS

    /// ALLORA adesso sembrerebbe funzionare. anche se stacco e poi riattacco, il sistema in automatico
     //  sembra ritrovare i valori di head e di count. inoltre sul memory browser tutto sembra funzionare
     //  come previsto. adesso bisogna vedere se dalla flash viene prelevato sempre anche l'array, per stamparlo sul display.

void database_init(){

   // ptr1 = &myDb;                    //i set ptr to point to myDb
    ptr2 = (volatile LogDB *)DATABASE_START;

    myDb = *ptr2;    //when i initialize, myDb (in Ram) copies data from flash


    printf(" dalla flash: head =%d   count = %d\n", myDb.head,myDb.count);
    if(myDb.count<0){myDb.count=0;}
    if(myDb.head<0){myDb.head=0;}
}

void add_log(dbStates s, const char* dH, const int* pin){
    printf("entrato su addlog\n");
    printf("head =%d   count = %d\n", myDb.head,myDb.count);

     myDb.dbArray[myDb.head].logState = s;           //to add state info

    strncpy(myDb.dbArray[myDb.head].dateHour,       //to copy the info about date and hour
            dH,
            sizeof(myDb.dbArray[myDb.head].dateHour) - 1);
    myDb.dbArray[myDb.head].dateHour[19] = '\0';    //to be sure that the last element is the string terminator

    int i;
    for(i=0; i<4; i++){                             //to copy the pin
        myDb.dbArray[myDb.head].used_pin[i] = pin[i];
        }

    myDb.head = (myDb.head + 1) % MAX_NUMBER_LOG_SAVED;         //this way when number element>10 the oldest one is overwritten

    if(myDb.count < MAX_NUMBER_LOG_SAVED) myDb.count++;         //to know the number of elements in the array
}

void serial_print_db(){
    printf("STAMPAAA \n");
    printf("count serial_print = %d \n", myDb.count);
    int i;
    int j;
    if(myDb.count==0){
        printf("niente da stampare \n");
    }
    for(i=0; i<myDb.count; i++){
        printf("%s  PIN:", myDb.dbArray[i].dateHour);       //to print date and hour

        for(j=0; j<4; j++){
            printf("%d ", myDb.dbArray[i].used_pin[j]); //to stamp the pin
        }
        switch(myDb.dbArray[i].logState){
        case USER: printf(" USER ACCESS \n"); break;
        case ADMIN: printf(" ADMIN ACCESS \n"); break;
        case DENIED: printf(" DENIED ACCESS \n"); break;
        }
    }
}

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
    MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,FLASH_SECTOR31);
    printf("saved in db \n");
}


void display_db(int page){
    int numPages = calc_num_pages_db(myDb.count);
    int index;
    int i;
    //-- INDEX --       calculation of the index of the element to show depending on the page
    if(myDb.count==MAX_NUMBER_LOG_SAVED){        //in this case, head points to the oldest element (the first to show)
        index = ((page*2) + myDb.head - 2)%MAX_NUMBER_LOG_SAVED;   //DOVREBBE ESSERE GIUSTO  //this way, in page 1, i show the oldest element (and at page 5 the youngest)
    } else {        //if i don't have erased some elements
        index = (page*2)-2;         //for example in page 3 the first element i will see is the 5th, so dbArray[4]. if i have not erased old elements yet
    }

    //-- PAGE NUMBER --
    char msg[20];
    sprintf(msg, " Page %d/%d", page, numPages);
    Graphics_setForegroundColor(&g_sContext, ClrBlack);
    GrContextFontSet(&g_sContext, &g_sFontCmss12);
    Graphics_drawStringCentered(&g_sContext, (int8_t *) msg,
                                            AUTO_STRING_LENGTH,
                                            23, 5,
                                            OPAQUE_TEXT);
    //--------------------------------------------------




    for(i=0; i<2; i++){
        if( ((index+i)%MAX_NUMBER_LOG_SAVED) > (myDb.count-1) ) {           //if the second element exceeds count, don't show anything
            return;
        }

        Graphics_setForegroundColor(&g_sContext, ClrBlack);                 //to write in color black (write_accessType changes the color)
        Graphics_drawLineH(&g_sContext, 1, 127, (29+i*50));                 //to draw the line between different logs

        write_number((page*2)-1+i);                                               //to write the number of the element
        Graphics_drawStringCentered(&g_sContext, (int8_t *) buffer,
                                                    AUTO_STRING_LENGTH,
                                                    7, (55+i*50),
                                                    OPAQUE_TEXT);


        GrContextFontSet(&g_sContext, &g_sFontCmss14);
        //to write each row, i re-write the variable "buffer" and show it
        write_date(&myDb.dbArray[(index+i)%MAX_NUMBER_LOG_SAVED]);         //to write the element at "index" position (the first) and the following one
        Graphics_drawStringCentered(&g_sContext, (int8_t *) buffer,
                                                AUTO_STRING_LENGTH,
                                                64, (40+i*50),
                                                OPAQUE_TEXT);

        write_pin(&myDb.dbArray[(index+i)%MAX_NUMBER_LOG_SAVED]);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) buffer,
                                                AUTO_STRING_LENGTH,
                                                64, (55+i*50),
                                                OPAQUE_TEXT);

        write_accessType(&myDb.dbArray[(index+i)%MAX_NUMBER_LOG_SAVED]);
        Graphics_drawStringCentered(&g_sContext, (int8_t *) buffer,
                                                AUTO_STRING_LENGTH,
                                                64, (70+i*50),
                                                OPAQUE_TEXT);
    }

}

int return_number_count(){
    return myDb.count;
}
int calc_num_pages_db(int numElements){
    int numPages = (int) (numElements + 2 - 1)/2;   //two elements each page. this formula allows you to ceil the result (ex if 7 elements -> 4 pages)
    //printf("numElem = %d,  numPages = %d \n", numElements, numPages);
    return numPages;
}

void write_date(const AccessData* d){
    sprintf(buffer, " %s ", d->dateHour);
}
void write_pin(const AccessData* d){
    int i;
    char tmp[5];
    sprintf(buffer, "PIN: ");
    for(i=0; i<4; i++){         //to write the pin number
        sprintf(tmp, "%d ", d->used_pin[i]);        //i need to convert int into char
        strcat(buffer, tmp);                //to concatenate all the strings
        }
}
void write_accessType(const AccessData* d){
    switch(d->logState){
    case USER: {
        sprintf(buffer, "USER ACCESS");
        Graphics_setForegroundColor(&g_sContext, ClrGreen);
        break;
    }
    case ADMIN: {
        sprintf(buffer, "ADMIN ACCESS");
        Graphics_setForegroundColor(&g_sContext, ClrGreen);
        break;
    }
    case DENIED: {
        sprintf(buffer, "ACCESS DENIED");
        Graphics_setForegroundColor(&g_sContext, ClrRed);
        break;
    }
    }
}
void write_number(int page){
    sprintf(buffer, "%d)", page);
    GrContextFontSet(&g_sContext, &g_sFontCmss16);
}
