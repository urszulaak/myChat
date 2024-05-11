#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <ncurses.h>
#include <errno.h>
#include <sys/wait.h>

volatile int flagc = 0;

void handler2(int signum){
    setlogmask(LOG_UPTO(LOG_INFO));
    flagc = 1;
}

void createConsole(){
    initscr(); // Inicjalizacja ncurses
    cbreak();  // Włącz tryb cbreak, wyłącz linie buforowane
    noecho();  // Nie wyświetlaj wprowadzanych znaków na ekranie
    nodelay(stdscr, TRUE); // Ustaw tryb nieblokujący dla getch()
}

void createUser(int *fclient, char *name, char *info, int fserver_write, char *downloadPath){
    *fclient = mkfifo(name, 0666);
    if (*fclient < 0){
        endwin();
        sprintf(info, "not created %s", name);
        perror(info);
        syslog(LOG_INFO, "%s" ,info);
        exit(EXIT_FAILURE);
    }
    else{
        if(downloadPath){
            sprintf(info, "created %s?%s", name, downloadPath);
        }else{
            sprintf(info, "created %s", name);
        }
        printw("%s\n", info);
        fserver_write = open("pipeServer", O_WRONLY);
        if (write(fserver_write, info, 255*sizeof(char)) < 0) {
            perror("Write error");
            close(fserver_write);
            exit(EXIT_FAILURE);
        }
        close(fserver_write);
        syslog(LOG_INFO, "%s" ,info);
    }
}

void writeToServer(char *to, char *content, int fserver_write, char *message, char **argv){
    nodelay(stdscr, FALSE);
    echo();
    printw("To whom: ");
    refresh();
    getstr(to);
    printw("Message: ");
    refresh();
    getstr(content);
    fserver_write = open("pipeServer", O_WRONLY);
    if (fserver_write < 0) {
        endwin();
        perror("Fserver error (write)");
        exit(EXIT_FAILURE);
    }
    sprintf(message, "%s:%s:%s", argv[2], to, content);
    if (write(fserver_write, message, 255*sizeof(char)) < 0) {
        endwin();
        perror("Write error");
        syslog(LOG_INFO, "Write to server FIFO error");
        close(fserver_write);
        exit(EXIT_FAILURE);
    }
    close(fserver_write);
    nodelay(stdscr, TRUE);
    noecho();
}

void readFromUser(int fclient_read, char *message2){
    if (fclient_read < 0) {
        endwin();
        perror("Fifo error (read)");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read = read(fclient_read, message2, 255*sizeof(char));
    if (bytes_read < 0) {
        if(errno != EAGAIN && errno != EWOULDBLOCK){
            endwin();
            perror("Read error");
            close(fclient_read);
            syslog(LOG_INFO, "Read from user FIFO error");
            exit(EXIT_FAILURE);
        }
    }   
    if (bytes_read > 0){
        printw("%s\n",message2);
        refresh();
    }
}

void end(char *name, char *info, int fserver_write){
    sprintf(info, "deleted %s\n",name);
    syslog(LOG_INFO, "%s" ,info);
    fserver_write = open("pipeServer", O_WRONLY);
    if (write(fserver_write, info, 255*sizeof(char)) < 0) {
        perror("Write error");
        close(fserver_write);
        closelog();
        exit(EXIT_FAILURE);  
    }
    close(fserver_write);
    fclose(stdout);
    closelog();
    exit(EXIT_SUCCESS);
}

int login(int argc, char **argv, char *downloadPath){
    int user = fork();
    int fclient_read = -1, fserver_write = -1;
    char name[256], info[256];
    int ch;
    int fclient = -1;
    char to[256], content[256], message[512], message2[512];
    sprintf(name, "pipe%s", argv[2]);
    openlog("Signal_Hadler2", LOG_PID | LOG_CONS, LOG_USER);
    signal(SIGQUIT, handler2);
    if (user == -1){
        endwin();
        perror("Fork error");
        return -1;
    }else if(user == 0){
        while(1){
            if(flagc){
                end(name, info, fserver_write);
            }
        }
    }else{
        createConsole();
        createUser(&fclient, name, info, fserver_write, downloadPath);
        fclient_read = open(name, O_RDONLY | O_NONBLOCK);
        printw("Type 's' to send message or type 'd' to send file\n");
        while(1){
            if(flagc){
                endwin();
                waitpid(user, NULL, 0);
                return 0;   
            }
            if ((ch = getch()) == 's') {
                writeToServer(to, content, fserver_write, message, argv);
            } else if ((ch = getch()) == 'd') {
                 nodelay(stdscr, FALSE);
                echo();
                printw("To whom: ");
                refresh();
                getstr(to);
                printw("Enter the file path: ");
                refresh();
                getstr(content);
                sprintf(message, "SEND-%s-%s",to, content);
                fserver_write = open("pipeServer", O_WRONLY);
                if (fserver_write < 0) {
                    endwin();
                    perror("Fserver error (write)");
                    exit(EXIT_FAILURE);
                }
                if (write(fserver_write, message, sizeof(message)) < 0) {
                    endwin();
                    perror("Write error");
                    syslog(LOG_INFO, "Write to server FIFO error");
                    close(fserver_write);
                    exit(EXIT_FAILURE);
                }
                close(fserver_write);
                nodelay(stdscr, TRUE);
                noecho();
                }
            readFromUser(fclient_read, message2);
        }
        close(fclient_read);
    }
    return 0;
}
