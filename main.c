// import motors.c qr.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "server.h"

#include <wiringPi.h>
#include <wiringPiI2C.h>

// 모터 제어 함수들은 이 파일에 정의
// #include <qr.cpp>   // QR코드 인식 함수들은 이 파일에 정의
#define DEVICE_ID 0x16      // <- I2C device ID. This is the ID of connection.

#define INTERVAL 20         // <- Interval time for each loop. unit: ms(miliseconds)
#define DURATION 6          // <- While loop duration. after (DURATION) seconds, the loop terminates. unit: seconds
#define INPUT 0

struct position {
    int x;      // x coord of the current position: min -1, max 5
    int y;      // y coord of the current position: min 0, max 4
    int dir;    // 0: RIGHT, 1: UP, 2: LEFT, 3: DOWN
};

struct position curr_pos = {0, 0, 0};
struct position oppo_pos = {0, 0, 0};

int gpio_init() {
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

    return fd;
}

/* SOCKET - SERVER PART */
int socket_init(int port) {
    // Initialize Socket connection with server
    printf("Socket setup Completed. %d\n", port);
    return port;
}

int sendSVR(int sock, int decision){
    // @leeminki02
    return 0;
}

// dooraemee0: include 같은건 추가해야함
int getTraceInfo(int fd) { 
    int tracking = digitalRead(27) << 3 | digitalRead(22) << 2 | digitalRead(17) << 1 | digitalRead(4);
    /* ===================================================
            Initial setting Tracking Sensors
            L <-  27 -  22 -  17 -   4 -> R
                SW2   SW1   SW3   SW4
    4 binary values: 0 is detected, 1 is not detected
    save in one single integer
    0b0000 - (27)(22)(17)(04)
    DON'T BE CONFUSED:: 0 is detected, 1 is not detected!
    =================================================== */
    
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
    
    return tracking;
}

char buf_go[5] = {0x01, 1, 50, 1, 50};
char buf_stop[2] = {0x02, 0};

int moveBot(int fd, int decision, int speed) { 
    // @dooraemee0
    // decision 값: 0: straight, 1: left, 2: down, 3: right
    // speed 값: 최대 속도. 돌거나 할 때 양 쪽 속도가 달라야 한다면 한쪽을 25~50% 정도 값을 주면 됨
    /* 
    buffer - command to send to the motors
            Example buffers
            GO:         [0x01, 1, left_speed, 1, right_speed]
            STOP:       [0x02, 0]
            Servo:      [0x03, id, angle]    // 서보모터 제어인데, 쓸모 없음
    */
    // 웬만하면 STOP을 쓰지 않고 GO(speed 0)을 주는 게 좋음: STOP 한번 주니까 계속 다시 실행 안되는 버그가 있는듯. 그냥 속도 0 주고, 마지막에 로봇 자체를 멈출 때만 STOP 주면 될듯
    // 각 방향 구현 방법은 motors_example.c 파일 참고!

    int res = write(fd, buf_go, 5);
    return res; // 비정상 종료시 -1, 나머지 숫자 반환시 정상적으로 전송된 것.
}; 

int decide(int tracked) {
    // @minseok
    // decide the next move / decide if we install trap here
    // send the decision to the server
    // set the next decision value: straight, left, right, back
    // return 0;
    // return the value of the decision
    // 0: straight, 1: left, 2: down, 3: right

    return 0;
}

int main(int argc, char *argv[]){
    if (argc == 1) {
        printf("No arguments provided.\n");
    } else {
        printf("Total arguments passed: %d\n", argc - 1); 
        for (int i = 1; i < argc; i++) {
            printf("Argument %d: %s\n", i, argv[i]);
        }
    }
    int portNo = atoi(argv[1]);
    int startP = atoi(argv[2]);

    /*    USER INPUT    */
    // TODO: get the port number from user -> portNo
    // TODO: get the starting position from user as 0 or 1 -> startP
    // get the first coordinate from argument

    /*     INITIAL CONFIG    */
    int sock = socket_init(portNo);
    int fd = gpio_init();
    /* DECLARE VARIABLES */
    int speed = 100;
    int decision;

    if (startP == 0) {
        curr_pos.x = -1;
        curr_pos.y = 0;
        curr_pos.dir = 0;
    } else if (startP == 1) {
        curr_pos.x = 5;
        curr_pos.y = 4;
        curr_pos.dir = 2;
    } else {
        printf("Invalid argument\n");
        return -1;
    }
    printf("[INIT] position: %d (%d, %d, %d)\n", startP, curr_pos.x, curr_pos.y, curr_pos.dir);

    if (fd == -1 || sock == -1) {
        printf("Error: initialization failed: fd[%d], sock[%d]\n", fd, sock);
        return -1;
    }
    printf("[INIT] GPIO:    %d\n", fd);
    printf("[INIT] Socket:  %d\n", portNo);
    

    int counter = 0;
    while (1) {
        /*     loop body     */
        int tracked = getTraceInfo(fd);     // get tracing info get_tracing_info(fd);
        int qr = readQR();                  // read qr code
        
        // @minseok -> complete conditions
        if (1 /* we detected intersection */) {
            decision = decide(tracked);       // decide the next move and save the value to decision
        } else if ( 1/* we detect qr code from camera */) {
            // slow down maybe?
            // speed /= 2;
        }
        // 여기까지 오민석

        int sockRes = sendSVR(sock, decision);             // send the decision to the server
        int moveRes = moveBot(fd, decision, speed);        // give command to the bot

        /*     loop control     */
        delay(INTERVAL);    // delay loop for INTERVAL miliseconds
        counter++;          // increment the counter
        int buffer[2] = {0x02, 0};
        if (counter * INTERVAL / 1000 > DURATION) { 
            // break the loop after DURATION seconds
            printf("Terminate the loop\n");
            buffer[0] = 0x02;
            buffer[1] = 0;
            write(fd, buffer, 2);
            delay(1000);
            printf("========== Terminate ==========\n");
            break;
        }
        
    }

    return 0; // 정상 실행 종료.
}
