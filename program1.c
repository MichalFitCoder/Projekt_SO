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
    printf("test\n");
    int fd = open("P1_P2_fifo",O_WRONLY);
    //if(fd == -1) printf("mamy problem");
    while(1){
        printf("Podaj cos: \n");
        scanf("%s", buffer);
        printf("%s\n",buffer);

        write(fd,&buffer,sizeof(buffer));
        if(strcmp(buffer,"end") == 0) break;

    }
   

    close(fd);




    return 0;
}