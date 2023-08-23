#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IP "127.0.0.1"
#define SIZE1 4
#define SIZE2 6

enum Colors {
    RE = 0, // RED
    BL = 1, // BLUE
    GR = 2, // GREEN
    YE = 3, // YELLOW
    WH = 4, // WHITE
    PU = 5, // PURPLE
    OR = 6, // ORANGE
    PI = 7, // PINK
    CY = 8, // CYAN
    BR = 9, // BROWN
    VI = 10, // VIOLET
    MA = 11, // MAGENTA
};

void print_board(const char board[SIZE1][SIZE2][15]) {
    printf("   ");
    for (int i = 1; i < SIZE2 + 1; i++)
        printf("%-3c", i + 64);
    printf("\n");
    for (int i = 0; i < SIZE1; i++) {
        printf("%d ", i + 1);
        for (int j = 0; j < SIZE2; j++) {
            printf(" \033[1;%s\033[0m", board[i][j]);
        }
        printf("\n");
    }
}

// Fisher-Yates algorithm
void shuffle_table(int table[], int size) {
    srand(time(NULL));
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = table[i];
        table[i] = table[j];
        table[j] = temp;
    }
}

void generate_board(char board[SIZE1][SIZE2][15]) {
    srand(time(NULL));

    int colors[24];
    for (int i = 0; i < 12; i++) {
        colors[i * 2] = i;
        colors[i * 2 + 1] = i;
    }
    shuffle_table(colors, 24);

    for (int i = 0; i < SIZE1; i++) {
        for (int j = 0; j < SIZE2; j++) {

            int color = colors[i * SIZE2 + j];
            switch (color) {
                case RE:
                    strcpy(board[i][j], "31mRE");
                    break;
                case BL:
                    strcpy(board[i][j], "34mBL");
                    break;
                case GR:
                    strcpy(board[i][j], "32mGR");
                    break;
                case YE:
                    strcpy(board[i][j], "33mYE");
                    break;
                case WH:
                    strcpy(board[i][j], "37mWH");
                    break;
                case PU:
                    strcpy(board[i][j], "35mPU");
                    break;
                case OR:
                    strcpy(board[i][j], "38;5;208mOR");
                    break;
                case PI:
                    strcpy(board[i][j], "38;5;205mPI");
                    break;
                case CY:
                    strcpy(board[i][j], "36mCY");
                    break;
                case BR:
                    strcpy(board[i][j], "38;5;130mBR");
                    break;
                case VI:
                    strcpy(board[i][j], "38;5;57mVI");
                    break;
                case MA:
                    strcpy(board[i][j], "38;5;201mMA");
                    break;
            }
        }
    }
}

int main() {
    char board[SIZE1][SIZE2][15];

    // communication with clients
    char buff[15];
    int sd,clen;
    struct sockaddr_in sad, //adres serwera
	                cad[2]; // adres klientów

    sd=socket(AF_INET,SOCK_DGRAM,0);
    memset((char *) &sad, 0,sizeof(sad));
    sad.sin_family=AF_INET;
    if( inet_pton( AF_INET, IP, & sad.sin_addr ) <= 0 ){
        perror( "inet_pton() ERROR" );
        exit(1);
    }
    sad.sin_port=htons((ushort) 5000);
    bind(sd,(struct sockaddr *) &sad, sizeof(sad));

    // players scores
    int score[2] = {0, 0};

    // games loop
    while(1){
        printf("Waiting for players...\n");
        for(int i = 0; i < 2; i++){
            clen=sizeof(cad[i]);
            recvfrom(sd,&buff,sizeof(int),0,(struct sockaddr *) &cad[i],&clen);
            printf("Player %d connected\n", i + 1);
        }

        generate_board(board);
        print_board(board);

        for(int i = 0; i < 2; i = !i){
            // send communicate who moves
            strcpy(buff, "start");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[i], sizeof(cad[i]));
            strcpy(buff, "wait");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));

            // get player choice
            char choice[15];
            recvfrom(sd, (char*) buff, sizeof(int), 0, (struct sockaddr *) &cad[i], &clen);
            strcpy(choice, buff);
            printf("Player %d choiced: %c %c\n", i, choice[0], choice[1]);
            // send coordinates to opponent player
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));

            // send cards to players
            char card1[15];
            strcpy(card1, board[choice[0] - '0'][choice[1] - '0']);
            sendto(sd, &card1, sizeof(card1), 0, (struct sockaddr *) &cad[i], sizeof(cad[i]));
            sendto(sd, &card1, sizeof(card1), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));

            // get player second choice
            recvfrom(sd, (char*) buff, sizeof(int), 0, (struct sockaddr *) &cad[i], &clen);
            strcpy(choice, buff);
            printf("Player %d choiced: %c %c\n", i, choice[0], choice[1]);
            // send coordinates to opponent player
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));

            // send cards to players
            char card2[15];
            strcpy(card2, board[choice[0] - '0'][choice[1] - '0']);
            sendto(sd, &card2, sizeof(card2), 0, (struct sockaddr *) &cad[i], sizeof(cad[i]));
            sendto(sd, &card2, sizeof(card2), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));

            // check if cards are the same
            if(strcmp(card1, card2) == 0){
                score[i]++;
                strcpy(buff, "same");
                // send communicate that cards are the same
                sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[i], sizeof(cad[i]));
                sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));
                // reverse to be reversed again into the same player
                i = !i;
            } else{
                strcpy(buff, "not");
                // send communicate that cards are not the same
                sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[i], sizeof(cad[i]));
                sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[!i], sizeof(cad[!i]));
            }
            // end of game
            if(score[0] + score[1] == 12){
                break;
            }
        }
        strcpy(buff, "end");
        sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[0], sizeof(cad[0]));
        sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[1], sizeof(cad[1]));
        if(score[0] > score[1]){
            strcpy(buff, "Wygrana!");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[0], sizeof(cad[0]));
            strcpy(buff, "Przegrana");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[1], sizeof(cad[1]));
        } else if(score[0] < score[1]){
            strcpy(buff, "Wygrana!");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[1], sizeof(cad[1]));
            strcpy(buff, "Przegrana");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[0], sizeof(cad[0]));
        } else{
            strcpy(buff, "Remis");
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[0], sizeof(cad[0]));
            sendto(sd, &buff, sizeof(buff), 0, (struct sockaddr *) &cad[1], sizeof(cad[1]));
        }
    }

    return 0;
}

