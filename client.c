#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define SIZE1 4
#define SIZE2 6

void fill_board(char board[SIZE1][SIZE2][15]) {
    for (int i = 0; i < SIZE1; i++) {
        for (int j = 0; j < SIZE2; j++) {
            // board[i][j] = '▯';
            strcpy(board[i][j], "▯ ");
        }
    }
}

void print_board(char board[SIZE1][SIZE2][15]) {
    printf("   ");
    for (int i = 1; i <= SIZE2; i++) {
        printf("%-3c", i + 64);
    }
    printf("\n");
    for (int i = 0; i < SIZE1; i++) {
        printf("%d ", i + 1);
        for (int j = 0; j < SIZE2; j++) {
            // printf(" %s", board[i][j]);
            if(strcmp(board[i][j], "▯ ") == 0)
                printf(" %s", board[i][j]);
            else 
                printf(" \033[1;%s\033[0m", board[i][j]);
        }
        printf("\n");
    }
}

int convert_to_int(char c) {
    int tmp = c - '0';
    if (tmp > 0 && tmp < 5) return tmp - 1; //return 0 - 4
    if (c >= 'a' && c <= 'f') return tmp; // return 49 - 54 (a - f)
    if (c >= 'A' && c <= 'F') return tmp - 17 + 49; // return 49 - 54 (A - F)
    return -1;
}

int* get_coordinates(int last_coords[2], char board[SIZE1][SIZE2][15]) {
    int coordinates[2];
    char x, y;
    while (1) {
        printf("Podaj wspolrzedne: ");
        scanf(" %c %c", &x, &y);
        coordinates[0] = convert_to_int(y);
        coordinates[1] = convert_to_int(x);
        if ((coordinates[0] >= 49 && coordinates[0] <= 54) && (coordinates[1] >= 0 && coordinates[1] <= 4)) {
            coordinates[0] -= 49;
            //swap coordinates
            int tmp = coordinates[0];
            coordinates[0] = coordinates[1];
            coordinates[1] = tmp;
            if(coordinates[0] == last_coords[0] && coordinates[1] == last_coords[1]){
                printf("Nie mozesz wybrac tej samej karty!\n");
                continue;
            }
            if(strcmp(board[coordinates[0]][coordinates[1]], "▯ ") != 0){
                printf("Nie mozesz wybrac karty, ktora juz zostala odkryta!\n");
                continue;
            }
            return coordinates;
        }
        if ((coordinates[0] >= 0 && coordinates[0] <= 4) && (coordinates[1] >= 49 && coordinates[1] <= 54)) {
            coordinates[1] -= 49;
            if(coordinates[0] == last_coords[0] && coordinates[1] == last_coords[1]){
                printf("Nie mozesz wybrac tej samej karty!\n");
                continue;
            }
            if(strcmp(board[coordinates[0]][coordinates[1]], "▯ ") != 0){
                printf("Nie mozesz wybrac karty, ktora juz zostala odkryta!\n");
                continue;
            }
            return coordinates;
        }
        printf("Niepoprawne wspolrzedne!\n");
    }
}

int main() {

    char board[SIZE1][SIZE2][15];
    fill_board(board);
    // print_board(board);

    // communication with server
    char buff[15];
    int sd,clen,slen;
    struct sockaddr_in sad, //adres serwera
                    cad; // adres klienta

    sd=socket(AF_INET,SOCK_DGRAM,0);
    if (sd < 0){
        perror( "socket() ERROR" );
        exit(1);
    }    
        memset((char *) &sad, 0,sizeof(sad));
        sad.sin_family=AF_INET;
    if( inet_pton( AF_INET, IP, & sad.sin_addr ) <= 0 ){
        perror( "inet_pton() ERROR" );
        exit( 1 );
    }
    sad.sin_port=htons((ushort) 5000);
    cad.sin_family=AF_INET;
    cad.sin_port=htons(0);  // automatyczny wybor portu
    slen=sizeof(sad);

    int score = 0;
    strcpy(buff, "Hello");
    //connect to server
    sendto(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,slen);

    while(1){
        // receive info about whoose turn it is
        recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
        if(strcmp(buff, "end") == 0){
            printf("Koniec gry!\n");
            printf("Twoj wynik: %d\n", score);
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            printf("%s\n", buff);
            break;
        }
        if(strcmp(buff, "start") == 0){
            printf("Twoj ruch!\n");
            print_board(board);
            // send coordinates
            int last[2] = {-1, -1};
            int* coordinates = get_coordinates(last, board);
            last[0] = coordinates[0];
            last[1] = coordinates[1];
            char coords[2] = {coordinates[0]+'0', coordinates[1]+'0'};
            sendto(sd,&coords,sizeof(coords),0,(struct sockaddr*)&sad,slen);

            // receive card
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            strcpy(board[coordinates[0]][coordinates[1]], buff);
            print_board(board);

            // send second coordinates
            coordinates = get_coordinates(last, board);
            char coords2[2] = {coordinates[0]+'0', coordinates[1]+'0'};
            sendto(sd,&coords2,sizeof(coords2),0,(struct sockaddr*)&sad,slen);

            // receive card
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            strcpy(board[coords2[0]-'0'][coords2[1]-'0'], buff);
            print_board(board);

            // receive info if the same
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            if(strcmp(buff, "same") == 0){
                printf("Para! Masz kolejny ruch!\n");
                score++;
            } else{
                printf("Pudlo!\n");
                // reverse cards
                strcpy(board[coords[0]-'0'][coords[1]-'0'], "▯ ");
                strcpy(board[coords2[0]-'0'][coords2[1]-'0'], "▯ ");
            }
        } else{
            printf("Ruch przeciwnika!\n");
            print_board(board);
            // receive coordinates
            char coords[15];
            recvfrom(sd,&coords,sizeof(coords),0,(struct sockaddr*)&sad,&slen);
            // receive card
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            strcpy(board[coords[0]-'0'][coords[1]-'0'], buff);
            print_board(board);

            // receive coordinates
            char coords2[15];
            recvfrom(sd,&coords2,sizeof(coords),0,(struct sockaddr*)&sad,&slen);
            // receive card
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            strcpy(board[coords2[0]-'0'][coords2[1]-'0'], buff);
            print_board(board);

            // receive info if the same
            recvfrom(sd,&buff,sizeof(buff),0,(struct sockaddr*)&sad,&slen);
            if(strcmp(buff, "same") == 0){
                printf("Para!\n");
            } else{
                printf("Pudlo!\n");
                // reverse cards
                strcpy(board[coords[0]-'0'][coords[1]-'0'], "▯ ");
                strcpy(board[coords2[0]-'0'][coords2[1]-'0'], "▯ ");
            }
        }
        // wait 3 seconds
        sleep(2);
        system("clear");
    }
    return 0;
}