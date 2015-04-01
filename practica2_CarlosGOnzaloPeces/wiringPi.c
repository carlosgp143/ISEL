#include <stdio.h>
#include "wiringPi.h"

void digitalWrite(int gpio, int value){

	char* pin;

	switch (gpio){
		case 2: pin="GPIO_BUTTON"; break;
		case 3: pin="GPIO_LED"; break;
		case 4: pin="GPIO_CUP"; break;
		case 5: pin="GPIO_COFFE"; break;
		case 6: pin="GPIO_MILK"; break;
		case 7: pin="GPIO_CHANGE"; break;
		case 8: pin="GPIO_COIN"; break;

	}

	//printf("El pin %s vale %d \n", pin, value);
	

}

void pinMode(int gpio, int mode){
}

