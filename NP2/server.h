//
// Created by kerwin on 12/7/21.
//
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>
#include<unistd.h>
#include<netdb.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#define PORT 8002
#define LOG 50
#ifndef NP2_SERVER_H
#define NP2_SERVER_H
char player_name[10][16] = {
        "STDIN","STDOUT","STDERR",
        "Socket","Sean","Kerwin",
        "Anna","Timmy","Kyle","Kao"
        };
int client_number, serverSocket, win_map[8];
short online[8], playing[10], mark[10];
pthread_mutex_t online_mutex = PTHREAD_MUTEX_INITIALIZER, in_game_mutex = PTHREAD_MUTEX_INITIALIZER;
struct match{
    char map[3][8];
    int player1, player2, count;
} game_map[10][10];
void *game_menu(void *arg);
#endif //NP2_SERVER_H
