#ifndef __motor_H
#define __motor_H

#include <stdint.h>
#define forward 1
#define back 0
#define stop 2
#define free 3

void motor_init(void);
void motor_control(uint8_t state,uint8_t compare);

	
#endif
