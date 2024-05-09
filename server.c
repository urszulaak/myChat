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
#include <libgen.h>
#include <errno.h>

int c_user = 0, fclient_write = -1, fserver_read = -1;
char users[50][256];

void handler(int signum){
    setlogmask(LOG_UPTO(LOG_INFO));
    close(fserver_read);
    syslog(LOG_INFO, "Server myChat end");
    for(int i=0;i<c_user;i++){
        if(strcmp(users[i]," ") != 0){
            fclient_write = open(users[i], O_WRONLY);
            if (write(fclient_write, "Server stopped", strlen("Server stopped")) < 0) {
                perror("Write error");
                exit(EXIT_FAILURE);
            }
            close(fclient_write);
        }
    }
    exit(EXIT_SUCCESS);
}

void createServer(int *fserver){
    *fserver = mkfifo("pipeServer", 0666);
    if (*fserver < 0){
        perror("not created pipeServer\n");
        syslog(LOG_INFO, "not created pipeServer ");
        exit(EXIT_FAILURE);
    }
    else{
        syslog(LOG_INFO, "created pipeServer");
    }
}

void readFromServer(char *message, int *read_flag, char (*tab)[128]){
    fserver_read = open("pipeServer", O_RDONLY);
    if (fserver_read < 0) {
        perror("server error (read)");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read = read(fserver_read, message, 255*sizeof(char));
    if (bytes_read < 0) {
        perror("server.c Read error");
        syslog(LOG_INFO, "Read from server FIFO error");
        close(fserver_read);
        exit(EXIT_FAILURE);
    }
    close(fserver_read);
    if (bytes_read > 0){
        char *token;
        *read_flag = 1;
        token = strtok(message, ":");
        if (token != NULL) {
            strcpy(tab[0], token);
        }
        token = strtok(NULL, ":");
        if (token != NULL) {
            strcpy(tab[1], token);
        }
        token = strtok(NULL, "");
        if (token != NULL) {
            strcpy(tab[2], token);
        }
    }
}

void writeToUser(char *topipe, char *deleter, char *not_found, int *read_flag, char *message2, char (*tab)[128]){
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
            c_user++;
        }else if(strncmp(tab[0],"deleted",7)==0){
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
        }else if (strncmp(tab[0], "SEND", 4) == 0) {
            char *token;
            token = strtok(tab[0], ":");
            token = strtok(NULL, ":");
            char *filePath = token;
            token = strtok(NULL, ":");
            char *destDirPath = token;
            char *fileName = basename(filePath);
            char destFilePath[256];
            sprintf(destFilePath, "%s/%s", destDirPath, fileName);

            // Implement file copying
            FILE *source = fopen(filePath, "rb");
            FILE *destination = fopen(destFilePath, "wb");
            if (!source || !destination) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            char buffer[1024];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                fwrite(buffer, 1, bytesRead, destination);
            }

            fclose(source);
            fclose(destination);
            printf("File %s successfully copied to %s\n", fileName, destDirPath);
        }else{
            sprintf(not_found,"There is no user like: %s",topipe);
            perror(not_found);
        }
    }else{
        if(strncmp(tab[0],"deleted",7)==0){
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
        }else if (strncmp(tab[0], "SEND", 4) == 0) {
            char *token;
            token = strtok(tab[2], ":");
            token = strtok(NULL, ":");
            char *filePath = token;
            token = strtok(NULL, ":");
            char *destDirPath = token;
            char *fileName = basename(filePath);
            char destFilePath[256];
            sprintf(destFilePath, "%s/%s", destDirPath, fileName);

            // Implement file copying
            FILE *source = fopen(filePath, "rb");
            FILE *destination = fopen(destFilePath, "wb");
            if (!source || !destination) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            char buffer[1024];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                fwrite(buffer, 1, bytesRead, destination);
            }

            fclose(source);
            fclose(destination);
            printf("File %s successfully copied to %s\n", fileName, destDirPath);
        } else {
            sprintf(message2, "%s:%s", tab[0], tab[2]);
            if (write(fclient_write, message2, 255*sizeof(char)) < 0) {
                perror("Write error");
                syslog(LOG_INFO, "Write to user FIFO error");
                exit(EXIT_FAILURE);
            }
        }
    }
    close(fclient_write);
    *read_flag = 0;
}

int server(){
    int fserver = -1;
    char message[256], message2[512], topipe[256], not_found[256];
    char tab[3][128];
    int read_flag = 0;
    char deleter[256];
    openlog("Signal_Hadler", LOG_PID | LOG_CONS, LOG_USER);
    signal(SIGQUIT, handler);
    createServer(&fserver);
    while(1){
        readFromServer(message, &read_flag, tab);
        if(read_flag){
            writeToUser(topipe, deleter, not_found, &read_flag, message2, tab);
        }
    }
    closelog();
    return 0;
}
