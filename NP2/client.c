#include "client.h"
static void sig_handler(int sig)
{
    char command[128];
    memset(command, 0, sizeof(command));
    if(sig==SIGINT){
        printf("\n\t***quit the game***\n\n");
        strcpy(command, "quit");
        write(sock, command, sizeof(command));
        close(sock);
        exit(0);
    }
}

int main()
{
    int connectok;
    struct sockaddr_in serverAddress;
    const char *serverIP = "127.0.0.1";
    unsigned short serverPort = SERVER_PORT;
    struct sigaction sa;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock<0){
        fprintf(stderr, "Error: socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    connectok = connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if(sigaction(SIGINT, &sa, NULL)==-1){
        fprintf(stderr, "Error: sigaction\n");
        exit(EXIT_FAILURE);
    }

    pthread_t receive_sock_t, send_sock_t;
    pthread_create(&receive_sock_t, NULL, receive_sock, (void*)&sock);
    pthread_create(&send_sock_t, NULL, send_sock, (void*)&sock);
    pthread_join(receive_sock_t, NULL);

    pthread_mutex_destroy(&data_mutex);

    close(sock);
    return 0;
}
void *send_sock(void *arg)
{
    int sock = *(int*)arg;
    char command[128];
    char response[128];
    FILE *fp = stdin;
    pthread_detach(pthread_self());

    printf("\n**************Welcome to the OO XX Game**************\n");
    printf("\tHere is the Game Menu\n");
    printf("CMD -> menu : Display the command you can use\n");
    printf("CMD -> list : Display the players who are online\n");
    printf("CMD -> match : Create the new room\n");
    printf("CMD -> quit : Exit the game\n");
    printf("********************************************************\n");

    while(1){
        printf("## Command : ");
        scanf("%s", command);

        /**
         * verify the CMD
         * menu
         * list
         * match
         * quit
         * Y
         * N
         */
        if(strcmp(command, "menu")==0){
            printf("\n\t***Here is the Game Menu***\n");
            printf("CMD -> menu : Display the command you can use\n");
            printf("CMD -> list : Display the players who are online\n");
            printf("CMD -> match : Create the new room\n");
            printf("CMD -> quit : Exit the game\n");
            printf("********************************************************\n");

        }
        else if(strcmp(command, "list")==0){
            write(sock, command, sizeof(command));
            pthread_mutex_lock(&data_mutex);
            data[strlen(data)-1]='\0';
            printf("\n**************player(s) online**************\n");
            printf("%s\n\n\n", data);
            pthread_mutex_unlock(&data_mutex);
            usleep(1000);
        }
        else if(strcmp(command, "match")==0){
            printf("Choose the one player below the list you want to invite : ");
            scanf("%s", player_name);
            strcat(command, " ");
            strcat(command, player_name);
            write(sock, command, sizeof(command));
            printf("\t\n Wait !!\n");
            pthread_mutex_lock(&data_mutex);
            pthread_mutex_unlock(&data_mutex);
            usleep(1000);
            if(match_flag) playing(sock);
            match_flag = 0;
        }
        else if(strcmp(command, "quit")==0){
            write(sock, command, sizeof(command));
            printf("\n\n\t*** Exit the game ***\n!!Wish you can have a girlfriend\n");
            break;
        }
        else if(strcmp(command, "Y")==0){
            int enter_flag = 0;
            pthread_mutex_lock(&match_mutex);
            if(match_flag){
                memset(response, 0, sizeof(response));
                strcpy(response, "Accept ");
                strcat(response, player_name);
                write(sock, response, sizeof(response));
                enter_flag = 1;
            }
            else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
            pthread_mutex_unlock(&match_mutex);
            usleep(1000);

            if(enter_flag) playing(sock);
            match_flag = 0;
        }
        else if(strcmp(command, "N")==0){
            pthread_mutex_lock(&match_mutex);
            if(match_flag){
                memset(response, 0, sizeof(response));
                strcpy(response, "Reject ");
                strcat(response, player_name);
                write(sock, response, sizeof(response));
            }
            else printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
            match_flag = 0;
            pthread_mutex_unlock(&match_mutex);
            usleep(1000);
        }
        else{
            printf("\n\t*** Error: Cannot find the command, please try again! ***\n");
        }
    }
}


void *receive_sock(void *arg)
{
    int sock = *(int*)arg;
    char *temp;
    while(1){
        pthread_mutex_lock(&data_mutex);
        read(sock, data, sizeof(data));
        if(strcmp(data, "quit")==0) break;
        else if(strncmp(data, "Invite", 6)==0){
            write(fileno(stdout), "\n", sizeof("\n"));
            write(fileno(stdout), data, sizeof(data));
            temp = strtok(data, " ");
            temp = strtok(NULL, " ");
            pthread_mutex_lock(&match_mutex);
            strcpy(player_name, temp);
            match_flag=1;
            pthread_mutex_unlock(&match_mutex);
            usleep(1000);
        }
        else if(strncmp(data, "Start", 5)==0){
            match_flag = 1;
            for(int i=0; i<40; i++) printf("*");
            printf("\n");
            temp = strtok(data, ";");
            printf("\t%s\n", temp);
            temp = strtok(NULL, ";");
            next_turn=0;
            if(strcmp(temp, username)==0) next_turn=1;
            temp = strtok(NULL, ";");
            printf("%s\n", temp);
        }
        else if(strncmp(data, "Reject", 6)==0){
            printf("%s\n\n", data);
        }
        else if(strcmp(data, "Win")==0){
            printf("\n\t*** You Win !! Congratulation !! ***\n\n");
        }
        else if(strcmp(data, "Lose")==0){
            printf("\n\t*** Yoe Lose !! You are such a looser !! ***\n\n");
        }
        else if(strncmp(data, "Even", 4)==0){
            printf("\n\t*** The game ended in a tie, try another game again !! ***\n\n");
        }
        else if(strncmp(data, "Leave", 5)==0){
            temp = strtok(data, ";");
            temp = strtok(NULL, ";");
            printf("%s\n", temp);
        }
        else if(strncmp(data, "username", 8)==0){
            temp = strtok(data, " ");
            temp = strtok(NULL, " ");
            memset(username, 0, sizeof(temp));
            strcpy(username, temp);
        }
        else if(strncmp(data, "Busy", 4)==0){
            printf("%s\n", data);
        }
        else if(strcmp(data, "error")==0){
            printf("\n\t*** Incorrect player name !! Please try again !!\n\n");
        }
        else if(strcmp(data, "Error")==0){
            printf("\n\t*** Incorrect input !! Please try again !!\n\n");
        }

        pthread_mutex_unlock(&data_mutex);
        usleep(100000);
    }
}

void playing(int socket)
{
    int next;
    char temp[2];
    char response[128];

    while(1){
        pthread_mutex_lock(&data_mutex);
        if(strcmp(data, "quit")==0 || strcmp(data, "Win")==0 || strcmp(data, "Lose")==0 || strncmp(data, "Even", 4)==0 || strncmp(data, "Leave", 5)==0){
            pthread_mutex_unlock(&data_mutex);
            break;
        }
        else if(next_turn){
            while(1){
                printf("enter the number to represent the position in the 3*3 table\n");
                printf("You turn (-1 is quit) : ");
                scanf("%d", &next);
                if(next>=1 && next<=9 || next==-1) break;
                else printf("*** Incorrect input !! Please enter the correct number(from  1 to 9) !!\n\n");
            }
            if(next ==-1 || strcmp(data, "quit")==0){
                printf("\n\t*** You leave the game !! ***\n");
                memset(response, 0, sizeof(response));
                strcpy(response, "Leave");
                write(socket, response, sizeof(response));
                pthread_mutex_unlock(&data_mutex);
                break;
            }
            memset(response, 0, sizeof(response));
            temp[1]='\0';
            temp[0]=next+48;
            strcpy(response, "Next;");
            strcat(response, temp);
            write(socket, response, sizeof(response));
        }
        pthread_mutex_unlock(&data_mutex);
        usleep(1000);
    }
}