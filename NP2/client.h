//
// Created by kerwin on 12/9/21.
//

#ifndef NP2_CLIENT_H
#define NP2_CLIENT_H

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
#define SERVER_PORT 8002

int sock;
int thread_end=0;
int data_flag=0;
int match_flag=0;
int next_turn=0;
char data[128];
char player_name[128];
char username[128];
pthread_mutex_t data_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t match_mutex=PTHREAD_MUTEX_INITIALIZER;

void *send_sock(void *arg);
void *receive_sock(void *arg);
void playing(int socket);

#endif //NP2_CLIENT_H
