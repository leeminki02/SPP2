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
    printf("*R \t");
    
    return tracking;
}

char buf_go[5] = {0x01, 1, 200, 1, 200};
char buf_stop[2] = {0x02, 0};

int traceLine(int fd, int tracked, int speed) {
    if (tracked == 13) {
        // turn right
        buf_go[1] = 1;
        buf_go[2] = 100;
        buf_go[3] = 1;
        buf_go[4] = 30;
        printf(">>> slight RIGHT\n");
    } else if (tracked == 11) {
        // turn left
        buf_go[1] = 1;
        buf_go[2] = 30;
        buf_go[3] = 1;
        buf_go[4] = 100;
        printf(">>> slight LEFT\n");
    } else if (tracked == 15) {
        buf_go[1] = 0;
        buf_go[2] = 100 / 2;
        buf_go[3] = 0;
        buf_go[4] = 100 / 2;
        printf(">>> BACK\n");
    } else if (tracked == 0b1001) {
        buf_go[1] = 1;
        buf_go[2] = 100;
        buf_go[3] = 1;
        buf_go[4] = 100;
    } else if (tracked == 0b1000 || tracked == 0b0000 || tracked == 0b0001) {
        // This means that tracker have detected a intersection
        printf(">>> Intersection detected\n");
        return -1;
    } else {
        printf(">>> Invalid tracking value\n");
        return -1;
    }
    int res = write(fd, buf_go, 5);
    delay(INTERVAL);    // delay loop for INTERVAL miliseconds
    return res; // 비정상 종료시 -1, 나먨지 숫자 반환시 정상적으로 전송된 것.
}

int makeTurn(int fd, int decision) {
    // FIXME: implement using the while loop to make sure it moves precisely.
    int traceInfo;
    switch (decision) {
        case 0: // straight
            break;
        case 1: // left
            printf("LEFT\n");
            buf_go[1] = 0;
            buf_go[2] = 0; // 왼쪽 속도 감소
            buf_go[3] = 1;
            buf_go[4] = 100;
            while(1) {
                write(fd, buf_go, 5);
                delay(10);
                traceInfo = getTraceInfo(fd);
                if (traceInfo != 0b1111) {
                    break;
                }
            }
            break;
        case 2: // U-turn
            printf("U-Turn\n");
            buf_go[1] = 0;
            buf_go[2] = 100;
            buf_go[3] = 1;
            buf_go[4] = 100;
            int tracecounter = 0;
            while(1) {
                write(fd, buf_go, 5);
                delay(25);
                traceInfo = getTraceInfo(fd);
                if (traceInfo == 0b1000 || traceInfo == 0b0000 || traceInfo == 0b0001 || traceInfo == 0b1001) {
                    tracecounter++;
                }
                if (tracecounter == 2) break;
            }
            break;
        case 3: // right
            printf("RIGHT\n");
            buf_go[1] = 1;
            buf_go[2] = 100;
            buf_go[3] = 0;
            buf_go[4] = 0; // 오른쪽 속도 감소
            while(1) {
                write(fd, buf_go, 5);
                delay(25);
                traceInfo = getTraceInfo(fd);
                if (traceInfo == 0b1000 || traceInfo == 0b0000 || traceInfo == 0b0001 || traceInfo == 0b1001) {
                    break;
                }
            }
            break;
        default:
            printf("Invalid decision value\n");
            return -1;
    }
    int res = write(fd, buf_go, 5);
    delay(INTERVAL);    // delay loop for INTERVAL miliseconds
    return res; // 비정상 종료시 -1, 나머지 숫자 반환시 정상적으로 전송된 것.
}

void updateCoord() {
    // 현재 위치와 방향에 따라 좌표를 업데이트
    // @minseok: check if it's properly done.
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
    if (curr_pos.x <-1) curr_pos.x =-1;
    if (curr_pos.x > 5) curr_pos.x = 5;
    if (curr_pos.y < 0) curr_pos.y = 0;
    if (curr_pos.y > 4) curr_pos.y = 4;

    printf("Updated position: (%d, %d, %d)\n", curr_pos.x, curr_pos.y, curr_pos.dir);
}

int decide(int tracked, struct position curr_pos) {
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
    /*    USER INPUT    */
    // get the port number from user -> portNo
    // get the starting position from user as 0 or 1 -> startP
    // get the first coordinate from argument
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

    /*     INITIAL CONFIG    */
    int sock = socket_init(portNo);
    int fd = gpio_init();
    /* DECLARE VARIABLES */
    int speed = 200;
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
        // default: trace the line until a new intersection is detected
        // if intersection is detected (or QR is detected)
        int trace = traceLine(fd, tracked, speed);
        if (trace == -1) {
            // when the bot arrives at an intersection
            // 1. update the current position
            // 2. make a decision here
            // 3. turn the bot. when the rotation is complete, then get back to loop
            updateCoord();
            decision = decide(tracked, curr_pos);
            decision = 1;
            int sockRes = sendSVR(sock, decision);             // send the decision to the server
            int turnRes = makeTurn(fd, decision);
        }

        /*     loop control     */

        counter++;          // increment the counter
        int buf_stop[2] = {0x02, 0};
        if (counter * INTERVAL / 1000 > DURATION) { 
            // break the loop after DURATION seconds
            printf("Terminate the loop\n");
            buf_stop[0] = 0x02;
            buf_stop[1] = 0;
            write(fd, buf_stop, 2);
            delay(1000);
            printf("========== Terminate ==========\n");
            break;
        }
    }

    return 0; // 정상 실행 종료.
}
