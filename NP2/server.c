#include "server.h"

void sig_handler(int sig)
{
    if(sig==SIGINT){
        for(int i=0; i<10; i++){
            if(online[i]==1) write(i, "quit", sizeof("quit"));
        }
        usleep(1000);
        close(serverSocket);
        exit(0);
    }
}

int main()
{
    int yes;
    int clientSocket;
    struct sockaddr_in serverAddress;
    int listening;
    struct sigaction sa;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket<0){
        fprintf(stderr, "Error: socket\n");
        exit(0);
    }

    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
        fprintf(stderr, "Error: setsockopt()\n");
        exit(1);
    }

    if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress))<0){
        fprintf(stderr, "Error: bind(), use netstat -nlp to kill the process\n");
        exit(2);
    }

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &sa, NULL)==-1){
        fprintf(stderr, "Error: sigaction\n");
        exit(3);
    }

    listening = listen(serverSocket, LOG);
    if(listening<0){
        fprintf(stderr, "Error: listen\n");
        exit(4);
    }

    pthread_t t;
    pthread_attr_t attr;

    memset(playing, -1, sizeof(playing));
    memset(online, 0, sizeof(online));
    memset(mark, -1, sizeof(mark));
    pthread_attr_init(&attr);                                       // thread detached
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    win_map[0] = 146; win_map[1] = 292; win_map[2] = 584;
    win_map[3] = 14; win_map[4] = 112; win_map[5] = 896;
    win_map[6] = 546; win_map[7] = 168;

    while(1){
        clientSocket = accept(serverSocket, NULL, NULL);
        pthread_create(&t, &attr, game_menu, (void *)&clientSocket);
        pthread_mutex_lock(&online_mutex);
        online[clientSocket]=1;
        pthread_mutex_unlock(&online_mutex);
    }
    pthread_mutex_destroy(&online_mutex);
    close(serverSocket);

    return 0;
}

void *game_menu(void *arg)
{
    char buffer[128], response[128], list[128], *player, *temp, data[128], res_player1[128], res_player2[128], username[128];
    int clientSocket = *(int *)arg, player_sock;

    printf("%d\n", clientSocket);
    memset(username, 0, sizeof(username));
    strcpy(username, "username: ");
    strcat(username, player_name[clientSocket]);
    strcat(username, " ");
    write(clientSocket, username, sizeof(username));

    while(1){
        memset(buffer, 0, sizeof(buffer));
        read(clientSocket, buffer, sizeof(buffer));
        /**
         * cmp the socket buffer to verify what to do in the CMD
         * quit
         * list
         * Accept
         * Reject
         * match
         * Next
         * Leave6
         */
        if(strcmp(buffer, "quit")==0){
            //pthread_exit(NULL);
            write(clientSocket, buffer, sizeof(buffer));
            pthread_mutex_lock(&in_game_mutex);
            if(playing[clientSocket]>=0){
                memset(response, 0, sizeof(response));
                strcpy(response, "Leave;");
                strcat(response, "\t\n*** ");
                strcat(response, player_name[clientSocket]);
                strcat(response, " Leave the room !! ***\n;");

                write(playing[clientSocket], response, sizeof(response));
                playing[playing[clientSocket]]=-1;
                playing[clientSocket]=-1;
            }
            pthread_mutex_unlock(&in_game_mutex);
            usleep(1000);
            break;
        }
        else if(strcmp(buffer, "list")==0){
            memset(list, 0, sizeof(list));
            for(int i=0; i<10;i++){
                if(online[i]){
                    strcat(list, player_name[i]);
                    if(i==clientSocket) strcat(list, "(you)");
                    if(playing[i]>0){
                        strcat(list, "\t(Playing with ");
                        strcat(list, player_name[playing[i]]);
                        strcat(list, ")");
                    }
                    strcat(list, "\n");
                }
            }
            write(clientSocket, list, sizeof(list));
        }
        else if(strncmp(buffer, "Accept", 6)==0){
            temp = strtok(buffer, " ");
            temp = strtok(NULL, " ");
            player_sock = -1;
            for(int i=0; i<10; i++){
                if(strcmp(temp, player_name[i])==0){
                    player_sock = i;
                    break;
                }
            }
            pthread_mutex_lock(&in_game_mutex);
            playing[clientSocket] = (short)player_sock;
            playing[player_sock] = (short)clientSocket;
            pthread_mutex_unlock(&in_game_mutex);
            usleep(1000);

            bzero(res_player1,  sizeof(res_player1));
            bzero(res_player2,  sizeof(res_player2));
            strcpy(res_player1, "Start Game\nO : ");
            strcat(res_player1, player_name[clientSocket]);
            strcat(res_player1, "(you)\tX : ");
            strcat(res_player1, player_name[player_sock]);
            strcpy(res_player2, "Start Game\nO : ");
            strcat(res_player2, player_name[clientSocket]);
            strcat(res_player2, "\tX : ");
            strcat(res_player2, player_name[player_sock]);
            strcat(res_player2, "(you)");
            strcat(res_player1, ";");
            strcat(res_player1, player_name[clientSocket]);    // who turn
            strcat(res_player1, ";");
            strcat(res_player2, ";");
            strcat(res_player2, player_name[clientSocket]);    // who turn
            strcat(res_player2, ";");
            mark[clientSocket] = 0;
            mark[player_sock] = 1;
            int min_socket = clientSocket > player_sock ? player_sock : clientSocket;
            int max_socket = clientSocket > player_sock ? clientSocket : player_sock;

            /**
             * draw the 3*3 matrix
             */
            for(int i=0; i<3; i++){
                for(int j=0; j<3; j++){
                    game_map[min_socket][max_socket].map[i][j*2]='_';
                    game_map[min_socket][max_socket].map[i][j*2+1]=' ';
                    game_map[min_socket][max_socket].player1=0;
                    game_map[min_socket][max_socket].player2=0;
                }
                game_map[min_socket][max_socket].map[i][6]='\n';
                game_map[min_socket][max_socket].map[i][7]='\0';
                strcat(res_player1, game_map[min_socket][max_socket].map[i]);
                strcat(res_player2, game_map[min_socket][max_socket].map[i]);
            }
            game_map[min_socket][max_socket].count = 0;
            strcat(res_player1, ";");
            strcat(res_player2, ";");

            write(clientSocket, res_player1, sizeof(res_player1));
            write(player_sock, res_player2, sizeof(res_player2));
        }
        else if(strncmp(buffer, "Reject", 6)==0){
            temp = strtok(buffer, " ");
            temp = strtok(NULL, " ");
            player_sock = -1;
            for(int i=0; i<10; i++){
                if(strcmp(temp, player_name[i])==0){
                    player_sock = i;
                    break;
                }
            }
            memset(response, 0, sizeof(response));
            write(player_sock, "Reject !!", sizeof("Reject !!"));
        }
        else if(strncmp(buffer, "match", 5)==0){
            player=strtok(buffer, " ");
            player=strtok(NULL, " ");
            player_sock=-1;
            for(int i=0; i<10; i++){
                if(strcmp(player, player_name[i])==0 && online[i]){
                    player_sock=i;
                    break;
                }
            }
            if(player_sock<0){
                memset(response, 0, sizeof(response));
                strcpy(response, "error");
                write(clientSocket, response, sizeof(response));
            }
            else{
                pthread_mutex_lock(&in_game_mutex);
                if(playing[player_sock]<0){
                    memset(data, 0, sizeof(data));
                    printf("Successful !!\n");
                    strcpy(data, "Invite: ");
                    strcat(data, player_name[clientSocket]);
                    strcat(data, " invites you (Y/N) ? ");
                    write(player_sock, data, sizeof(data));
                    usleep(1000);
                }
                else{
                    memset(response, 0, sizeof(response));
                    strcpy(response, "Busy : ");
                    strcat(response, player);
                    strcat(response, " is in game now !! Please choose another player !!\n");
                    write(clientSocket, response, sizeof(response));
                }
                pthread_mutex_unlock(&in_game_mutex);
                usleep(1000);
            }
        }
        else if(strncmp(buffer, "Next", 4)==0){
            int location = -1;
            int min_socket = playing[clientSocket] > clientSocket ? clientSocket : playing[clientSocket];
            int max_socket = playing[clientSocket] > clientSocket ? playing[clientSocket] : clientSocket;
            char temp_mark =  mark[clientSocket]==0 ? 'O' : 'X';
            int win_flag = 0;
            char temp1;

            temp = strtok(buffer, ";");
            temp = strtok(NULL, ";");
            location = 1 << (temp[0] - 48);
            switch(temp[0]){
                case '1':
                    temp1 = game_map[min_socket][max_socket].map[0][0];
                    break;
                case '2':
                    temp1 = game_map[min_socket][max_socket].map[0][2];
                    break;
                case '3':
                    temp1 = game_map[min_socket][max_socket].map[0][4];
                    break;
                case '4':
                    temp1 = game_map[min_socket][max_socket].map[1][0];
                    break;
                case '5':
                    temp1 = game_map[min_socket][max_socket].map[1][2];
                    break;
                case '6':
                    temp1 = game_map[min_socket][max_socket].map[1][4];
                    break;
                case '7':
                    temp1 = game_map[min_socket][max_socket].map[2][0];
                    break;
                case '8':
                    temp1 = game_map[min_socket][max_socket].map[2][2];
                    break;
                case '9':
                    temp1 = game_map[min_socket][max_socket].map[2][4];
                    break;
            }
            if(temp1!='_'){
                write(clientSocket, "Error", sizeof("Error"));
                continue;
            }
            ++game_map[min_socket][max_socket].count;

            if(min_socket==clientSocket){
                game_map[min_socket][max_socket].player1 |= location;
                for(int i=0; i<8; i++){
                    if((game_map[min_socket][max_socket].player1 & win_map[i])==win_map[i]){
                        win_flag = 1;
                        write(min_socket, "Win", sizeof("Win"));
                        write(max_socket, "Lose", sizeof("Lose"));
                        playing[min_socket]=playing[max_socket]=-1;
                        break;
                    }
                }
            }
            else{
                game_map[min_socket][max_socket].player2 |= location;
                for(int i=0; i<8; i++){
                    if((game_map[min_socket][max_socket].player2 & win_map[i])==win_map[i]){
                        win_flag = 1;
                        write(min_socket, "Lose", sizeof("Lose"));
                        write(max_socket, "Win", sizeof("Win"));
                        playing[min_socket]=playing[max_socket]=-1;
                        break;
                    }
                }
            }
            if(win_flag) continue;
            else if(game_map[min_socket][max_socket].count >= 9){
                memset(response, 0, sizeof(response));
                strcpy(response, "Even;The game ended in a tie !!;");
                write(min_socket, response, sizeof(response));
                write(max_socket, response, sizeof(response));
                playing[min_socket]=playing[max_socket]=-1;
                continue;
            }

            memset(response, 0, sizeof(response));
            strcpy(response, "Start;");
            pthread_mutex_lock(&in_game_mutex);
            strcat(response, player_name[playing[clientSocket]]);
            strcat(response, ";");

            switch(temp[0]){
                case '1':
                    game_map[min_socket][max_socket].map[0][0] = temp_mark;
                    break;
                case '2':
                    game_map[min_socket][max_socket].map[0][2] = temp_mark;
                    break;
                case '3':
                    game_map[min_socket][max_socket].map[0][4] = temp_mark;
                    break;
                case '4':
                    game_map[min_socket][max_socket].map[1][0] = temp_mark;
                    break;
                case '5':
                    game_map[min_socket][max_socket].map[1][2] = temp_mark;
                    break;
                case '6':
                    game_map[min_socket][max_socket].map[1][4] = temp_mark;
                    break;
                case '7':
                    game_map[min_socket][max_socket].map[2][0] = temp_mark;
                    break;
                case '8':
                    game_map[min_socket][max_socket].map[2][2] = temp_mark;
                    break;
                case '9':
                    game_map[min_socket][max_socket].map[2][4] = temp_mark;
                    break;
            }

            for(int i=0; i<3; i++){
                strcat(response, game_map[min_socket][max_socket].map[i]);
            }
            strcat(response, ";");

            write(playing[clientSocket], response, sizeof(response));
            pthread_mutex_unlock(&in_game_mutex);
            write(clientSocket, response, sizeof(response));
        }
        else if(strcmp(buffer, "Leave")==0){
            int min_socket = playing[clientSocket] > clientSocket ? clientSocket : playing[clientSocket];
            int max_socket = playing[clientSocket] > clientSocket ? playing[clientSocket] : clientSocket;

            memset(response, 0, sizeof(response));
            strcat(response, "Leave;");
            strcat(response, "\t\n*** ");
            strcat(response, player_name[clientSocket]);
            strcat(response, " Leave the room !! ***\n;");
            write(playing[clientSocket], response, sizeof(response));
            playing[min_socket]=playing[max_socket]=-1;
        }
    }
    pthread_mutex_lock(&online_mutex);
    online[clientSocket]=0;
    pthread_mutex_unlock(&online_mutex);

    close(clientSocket);
    return NULL; //pthread_exit(NULL);
}

