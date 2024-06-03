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
sem_t mapLock;

// 모터 제어 함수들은 이 파일에 정의
#include "qr.h" // QR코드 인식 함수들은 이 파일에 정의
#define DEVICE_ID 0x16      // <- I2C device ID. This is the ID of connection.
#define GRID_SIZE 5
#define INTERVAL 5         // <- Interval time for each loop. unit: ms(miliseconds)
#define DURATION 6          // <- While loop duration. after (DURATION) seconds, the loop terminates. unit: seconds
#define INPUT 0

struct position {
    int x;      // x coord of the current position: min -1, max 5
    int y;      // y coord of the current position: min 0, max 4
    int dir;    // 0: RIGHT, 1: UP, 2: LEFT, 3: DOWN
};

struct position curr_pos = {0, 0, 0};
// struct position oppo_pos = {0, 0, 0};
int player;
struct DGIST receivedData;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point location;
    int score;
} Item;

int manhattanDistance(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

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

// Print the map
void printRcvd(void* arg) {
    DGIST* dgist = (DGIST*)arg;
    Item tmpItem;

    // DEBUG: seriously, this is just for debugging, so make sure you delete them.
	printf("\n\n===========MAP===========\n");
    sem_wait(&mapLock);
	for (int i = 0; i < MAP_ROW; i++) {
        printf("\t");
		for (int j = 0; j < MAP_COL; j++) {
            tmpItem = (dgist->map[i][j]).item;
            switch (tmpItem.status) {
                case nothing:
                    printf("- ");
                    break;
                case item:
                    printf("%d ", tmpItem.score);
                    break;
                case trap:
                    printf("x ");
                    break;
            }
        }
        printf("\n");
    }
    sem_post(&mapLock);


	client_info client;
	for(int i=0; i < MAX_CLIENTS; i++){
		client = dgist->players[i];
		printf("[Player %d]   ",i+1);
		printf("(%d,%d)\t",client.row, client.col);
		printf("Score: %d\t",client.score);
		printf("Bomb: %d\n",client.bomb);
	}
}

/* SOCKET - SERVER PART */
void* sockListener(void* arg) {
    DGIST* dgist = (DGIST*)arg;
    int client_socket = dgist->players[0].socket;

    while(1) {
        if (read(client_socket, dgist, sizeof(DGIST)) == 0) {
            perror("Error reading from server");
            exit(-1);
        }
        receivedData = *dgist;
        printRcvd(dgist);
    }

    return NULL;
}

int sendSVR(int sock, int decision){
    ClientAction action_decision;
    action_decision.col = curr_pos.x;
    action_decision.row = curr_pos.y;
    action_decision.action = decision == -1 ? setBomb : move;
    if (send(sock, action_decision, sizeof(ClientAction), 0) < 0){
        perror("Error sending to server");
        return -1;
    }
    if (decision == -1) {
        printf("Trap set at (%d, %d)\n", curr_pos.x, curr_pos.y);
    } else {
        printf("Position at (%d, %d)\n", curr_pos.x, curr_pos.y);
    }
    return 0;
}

int readQR() {
    // 강규영
    
    // QR 코드 인식 함수
    return qr_detector();
}

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

char buffer[5] = {0x01, 1, 200, 1, 200};
char old_buffer[5] = {0x01, 1, 200, 1, 200};
char buf_stop[2] = {0x02, 0};
int lineout_counter = 0;
int line_counter = 0;

int traceLine(int fd, int tracking, int shift) { //no shift = 0, robot left shift = 1, robot right shift = 2
    if (tracking == 0b1111){
        lineout_counter++;
    }
    else{
        lineout_counter = 0;
    }
    buffer[0] = 0x01;
    int res = 0;
    if(shift == 0){
        if (tracking == 0|| tracking == 1||tracking == 2||tracking == 4||tracking == 8||tracking == 10||tracking == 5||tracking == 6){ //0000 0001 0010 0100 1000 1010 0101 0110
            res = -1;
            printf(">>> INTERSECTION :[%d]\n", tracking);
        }
        else if (tracking == 13 || tracking == 12) { //1101 1100
            // turn right
            buffer[1] = 1;
            buffer[2] = 100;
            buffer[3] = 1;
            buffer[4] = 50;
            printf(">>> slight RIGHT\n");
        } else if (tracking == 14) { // 1110
            // turn right
            buffer[1] = 1;
            buffer[2] = 130;
            buffer[3] = 1;
            buffer[4] = 40;
            printf(">>> big RIGHT\n");
        } else if (tracking == 11||tracking == 3 ) { //1011 0011 
            // turn left
            buffer[1] = 1;
            buffer[2] = 50;
            buffer[3] = 1;
            buffer[4] = 100;
            printf(">>> slight LEFT\n");
        } else if (tracking == 7 ) { // 0111
            // turn left
            buffer[1] = 1;
            buffer[2] = 40;
            buffer[3] = 1;
            buffer[4] = 130;
            printf(">>> big LEFT\n");
        } else if (tracking == 15 && lineout_counter<250) { //1111
            buffer[1] = old_buffer[1];
            buffer[2] = old_buffer[2];
            buffer[3] = old_buffer[3];
            buffer[4] = old_buffer[4];
            printf(">>> lineout : go\n");
        }
        else if (tracking == 15){ //1111
            buffer[1] = 0;
            buffer[2] = 40;
            buffer[3] = 0;
            buffer[4] = 40;
            printf(">>> line out : BACK\n");
        }
        else if (tracking == 9) {//1001
            // go straight
            buffer[1] = 1;
            buffer[2] = 80;
            buffer[3] = 1;
            buffer[4] = 80;
            printf(">>> STRAIGHT\n");
        }   
    }
    else if(shift == 1){ // left shift
        if (line_counter>40 && tracking == 7||tracking == 3||tracking == 0|| tracking == 1||tracking == 2||tracking == 4||tracking == 8||tracking == 10||tracking == 5||tracking == 6){ //0000 0001 0010 0100 1000 1010 0101 0110
            res = -1;
            printf(">>> INTERSECTION :[%d]\n", tracking);
            line_counter = 0;
        }
        else if (tracking == 12) { //1101 1100
            // turn right
            buffer[1] = 1;
            buffer[2] = 100;
            buffer[3] = 1;
            buffer[4] = 50;
            printf(">>> slight RIGHT\n");
        } else if (tracking == 14) { // 1110
            // turn right
            buffer[1] = 1;
            buffer[2] = 130;
            buffer[3] = 1;
            buffer[4] = 40;
            printf(">>> big RIGHT\n");
        } else if (tracking == 11||tracking == 3 ) { //1011 0011 
            // turn left
            buffer[1] = 1;
            buffer[2] = 50;
            buffer[3] = 1;
            buffer[4] = 100;
            printf(">>> slight LEFT\n");
        } else if (tracking == 7 ) { // 0111
            // turn left
            buffer[1] = 1;
            buffer[2] = 40;
            buffer[3] = 1;
            buffer[4] = 130;
            printf(">>> big LEFT\n");
        } else if (tracking == 15 && lineout_counter<250) { //1111
            buffer[1] = old_buffer[1];
            buffer[2] = old_buffer[2];
            buffer[3] = old_buffer[3];
            buffer[4] = old_buffer[4];
            printf(">>> lineout : go\n");
        }
        else if (tracking == 15){ //1111
            buffer[1] = 0;
            buffer[2] = 40;
            buffer[3] = 0;
            buffer[4] = 40;
            printf(">>> line out : BACK\n");
        }
        else if (tracking == 13 ||tracking == 9) {//1001 1101
            // go straight
            buffer[1] = 1;
            buffer[2] = 70;
            buffer[3] = 1;
            buffer[4] = 80;
            printf(">>> STRAIGHT\n");
        }   
    }
    else if(shift == 2){
                if (line_counter>40 && tracking == 14||tracking == 12||tracking == 0|| tracking == 1||tracking == 2||tracking == 4||tracking == 8||tracking == 10||tracking == 5||tracking == 6){ //0000 0001 0010 0100 1000 1010 0101 0110
            res = -1;
            printf(">>> INTERSECTION :[%d]\n", tracking);
            line_counter = 0;
        }
        else if (tracking == 13 || tracking == 12) { //1101 1100
            // turn right
            buffer[1] = 1;
            buffer[2] = 100;
            buffer[3] = 1;
            buffer[4] = 50;
            printf(">>> slight RIGHT\n");
        } else if (tracking == 14) { // 1110
            // turn right
            buffer[1] = 1;
            buffer[2] = 130;
            buffer[3] = 1;
            buffer[4] = 40;
            printf(">>> big RIGHT\n");
        } else if (tracking == 3 ) { // 0011 
            // turn left
            buffer[1] = 1;
            buffer[2] = 50;
            buffer[3] = 1;
            buffer[4] = 100;
            printf(">>> slight LEFT\n");
        } else if (tracking == 7 ) { // 0111
            // turn left
            buffer[1] = 1;
            buffer[2] = 40;
            buffer[3] = 1;
            buffer[4] = 130;
            printf(">>> big LEFT\n");
        } else if (tracking == 15 && lineout_counter<250) { //1111
            buffer[1] = old_buffer[1];
            buffer[2] = old_buffer[2];
            buffer[3] = old_buffer[3];
            buffer[4] = old_buffer[4];
            printf(">>> lineout : go\n");
        }
        else if (tracking == 15){ //1111
            buffer[1] = 0;
            buffer[2] = 40;
            buffer[3] = 0;
            buffer[4] = 40;
            printf(">>> line out : BACK\n");
        }
        else if (tracking == 11 ||tracking == 9) {//1001 1011
            // go straight
            buffer[1] = 1;
            buffer[2] = 80;
            buffer[3] = 1;
            buffer[4] = 70;
            printf(">>> STRAIGHT\n");
        }   

    }
    write(fd, buffer, 5);
    old_buffer[1] = buffer[1];
    old_buffer[2] = buffer[4];
    old_buffer[3] = buffer[3];
    old_buffer[4] = buffer[2];
    delay(INTERVAL);    // delay loop for INTERVAL miliseconds
    return res; // res = -1 for intersection, 0 for rest
    
}

int makeTurn(int fd, int decision) { //left = 0, up = 1, left = 2, uturn= 3
    int tracking = 0;
    int turn_count = 0;
    int real_turn = 0; 
    char buffer[] = {0x01, 1, 200, 1, 200};
    while (1){
        switch(curr_pos.dir){
            case 0:
                real_turn = (decision + 1) % 4;
            case 1:
                real_turn = decision;
            case 2:
                real_turn = (decision + 3) % 4;
            case 3:
                real_turn = (decision + 2) % 4;
        }
        if (real_turn == 0){
            buffer[1] = 0;
            buffer[2] = 70;
            buffer[3] = 1;
            buffer[4] = 70;
        }
        else if(real_turn == 2){
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
        if (real_turn != 2){
            tracking = getTraceInfo(fd);
            if ((tracking != 0b1111) &&turn_count>60){//150-20
                printf("turn_count :%d",turn_count);
                break;
            }
            write(fd, buffer, 5);
            delay(5);
            turn_count ++;
            printf(">>> turning\n");
            if(turn_count > 200){//150-80
                printf("kill!!!!!!!!!!!!");
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
            printf(">>> turning\n");
            if(turn_count > 400){
                printf("kill!!!!!!!!!!!!!1");
                return tracking;
            }

        }
    }
    return tracking;
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

    // // 좌표가 범위를 벗어나지 않도록 확인
    // if (curr_pos.x < -1) curr_pos.x =-1;
    // if (curr_pos.x >  5) curr_pos.x = 5;
    // if (curr_pos.y <  0) curr_pos.y = 0;
    // if (curr_pos.y >  4) curr_pos.y = 4;

    printf("Updated position: (%d, %d, %d)\n", curr_pos.x, curr_pos.y, curr_pos.dir);
}

int decide_loc(int tracked, DGIST receivedData, struct position curr_pos) {
    // @minseok
    int location = 0;
    Item bestItem;
    bestItem.score = 0;
    bestItem.location.x = -1;
    bestItem.location.y = -1;
    
    int minDistance = 65535;
    // 모든 좌표 탐색하여 가장 가까운 아이템 중 가장 크기가 큰 아이템을 best 아이템으로 선정
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (receivedData.map[x][y].item.status == item) {
                int distance = manhattanDistance((Point){curr_pos.x, curr_pos.y}, (Point){x, y});
                if (distance < minDistance || (distance == minDistance && receivedData.map[x][y].item.score > bestItem.score)) {
                    minDistance = distance;
                    bestItem.score = receivedData.map[x][y].item.score;
                    bestItem.location = (Point){x, y};
                }
            }
        }
    }
    // 가로 방향으로 우선적으로 이동한다. 이동하려는 방향의 item이 trap이 아닌 경우
    // 가로 방향 우선 탐색
    if (bestItem.location.x > curr_pos.x && receivedData.map[curr_pos.x + 1][curr_pos.y].item.status != trap) {
        return 0; // 오른쪽으로 이동
    } else if (bestItem.location.x < curr_pos.x && receivedData.map[curr_pos.x - 1][curr_pos.y].item.status != trap) {
        return 2; // 왼쪽으로 이동
    }

    // 세로 방향 탐색
    if (bestItem.location.y > curr_pos.y && receivedData.map[curr_pos.x][curr_pos.y + 1].item.status != trap) {
        return 1; // 위쪽으로 이동
    } else if (bestItem.location.y < curr_pos.y && receivedData.map[curr_pos.x][curr_pos.y - 1].item.status != trap) {
        return 3; // 아래쪽으로 이동
    }

    // 함정을 피할 수 없는 경우 랜덤하게 이동
    // return rand() % 4;
        // 0: right, 1: up, 2: left, 3: down
    if (curr_pos.x + 1 <= 4) {
        return 0;
    } else if (curr_pos.y + 1 <= 4) {
        return 1;
    } else if (curr_pos.x - 1 >= 0) {
        return 2;
    } else if (curr_pos.y - 1 >= 0) {
        return 3;
    }
    
    // return the value of the decision
    // 0: straight, 1: left, 2: down, 3: right
}

int decision_trap(int tracked, DGIST receivedData, struct position curr_pos) {
    // 상대 플레이어 위치 확인
    int oppo_row = receivedData.players[1-player].row; // 0번 플레이어가 우리라고 가정
    int oppo_col = receivedData.players[1-player].col;
    oppo_pos.x = oppo_row;
    oppo_pos.y = oppo_col;
    // 현재 위치와 상대 플레이어 위치의 맨해튼 거리 계산
    int distance = abs(curr_pos.x - oppo_row) + abs(curr_pos.y - oppo_col);

    // 상대 플레이어가 1칸 이내에 있고, 남은 함정 개수가 1개 이상인 경우
    if (distance <= 1 && receivedData.players[player].bomb > 0) {
        return 1; // 함정 설치
    } else {
        return 0; // 함정 설치 안 함
    }
}

int main(int argc, char *argv[]){
    /*    USER INPUT    */
    // get the port number from user -> portNo
    // get the starting position from user as 0 or 1 -> player
    // get the first coordinate from argument
    if (argc != 3) {
        printf("Usage: %s <server_ip> <server_port> <startPosition: 0/1>\n", argv[0]);
        exit(-1);
    }
    int portNo = atoi(argv[2]);
    player = atoi(argv[3]);

    /*     INITIAL CONFIG    */
    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t tid;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNo);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server\n");
        exit(-1);
    }

    DGIST dgist;
    dgist.players[0].socket = client_socket;

    pthread_create(&tid, NULL, sockListener, &dgist);

    int fd = gpio_init();
    /* DECLARE VARIABLES */
    int speed = 200;
    int decision_pos, trap_decision;

    if (player == 0) {
        curr_pos.x = -1;
        curr_pos.y = 0;
        curr_pos.dir = 0;
    } else if (player == 1) {
        curr_pos.x = 5;
        curr_pos.y = 4;
        curr_pos.dir = 2;
    } else {
        printf("Invalid argument\n");
        return -1;
    }
    printf("[INIT] position: %d (%d, %d, %d)\n", player, curr_pos.x, curr_pos.y, curr_pos.dir);

    if (fd == -1) {
        printf("Error: GPIO initialization failed: fd[%d]\n", fd);
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
            decision_dir = decide_loc(tracked, curr_pos);
            trap_decision = decision_trap(tracked, receivedData, curr_pos);
            int sockRes = sendSVR(client_socket, trap_decision, curr_pos);             // send the decision to the server
            int turnRes = makeTurn(fd, decision_dir);
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
