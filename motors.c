// run by """make c FILE=motors"""

#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>


#define LED_B 20
#define LED_R 21
#define DEVICE_ID 0x16

int main(void)
{
    // uses BCM numbering of the GPIOs and directly accesses the GPIO registers.
    wiringPiSetupGpio();
    // Initializing I2C connection
    int fd = wiringPiI2CSetup(DEVICE_ID);

    if (fd == -1)
    {
        printf("Error: cannot init I2C communication\n");
        return -1;
    }
    printf("I2C setup Completed. %d\n", fd);
    
    /* 
    ================================
    Initial setting Tracking Sensors
    L <-  27 -  22 -  17 -   4 -> R
         SW2   SW1   SW3   SW4
    ================================
    */

    pinMode(27, INPUT); // SW2 - LEFTMOST
    pinMode(22, INPUT); // SW1
    pinMode(17, INPUT); // SW3
    pinMode( 4, INPUT); // SW4 - RIGHTMOST

    char buffer[] = {0x01, 1, 10, 1, 30};
    /* buffer - command to send to the motors
    Example buffers
    GO straight:    0x01, 1, 50, 1, 50
    STOP:           0x02, 0
    Move servo:     0x03, id, angle
    
     */
    // write(fd, buffer, sizeof(buffer));
    // // printf("writing GO complete\n");
    // sleep(1);
    // // wiringPiI2CWriteReg8(fd, 0x02, 0);
    // buffer[0] = 0x02;
    // buffer[1] = 0;
    // write(fd, buffer, 2);
    // // printf("writing STOP complete\n");
    // // sleep(1);
    // move servo: 0x03, id, angle
    buffer[0] = 0x03;
    buffer[1] = 1;
    buffer[2] = 45;
    write(fd, buffer, 3);
    delay(100);
    buffer[2] = 90;
    int res = write(fd, buffer, 3);
    printf("Write result: %d\n", res);

    // test tracking module

    int tracking = 0b0;
    while (1) {
        // 4 binary values: 0 is detected, 1 is not detected
        // save in one single integer
        // 0b0000 - (27)(22)(17)(04)
        // DON'T BE CONFUSED:: 0 is detected, 1 is not detected!
        tracking = digitalRead(27) << 3 | digitalRead(22) << 2 | digitalRead(17) << 1 | digitalRead(4);
        printf("Tracking module: %0x\tL*", tracking);
        // if most significant bit is 0: print - else print " "
        if ((tracking & 0b1000) == 0) {printf("-");} else {printf(" ");}
        // if x0xx: print | else print " "
        if ((tracking & 0b0100) == 0) {printf("|");} else {printf(" ");}
        // if xx0x: print | else print " "
        if ((tracking & 0b0010) == 0) {printf("|");} else {printf(" ");}
        // if xxx0: print - else print " "
        if ((tracking & 0b0001) == 0) {printf("-");} else {printf(" ");}
        printf("*R\n");


        delay(500); // TODO: change the frequency as needed
    }
    return 0;
}
