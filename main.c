#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "server.h"

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
    // Initializing I2C connection
    wiringPiSetupGpio();
    int fd = wiringPiI2CSetup(DEVICE_ID);
    pinMode(27, INPUT); // SW2 - LEFTMOST
    pinMode(22, INPUT); // SW1
    pinMode(17, INPUT); // SW3
    pinMode( 4, INPUT); // SW4 - RIGHTMOST

    if (fd == -1) {
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

int readQR() {
    // 강규영
    // QR 코드 인식 함수
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
    printf("*R\n");
    
    return tracking;
}

char buf_go[5] = {0x01, 1, 50, 1, 50};
char buf_stop[2] = {0x02, 0};

int moveBot(int fd, int decision, int speed) { 
    // @dooraemee0
    char buffer[5];
    buffer[0] = 0x01; // GO

    switch (decision) {
        case 0: // straight
            buffer[1] = 1;
            buffer[2] = speed;
            buffer[3] = 1;
            buffer[4] = speed;
            break;
        case 1: // left
            buffer[1] = 1;
            buffer[2] = speed * 0.25; // 왼쪽 속도 감소
            buffer[3] = 1;
            buffer[4] = speed;
            break;
        case 2: // down (reverse)
            buffer[1] = 0;
            buffer[2] = speed;
            buffer[3] = 0;
            buffer[4] = speed;
            break;
        case 3: // right
            buffer[1] = 1;
            buffer[2] = speed;
            buffer[3] = 1;
            buffer[4] = speed * 0.25; // 오른쪽 속도 감소
            break;
        default:
            printf("Invalid decision value\n");
            return -1;
    }

    int res = write(fd, buffer, 5);
    return res; // 비정상 종료시 -1, 나머지 숫자 반환시 정상적으로 전송된 것.
    int res = write(fd, buf_go, 5);
    return res; // 비정상 종료시 -1, 나머지 숫자 반환시 정상적으로 전송된 것.
}; 

void onNode() {
    // 현재 위치와 방향에 따라 좌표를 업데이트
    switch (curr_pos.dir) {
        case 0: // RIGHT
            curr_pos.x += 1;
            break;
        case 1: // UP
            curr_pos.y += 1;
            break;
        case 2: // LEFT
            curr_pos.x -= 1;
            break;
        case 3: // DOWN
            curr_pos.y -= 1;
            break;
        default:
            printf("Invalid direction\n");
            break;
    }

    // 좌표가 범위를 벗어나지 않도록 확인
    if (curr_pos.x < -1) curr_pos.x = -1;
    if (curr_pos.x > 5) curr_pos.x = 5;
    if (curr_pos.y < 0) curr_pos.y = 0;
    if (curr_pos.y > 4) curr_pos.y = 4;

    printf("Updated position: (%d, %d, %d)\n", curr_pos.x, curr_pos.y, curr_pos.dir);
}

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
            onNode();                         // update the current position
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
