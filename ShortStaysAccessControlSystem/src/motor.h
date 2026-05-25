#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>
#include <stdbool.h>
#include "timers.h"


#define STEPS_FOR_360  2038       //the number of steps to make a full rotation. it's not 2048
#define MOTOR_DELAY  2           //delay between pulses





void moveMotor(int angle);




#endif /* MOTOR_H_ */
