#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "server.h"

sem_t mapLock;

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

void* sockListener(void* arg) {
    DGIST* dgist = (DGIST*)arg;
    int client_socket = dgist->players[0].socket;

    while(1) {
        if (read(client_socket, dgist, sizeof(DGIST)) == 0) {
            perror("Error reading from server");
            exit(-1);
        }
        printRcvd(dgist);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
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

    pthread_create(&tid, NULL, sockListener, &dgist);

    ClientAction action;
    action.row = -1;
    action.col = 0;

    printf("Enter row, col, action: \n");
    int row, col, act;
    scanf("%d %d %d", &row, &col, &act);
    while (1) {
        // if (action.row < 0) break;
        action.row = row;
        action.col = col;
        action.action = act == 0 ? move : setBomb;
        if (send(client_socket, &action, sizeof(ClientAction), 0) < 0) {
            perror("Error sending to server");
            exit(-1);
        }
    }
    close(client_socket);
    return 0;
}