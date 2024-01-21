//Sama logika programu dziala, trzeba teraz tylko dodac sygnaly do komunikacji miedzy procesami
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
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int id,fd;
#define fName "P1_P2_fifo"
sem_t *semP2_P3;
sem_t *semP3_P2;
int* buffer2;

int main(){
    char buffer[100]; // Zakladamy nie wiekszy rozmiar wiersza niz 100
    mkfifo(fName, 0666);

    //Otwieranie pliku
    FILE *plik;
    plik = fopen("dane.txt", "r");
    if (plik == NULL) {
        perror("Błąd przy otwieraniu pliku");
        return 1;
    }

    //Tworzenie pamieci wspoldzielonej
    int shm_id = shmget(IPC_PRIVATE, sizeof(sem_t) * 2 + sizeof(int), IPC_CREAT | 0666);
    
    //Dodanie semaforow i bufferu do pamieci wspoldzielonej
    semP2_P3 = (sem_t *)shmat(shm_id,NULL,0);
    semP3_P2 = semP2_P3 + 1;
    buffer2 = (int*)(semP3_P2 + 1);

    //Inicjalizacja semafora
    sem_init(semP2_P3, 1, 1);
    sem_init(semP3_P2, 1, 0);



    if(fork()){
        id = 0;
    }else{
        if(fork()){
            id = 1;
        }else{
            if(fork()){
                id = 2;
            }
            else
                id = 3;
        }
    }

    //Kod dla 1 procesu
    if(id == 1){

        printf("(1) Jestem procesem:  %d\n", getpid());
        fd = open(fName, O_WRONLY);
        if(fd == -1){
            printf("cos poszlo nie tak!");
            return 1;
        }

        while(fgets(buffer, sizeof(buffer), plik)) {
        
            write(fd, &buffer, sizeof(buffer));
        }
        //Wyslanie komunikatu o zakonczeniu czytania z pliku
        strcpy(buffer,"Koniec");
        write(fd, &buffer, sizeof(buffer));
        close(fd);

    }

    // Kod dla 2 procesu
    if(id == 2){
        printf("(2) Jestem procesem:  %d\n", getpid());
        int size = 0;
        fd = open(fName,O_RDONLY);

        if(fd == -1){
            printf("cos poszlo nie tak!");
            return 1;
        }
        while(1)
        { 
            read(fd, &buffer, sizeof(buffer));
            if(strcmp(buffer,"Koniec") == 0) break;
            size = strlen(buffer);
            sem_wait(semP2_P3);
            *buffer2 = size;
            sem_post(semP3_P2);
           
        }
        close(fd);
    }

    //Kod dla 3 procesu
    if(id == 3){
        printf("(3) Jestem procesem:  %d\n", getpid());
        while(1){
            sem_wait(semP3_P2);
            printf("Odebralem od P2: %d\n", *buffer2);
            sem_post(semP2_P3);
        }
    }

    // Kod procesu macierzystego
    if(id == 0){
        printf("(Macierzysty) Jestem procesem:  %d\n", getpid());

    }

    
    wait(NULL);


    return 0;
}