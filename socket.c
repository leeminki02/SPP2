#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "server.h"

sem_t mapLock;

#define GRID_SIZE 5
#define INTERVAL 5         // <- Interval time for each loop. unit: ms(miliseconds)
#define DURATION 6          // <- While loop duration. after (DURATION) seconds, the loop terminates. unit: seconds
#define INPUT 0

struct position {
    int x;      // x coord of the current position: min -1, max 5
    int y;      // y coord of the current position: min 0, max 4
    int dir;    // 0: RIGHT, 1: UP, 2: LEFT, 3: DOWN
};

// struct position curr_pos = {0, 0, 0};
// struct position oppo_pos = {0, 0, 0};
int player;
ClientAction action;
// struct DGIST receivedData;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point location;
    int score;
} Item_val;

int manhattanDistance(Point a, Point b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}


// Print the map
void printRcvd(void* arg) {
    DGIST* dgist = (DGIST*)arg;
    Item tmpItem;

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

int decide_loc(int tracked, DGIST receivedData) {
    // @minseok
    struct position curr_pos = {receivedData.players[player].row, receivedData.players[player].col, 0};
    int location = 0;
    Item_val bestItem;
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

    if (bestItem.location.x != -1 && bestItem.location.y != -1) {
        // 가로 방향으로 우선적으로 이동한다. 이동하려는 방향의 item이 trap이 아닌 경우
        // 가로 방향 우선 탐색
        if (bestItem.location.x > curr_pos.x 
                && receivedData.map[curr_pos.x + 1][curr_pos.y].item.status != trap) {
            printf("BestItem(%d, %d) Decision 0\n", bestItem.location.x, bestItem.location.y);
            return 0; // 오른쪽으로 이동
        } else if (bestItem.location.x < curr_pos.x 
                && receivedData.map[curr_pos.x - 1][curr_pos.y].item.status != trap) {
            printf("BestItem(%d, %d) Decision 2\n", bestItem.location.x, bestItem.location.y);
            return 2; // 왼쪽으로 이동
        }

        // 세로 방향 탐색
        if (bestItem.location.y > curr_pos.y 
                && receivedData.map[curr_pos.x][curr_pos.y + 1].item.status != trap) {
            printf("BestItem(%d, %d) Decision 1\n", bestItem.location.x, bestItem.location.y);
            return 1; // 위쪽으로 이동
        } else if (bestItem.location.y < curr_pos.y 
                && receivedData.map[curr_pos.x][curr_pos.y - 1].item.status != trap) {
            printf("BestItem(%d, %d) Decision 3\n", bestItem.location.x, bestItem.location.y);
            return 3; // 아래쪽으로 이동
        }

    }
    // 함정을 피할 수 없는 경우 랜덤하게 이동
    // return rand() % 4;
        // 0: right, 1: up, 2: left, 3: down
    if (curr_pos.x + 1 <= 4 && receivedData.map[curr_pos.x + 1][curr_pos.y].item.status != trap) {
        return 0;
    } else if (curr_pos.y + 1 <= 4 && receivedData.map[curr_pos.x][curr_pos.y + 1].item.status != trap) {
        return 1;
    } else if (curr_pos.x - 1 >= 0 && receivedData.map[curr_pos.x - 1][curr_pos.y].item.status != trap) {
        return 2;
    } else{
        return 3;
    }
    
    // return the value of the decision
    // 0: straight, 1: left, 2: down, 3: right
}

int decision_trap(int tracked, DGIST receivedData) {
    // 상대 플레이어 위치 확인
    int oppo_row = receivedData.players[1-player].row; // 0번 플레이어가 우리라고 가정
    int oppo_col = receivedData.players[1-player].col;
    // 현재 위치와 상대 플레이어 위치의 맨해튼 거리 계산
    int distance = abs(receivedData.players[player].row - oppo_row) + abs(receivedData.players[player].col - oppo_col);

    // 상대 플레이어가 1칸 이내에 있고, 남은 함정 개수가 1개 이상인 경우
    if (distance <= 1 && receivedData.players[player].bomb > 0) {
        return 1; // 함정 설치
    } else {
        return 0; // 함정 설치 안 함
    }
}

int decision;
int dec_trap;
struct position curr_pos = {0, 0, 0};

void* listener(void* arg) {
    DGIST* dgist = (DGIST*)arg;
    int client_socket = dgist->players[0].socket;

    while(1) {
        if (read(client_socket, dgist, sizeof(DGIST)) == 0) {
            perror("Error reading from server");
            exit(-1);
        }
        printRcvd(dgist);
        curr_pos.x = dgist->players[player].row;
        curr_pos.y = dgist->players[player].col;

        decision = decide_loc(INPUT, *dgist);
        dec_trap = decision_trap(INPUT, *dgist);
    }

    return NULL;
}



int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(-1);
    }

    int client_socket;
    struct sockaddr_in server_addr;
    pthread_t tid;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server\n");
        exit(-1);
    }

    DGIST dgist;
    dgist.players[0].socket = client_socket;

    pthread_create(&tid, NULL, listener, (void *)&dgist);


    player = atoi(argv[3]);
    if (player == 0) {
        action.row = 0;
        action.col = 0;
    } else {
        action.row = 4;
        action.col = 4;
    }
    action.action = 0;
    while (1) {
        // if (action.row < 0) break;
        // decision: 0 - right, 1 - up, 2 - left, 3 - down
        if (send(client_socket, &action, sizeof(ClientAction), 0) < 0) {
            perror("Error sending to server");
            exit(-1);
        }
        sleep(1);
        printf("Decision: %d\n", decision);
        printf("Trap: %d\n", dec_trap);
        switch(decision) {
            case 0:
                action.row = action.row + 1;
                action.col = action.col;
                break;
            case 1:
                action.row = action.row;
                action.col = action.col + 1;
                break;
            case 2:
                action.row = action.row - 1;
                action.col = action.col;
                break;
            case 3:
                action.row = action.row;
                action.col = action.col - 1;
                break;
        }
        switch(dec_trap){
            case 0:
                action.action = move;
                break;
            case 1:
                action.action = setBomb;
                break;
        }
    }
    close(client_socket);
    return 0;
}