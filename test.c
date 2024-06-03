// run by """make c FILE=motors"""

#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>


#define LED_B 20
#define LED_R 21
#define DEVICE_ID 0x16
int tracking;

int getTraceInfo(int fd) {
    tracking = digitalRead(27) << 3 | digitalRead(22) << 2 | digitalRead(17) << 1 | digitalRead(4);
    // printf("[tracking]: %d\tL*", tracking);
    // // if most significant bit is 0: print - else print " "
    // if ((tracking & 0b1000) == 0) {printf("-");} else {printf(" ");}
    // // if x0xx: print | else print " "
    // if ((tracking & 0b0100) == 0) {printf("|");} else {printf(" ");}
    // // if xx0x: print | else print " "
    // if ((tracking & 0b0010) == 0) {printf("|");} else {printf(" ");}
    // // if xxx0: print - else print " "
    // if ((tracking & 0b0001) == 0) {printf("-");} else {printf(" ");}
    // printf("*R\t");
    return tracking;
}
    int turn90(int fd, int left_right){ //left = 0, right = 1, uturn= 2
        int tracking = 0;
        int turn_count = 0; 
        char buffer[] = {0x01, 1, 200, 1, 200};
        while (1){
            if (left_right == 0){
                buffer[1] = 0;
                buffer[2] = 70;
                buffer[3] = 1;
                buffer[4] = 70;
            }
            else if(left_right == 1){
                buffer[1] = 1;
                buffer[2] = 70;
                buffer[3] = 0;
                buffer[4] = 70;
            }
            else{
                buffer[1] = 1;
                buffer[2] = 80;
                buffer[3] = 0;
                buffer[4] = 80;

            }
            if (left_right != 2){
                tracking = getTraceInfo(fd);
                if (tracking != 0b1111) {
                    printf("early turn_count :%d",turn_count);
                }
                if ((tracking != 0b1111) &&turn_count>60){//150-20
                    printf("turn_count :%d",turn_count);
                    break;
                }
                write(fd, buffer, 5);
                delay(5);
                turn_count ++;
                printf(">>> turning : %d\n",left_right);
                if(turn_count > 200){//150-80
                    printf("kill!!!!!!!!!!!!!1");
                    return tracking;
                }
            }
            else{
                tracking = getTraceInfo(fd);
                if (tracking != 0b1111 &&turn_count>180){
                    printf("turn_count :%d",turn_count);
                    break;
                }
                write(fd, buffer, 5);
                delay(5);
                turn_count ++;
                printf(">>> turning : %d\n",left_right);
                if(turn_count > 400){
                    printf("kill!!!!!!!!!!!!!1");
                    return tracking;
                }

            }
        }
        return tracking;
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

    char buffer[] = {0x01, 1, 200, 1, 200};
    char old_buffer[] = {0x01, 1, 50, 1, 50};

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
    int old_tracking = 0b0;
    int runcounter = 0;
    int lineout_counter = 0;
    int line_counter = 0;
    write(fd, buffer, 2);
    delay(100);
    int int_res;
    while (1) {
        // 4 binary values: 0 is detected, 1 is not detected
        // save in one single integer
        // 0b0000 - (27)(22)(17)(04)
        // DON'T BE CONFUSED:: 0 is detected, 1 is not detected!
        tracking = getTraceInfo(fd);
        if (tracking == 0b1111){
            lineout_counter++;
        }
        else{
            lineout_counter = 0;
        }
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

        // simple line tracer
            // rotate until 0b1001
            // if 0b1101: turn right
            // if 0b1011: turn left
        buffer[0] = 0x01;
        if (tracking != old_tracking)
        printf("tracking :%d",tracking);

        if (line_counter>40 && tracking == 7||tracking == 3||tracking == 0|| tracking == 1||tracking == 2||tracking == 4||tracking == 8||tracking == 10||tracking == 5||tracking == 6){ //0000 0001 0010 0100 1000 1010 0101 0110
            tracking = turn90(fd,0);
            if (tracking != old_tracking)
            printf(">>> INTERSECTION :[%d]\n", tracking);
            line_counter = 0;
            // TODO: find which way to go here.
        }
        else if (tracking == 12) { //1101 1100
            // turn right
            buffer[1] = 1;
            buffer[2] = 100;
            buffer[3] = 1;
            buffer[4] = 50;
            if (tracking != old_tracking)
            printf(">>> slight RIGHT\n");
        } else if (tracking == 14) { // 1110
            // turn right
            buffer[1] = 1;
            buffer[2] = 130;
            buffer[3] = 1;
            buffer[4] = 40;
            if (tracking != old_tracking)
            printf(">>> big RIGHT\n");
        } else if (tracking == 11||tracking == 3 ) { //1011 0011 
            // turn left
            buffer[1] = 1;
            buffer[2] = 50;
            buffer[3] = 1;
            buffer[4] = 100;
            if (tracking != old_tracking)
            printf(">>> slight LEFT\n");
        } else if (tracking == 7 ) { // 0111
            // turn left
            buffer[1] = 1;
            buffer[2] = 40;
            buffer[3] = 1;
            buffer[4] = 130;
            if (tracking != old_tracking)
            printf(">>> big LEFT\n");
        }else if (tracking == 15 && lineout_counter<250) { //1111
            buffer[1] = old_buffer[1];
            buffer[2] = old_buffer[2];
            buffer[3] = old_buffer[3];
            buffer[4] = old_buffer[4];
            if (tracking != old_tracking)
            printf(">>> lineout : go\n");
        }
        else if (tracking == 15){ //1111
            buffer[1] = 0;
            buffer[2] = 40;
            buffer[3] = 0;
            buffer[4] = 40;
            if (tracking != old_tracking)
            printf(">>> line out : BACK\n");
        }
        else if (tracking == 13 ||tracking == 9) {//1001 1101
            // go straight
            buffer[1] = 1;
            buffer[2] = 70;
            buffer[3] = 1;
            buffer[4] = 80;
            if (tracking != old_tracking)
            printf(">>> STRAIGHT\n");
        }
        if (runcounter > 1500) {
            buffer[0] = 0x02;
            buffer[1] = 0;
            write(fd, buffer, 2);
            delay(1000);
            if (tracking != old_tracking)
            printf("========== Terminate ==========\n");
            break;
        }        
        old_tracking  = tracking;
        int res = write(fd, buffer, 5);
        old_buffer[1] = buffer[1];
        old_buffer[2] = buffer[4];
        old_buffer[3] = buffer[3];
        old_buffer[4] = buffer[2];
        runcounter++;
        line_counter++;
        delay(5); // TODO: change the frequency as needed
    }
    return 0;
}
