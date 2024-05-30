// run by """make c FILE=motors"""

#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>


#define LED_B 20
#define LED_R 21
#define DEVICE_ID 0x16

int getTraceInfo(int fd) {
    tracking = digitalRead(27) << 3 | digitalRead(22) << 2 | digitalRead(17) << 1 | digitalRead(4);
    printf("[tracking]: %d\tL*", tracking);
    // if most significant bit is 0: print - else print " "
    if ((tracking & 0b1000) == 0) {printf("-");} else {printf(" ");}
    // if x0xx: print | else print " "
    if ((tracking & 0b0100) == 0) {printf("|");} else {printf(" ");}
    // if xx0x: print | else print " "
    if ((tracking & 0b0010) == 0) {printf("|");} else {printf(" ");}
    // if xxx0: print - else print " "
    if ((tracking & 0b0001) == 0) {printf("-");} else {printf(" ");}
    printf("*R\t");
    
}

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

    char buffer[] = {0x01, 1, 50, 1, 50};
    /* buffer - command to send to the motors
    Example buffers
    GO straight:    0x01, 1, 50, 1, 50
    STOP:           0x02, 0
    Move servo:     0x03, id, angle    
     */
    // move servo: 0x03, id, angle
    buffer[0] = 0x03;
    buffer[1] = 1;
    buffer[2] = 45;
    write(fd, buffer, 3);
    delay(100);
    buffer[2] = 90;
    write(fd, buffer, 3);
    delay(100);
    buffer[2] = 45;
    write(fd, buffer, 3);
    delay(100);
    buffer[2] = 90;
    write(fd, buffer, 3);
    delay(100);
    // test tracking module

    int tracking = 0b0;
    int runcounter = 0;
    write(fd, buffer, 2);
    delay(100);


    while (1) {
        // 4 binary values: 0 is detected, 1 is not detected
        // save in one single integer
        // 0b0000 - (27)(22)(17)(04)
        // DON'T BE CONFUSED:: 0 is detected, 1 is not detected!
        tracking = digitalRead(27) << 3 | digitalRead(22) << 2 | digitalRead(17) << 1 | digitalRead(4);
/*         // change for debug
        if (runcounter < 10) {
            // go straight
            tracking = 9;
        } else if (runcounter < 15) {
            // turn right
            tracking = 13;
        } else if (runcounter < 20) {
            // turn left
            tracking = 11;
        } else {
            // go back
            tracking = 15;
        } */
        printf("[tracking]: %d\tL*", tracking);
        // if most significant bit is 0: print - else print " "
        if ((tracking & 0b1000) == 0) {printf("-");} else {printf(" ");}
        // if x0xx: print | else print " "
        if ((tracking & 0b0100) == 0) {printf("|");} else {printf(" ");}
        // if xx0x: print | else print " "
        if ((tracking & 0b0010) == 0) {printf("|");} else {printf(" ");}
        // if xxx0: print - else print " "
        if ((tracking & 0b0001) == 0) {printf("-");} else {printf(" ");}
        printf("*R\t");

        // simple line tracer
            // rotate until 0b1001
            // if 0b1101: turn right
            // if 0b1011: turn left
        buffer[0] = 0x01;
        if (tracking == 13) {
            // turn right
            buffer[1] = 1;
            buffer[2] = 80;
            buffer[3] = 1;
            buffer[4] = 40;
            printf(">>> RIGHT\n");
        } else if (tracking == 11) {
            // turn left
            buffer[1] = 1;
            buffer[2] = 40;
            buffer[3] = 1;
            buffer[4] = 80;
            printf(">>> LEFT\n");
        } else if (tracking == 15) {
            buffer[1] = 0;
            buffer[2] = 40;
            buffer[3] = 0;
            buffer[4] = 40;
            printf(">>> BACK\n");
        } else if (tracking == 0){
            buffer[1] = 1;
            buffer[2] = 0;
            buffer[3] = 1;
            buffer[4] = 0;
            // TODO: find which way to go here.
            printf(">>> INTERSECTION\n");
        } else if (tracking == 9) {
            // go straight
            buffer[1] = 1;
            buffer[2] = 80;
            buffer[3] = 1;
            buffer[4] = 80;
            printf(">>> STRAIGHT\n");
        } else {
            buffer[1] = 1;
            buffer[2] = 30;
            buffer[3] = 0;
            buffer[4] = 30;
            printf(">>> others\n");
        }

        if (runcounter > 200) {
            buffer[0] = 0x02;
            buffer[1] = 0;
            write(fd, buffer, 2);
            delay(1000);
            printf("========== Terminate ==========\n");
            break;
        }

        int res = write(fd, buffer, 5);
        runcounter++;
        delay(25); // TODO: change the frequency as needed
    }
    return 0;
}

