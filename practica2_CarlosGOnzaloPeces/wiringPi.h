#ifndef WIRING_PI
#define WIRING_PI

#define LOW 0
#define HIGH 1

#define INPUT 0
#define OUTPUT 1

#define wiringPiSetup()
#define wiringPiISR(gpio,mode,func)
void pinMode(int gpio, int mode);

void digitalWrite(int gpio,int value);



#endif