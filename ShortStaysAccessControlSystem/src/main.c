#include "msp.h"
#include <stdio.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

#include "fsm.h"

int main(void)
{
    while(1){
        FSM_Run();
    }
}

