#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>

volatile int flag = 1;
int c_user=0, fclient_write, fserver_read;
char users[50][256];

// Handler do zatrzymywania programu i wysylania informacji
void handler(int signum){
    setlogmask(LOG_UPTO(LOG_INFO));
    //flag = 0;
    close(fserver_read);
    printf("Server myChat end\n");
    syslog(LOG_INFO, "Server myChat end");
        for(int i=0;i<c_user;i++){
            if(strcmp(users[i]," ") != 0){
                fclient_write = open(users[i], O_WRONLY);
                if (write(fclient_write, "Server stopped", 255*sizeof(char)) < 0) {
                    perror("Write error");
                    exit(-1);
                }
                close(fclient_write);
            }
        }
    exit(0);
}

int server(){
    int fserver;
    char message[256], message2[512], topipe[256], not_found[256];
    char tab[3][128];
    int read_flag = 0;
    char deleter[256];
    openlog("Signal_Hadler", LOG_PID | LOG_CONS, LOG_USER);
    signal(SIGQUIT, handler);
    fserver = mkfifo("pipeServer", 0666);
    if (fserver < 0){
        perror("not created pipeServer\n");
        syslog(LOG_INFO, "not created pipeServer ");
        return -1;
    }
    else{
        printf("created pipeServer\n");
        syslog(LOG_INFO, "created pipeServer");
    }
        while(1){
            fserver_read = open("pipeServer", O_RDONLY);
            if (fserver_read < 0) {
                perror("server error (read)");
                return -1;
            }
            ssize_t bytes_read = read(fserver_read, message, 255*sizeof(char));
            if (bytes_read < 0) {
                perror("server.c Read error");
                syslog(LOG_INFO, "Read from server FIFO error");
                close(fserver_read);
                return -1;
            }
            close(fserver_read);
            if (bytes_read > 0){
                char *token;
                read_flag = 1;
                // Pobierz pierwszy token przed pierwszym znakiem ":"
                token = strtok(message, ":");
                if (token != NULL) {
                    strcpy(tab[0], token);
                }

                // Pobierz drugi token po pierwszym znaku ":"
                token = strtok(NULL, ":");
                if (token != NULL) {
                    strcpy(tab[1], token);
                }

                // Pobierz cały dalszy tekst po drugim znaku ":"
                token = strtok(NULL, "");
                if (token != NULL) {
                    strcpy(tab[2], token);
                }
            }

            if(read_flag == 1){
                sprintf(topipe, "pipe%s", tab[1]);
                fclient_write = open(topipe, O_WRONLY);
                if (fclient_write < 0) {
                    if(strncmp(tab[0],"created",7)==0){
                        char *token;
                        token = strtok(tab[0], " ");
                        token = strtok(NULL, "");
                        if (token != NULL){
                            strcpy(users[c_user], token);
                        }
                        printf("created %s\n",users[c_user]);
                        c_user++;
                    }else if(strncmp(tab[0],"deleted",7)==0){
                        printf("%s",tab[0]);
                        char *token;
                        token = strtok(tab[0], " ");
                        token = strtok(NULL, "");
                        if (token != NULL) {
                            strcpy(deleter, token);
                        }
                        for(int i=0;i<c_user;i++){
                            if(strncmp(users[i],deleter,strlen(deleter)-1) == 0){
                                strcpy(users[i], " ");
                                break;
                            }
                        }
                    }else{
                        sprintf(not_found,"There is no user like: %s",topipe);
                        perror(not_found);
                    }
                }else{
                    if(strncmp(tab[0],"deleted",7)==0){
                        printf("%s",tab[0]);
                        char *token;
                        token = strtok(tab[0], " ");
                        token = strtok(NULL, "");
                        if (token != NULL) {
                            strcpy(deleter, token);
                        }
                        for(int i=0;i<c_user;i++){
                            if(strncmp(users[i],deleter,strlen(deleter)-1) == 0){
                                strcpy(users[i], " ");
                                break;
                            }
                        }
                    }else{
                        sprintf(message2, "%s:%s", tab[0], tab[2]);
                        printf("From: %s\nTo: %s\nMessage: %s\n",tab[0],tab[1],tab[2]);
                        if (write(fclient_write, message2, 255*sizeof(char)) < 0) {
                                perror("Write error");
                                syslog(LOG_INFO, "Write to user FIFO error");

                                return -1;
                        }
                    }
                }
                close(fclient_write);
                read_flag = 0;
            }
        }
    //}
    closelog();
    return 0;
}