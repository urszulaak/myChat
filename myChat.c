#include "server.h"
#include "login.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc,char** argv){
    if(argc==2){
        if(strcmp(argv[1],"--start") == 0 && argc==2){
        server();
        }else{
            perror("Invalid instruction");
            return -1;
        }
    }else if(argc==3){
        if(strcmp(argv[1],"--login") == 0){
        login(argc, argv);
        }else{
            perror("Invalid instruction");
            return -1;
        }
    }else{
        perror("Invalid instruction");
        return -1;
    }
    return 0;
}