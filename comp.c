#include <stdio.h>
#include <stdlib.h>

int main(){
    system("rm P1_P2_fifo");
    system("gcc -o program1 program1.c");
    system("./program1");
    return 0;
}