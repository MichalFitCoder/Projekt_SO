#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>


int main(){
    char buffer[100];


    mkfifo("P1_P2_fifo", 0666);
    int fd = open("P1_P2_fifo",O_RDONLY);
    while(1)
    {
        read(fd, buffer,sizeof(buffer));
        if(strcmp(buffer,"end") == 0) break;
        printf("Otrzymano: %s\n", buffer);
        
    }

    close(fd);    





    return 0;
}