#include "motor.h"

bool pattern[4][4] = {
  {1,1,0,0},
  {0,1,1,0},
  {0,0,1,1},
  {1,0,0,1}
};

uint_fast8_t portArray[4] = { GPIO_PORT_P2,GPIO_PORT_P6, GPIO_PORT_P6, GPIO_PORT_P2 };
uint_fast16_t pinArray[4] = { GPIO_PIN5, GPIO_PIN6, GPIO_PIN7, GPIO_PIN3 };

float anglePerSteps;

void moveMotor(int angle){

    int numSteps = (int) angle / anglePerSteps;
    bool reverse = false;
    int tmp = 0;
    int j = 0;


    if(numSteps<0){     //to see if the motor runs clockwise or counterClockwise
        reverse = true;
      } else { reverse = false; }


///---      if motor runs counterclockwise, the for-cicle starts "from the end" and this way the motor runs on right verse    ---
    int i = 0 + reverse*numSteps;
    for(i; i < (numSteps - reverse*numSteps); i++){

        tmp = i%4;
        if(tmp<0) tmp = -tmp;

        for(j=0; j<4; j++){

          if(pattern[tmp][j]){          //if the pin needs to go HIGH
            GPIO_setOutputHighOnPin(portArray[j], pinArray[j]);
          } else {                      //if the pin needs to go LOW
            GPIO_setOutputLowOnPin(portArray[j], pinArray[j]);
          }

        }
        delay_ms(MOTOR_DELAY);
      }
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN3);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN7);
}


void motor_init(){
    anglePerSteps = (float) 360 / STEPS_FOR_360;
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN6);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN7);

    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN3);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN6);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN7);
}


