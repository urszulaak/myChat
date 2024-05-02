#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <ncurses.h>
#include <errno.h>

volatile int flagc = 0;

void handler2(int signum){
    setlogmask(LOG_UPTO(LOG_INFO));
    flagc = 1;
}

void end(char *name, char *info, int fserver_write){
    sprintf(info, "deleted %s\n",name);
    syslog(LOG_INFO, info);
    fserver_write = open("pipeServer", O_WRONLY);
    if (write(fserver_write, info, 255*sizeof(char)) < 0) {
        perror("Write error");
        close(fserver_write);
        exit(EXIT_FAILURE);  
    }
    close(fserver_write);
    fclose(stdout);
    exit(EXIT_SUCCESS);
}


int login(int argc, char **argv){
    int user = fork();
    int fclient_read, fserver_write, flag_start=1;
    char name[256], info[256];
    int ch;
    int fclient;
    char to[256], content[256], message[256], message2[512];
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
        initscr(); // Inicjalizacja ncurses
        cbreak();  // Włącz tryb cbreak, wyłącz linie buforowane
        noecho();  // Nie wyświetlaj wprowadzanych znaków na ekranie
        nodelay(stdscr, TRUE); // Ustaw tryb nieblokujący dla getch()
        fclient = mkfifo(name, 0666);
        if (fclient < 0){
            endwin();
            sprintf(info, "not created %s", name);
            perror(info);
            syslog(LOG_INFO, info);
            return -1;
        }
        else{
            sprintf(info, "created %s", name);
            printw("%s\n", info);
            fserver_write = open("pipeServer", O_WRONLY);
            if (write(fserver_write, info, 255*sizeof(char)) < 0) {
                perror("Write error");
                close(fserver_write);
                return -1;
            }
            close(fserver_write);
            syslog(LOG_INFO, info);
        }
        fclient_read = open(name, O_RDONLY | O_NONBLOCK);
        printw("Type 's' to send message or type 'd' to send file\n");
        while(1){
            if(flagc){
                endwin();
                wait();
                return 0;   
            }
            if ((ch = getch()) == 's') {
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
                    return -1;
                }
                sprintf(message, "%s:%s:%s", argv[2], to, content);
                if (write(fserver_write, message, 255*sizeof(char)) < 0) {
                    endwin();
                    perror("Write error");
                    close(fserver_write);
                    return -1;
                }
                close(fserver_write);
                nodelay(stdscr, TRUE);
                noecho();
            }
            if (fclient_read < 0) {
                endwin();
                perror("Fifo error (read)");
                return -1;
            }
            ssize_t bytes_read = read(fclient_read, message2, 255*sizeof(char));
            if (bytes_read < 0) {
                if(errno != EAGAIN && errno != EWOULDBLOCK){
                    endwin();
                    perror("Read error");
                    close(fclient_read);
                    syslog(LOG_INFO, "Blad przeslania wiadomosci");
                    return -1;
                }
            }   
            if (bytes_read > 0){
                printw("%s\n",message2);
                refresh();
            }
        }
        close(fclient_read);
    }
    closelog();
    return 0;
}