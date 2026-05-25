#include "motor.h"

bool pattern[4][4] = {
  {1,1,0,0},
  {0,1,1,0},
  {0,0,1,1},
  {1,0,0,1}
};

float anglePerSteps = (float) 360 / STEPS_FOR_360;




void moveMotor(int angle){
//    anglePerSteps  = (float) 360 / STEPS_FOR_360;
    int numSteps = (int) angle / anglePerSteps;
    bool reverse;
    int tmp;
    int j;

    if(numSteps<0){     //to see if the motor runs clockwise or counterClockwise
        reverse = true;
      } else { reverse = false; }

    int i = 0 + reverse*numSteps;


    for(i; i < (numSteps - reverse*numSteps); i++){

        tmp = i%4;
        if(tmp<0) tmp = -tmp;

        for(j=0; j<4; j++){

/*          if(pattern[tmp][j]){
            digitalWrite(pin[j], HIGH);
          } else {
            digitalWrite(pin[j], LOW);
          } */
        }
        delay_ms(MOTOR_DELAY);
      }
}

