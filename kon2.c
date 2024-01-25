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
#include <signal.h>
#include <bits/sigaction.h>
#include <asm-generic/signal-defs.h>
#include <sys/msg.h>


#define M_ODCZYT 1
#define STOP 19
#define GO 18

int id,fd;
#define fName "P1_P2_fifo"
sem_t *semP2_P3;
sem_t *semP3_P2;
int* buffer2;
int *msgid;
int *running;
int klawiatura;


struct message {
    long mtype;
    int val;
};


// Przeslanie sygnalu z P3 do PM
void handle_sigstpP3(int sig)
{
    if(sig == SIGTSTP){
        kill(getpid() - 3, SIGUSR1);
    }
    if(sig == SIGQUIT){
        kill(getpid() - 3, SIGUSR2);
    }
}


// Tworzenie kolejki komunikatow (PM)
void handle_sigPM(int sig)
{   
    struct message odczytP1;
    odczytP1.mtype = M_ODCZYT;
    if(sig == SIGUSR1){
        kill(getpid() + 1, STOP);
        odczytP1.val = 0;
        printf("Przerwano wczytywanie...\n");
    }
    if(sig == SIGUSR2){
        kill(getpid() + 1, GO);  
        odczytP1.val = 1;
    }

    *msgid = msgget((key_t)555, 0666 | IPC_CREAT); 
    if(*msgid == -1) perror("Blad przy tworzeniu kolejki");

    if((msgsnd(*msgid, (void *)&odczytP1, sizeof(struct message), 0)) == -1) perror("msg not sent");
    kill(getpid() + 1, SIGUSR1);

}


void Nothing(int sig){}


void handle_sigP1(int sig)
{   
    long int msg_to_rec = 0;
    struct message odczytZPM;
    *msgid = msgget((key_t)555, 0666 | IPC_CREAT); 
    msgrcv(*msgid, (void*)&odczytZPM, sizeof(struct message), msg_to_rec, 0);

    if(odczytZPM.val == 0);
        //kill(getpid() + 1, STOP);

    if(odczytZPM.val == 1);
        //kill(getpid() + 1, GO);
    
    //if((msgsnd(*msgid, (void *)&odczytZPM, sizeof(struct message), 0)) == -1) perror("msg not sent");

    kill(getpid() + 1, SIGUSR1);
}


void handle_sigP2(int sig)
{   
    long int msg_to_rec2 = 0;
    struct message odczytZP1;
    //*msgid = msgget((key_t)5556, 0666 | IPC_CREAT);
    //msgrcv(*msgid, (void*)&odczytZP1, sizeof(struct message), msg_to_rec2, 0);

    if(odczytZP1.val == 0);
        //kill(getpid() + 1, STOP);

    if(odczytZP1.val == 1);
        //kill(getpid() + 1, GO);

    //if((msgsnd(*msgid, (void *)&odczytZP1, sizeof(struct message), 0)) == -1) perror("msg not sent");
    kill(getpid() + 1, SIGUSR1);
}


void handle_sigP3(int sig)
{   
    printf("(3) Otrzymalem sygnal od P2 o wznowieniu dzialania\n");
    //*msgid = msgget((key_t)555, 0666 | IPC_CREAT); 
    long int msg_to_rec3 = 0;
    struct message odczytP3;
    //msgrcv(*msgid, (void*)&odczytP3, sizeof(struct message), msg_to_rec3, 0);

    if(odczytP3.val == 0);
        //kill(getpid() + 1, STOP);

    if(odczytP3.val == 1);
        //kill(getpid() + 1, GO);
}



int main()
{
    char buffer[200];

    int wybor;  // opcja wyboru klawaitura vs odczyt z pliku
    printf("#################  MENU  #################\n");
    printf("#  1 - klawiatura | 2 - odczyt z pliku  #\n");
    printf("==========================================\n");
    scanf("%d", &wybor);

    FILE *wejscie;

    if(wybor == 2) 
    {
        wejscie = fopen("wiedzmin.txt" , "r");

        if (wejscie == NULL) {
            perror("bład odczytu pliku\n");
            return 1;
        }

    }else if(wybor == 1)
    {
        klawiatura = 1;
    }else
    {
        printf("Podano nie prawidlowa wartosc\n");
    }

    mkfifo(fName, 0666);

  
    //Tworzenie pamieci wspoldzielonej
    int shm_id = shmget(IPC_PRIVATE, sizeof(sem_t) * 2 + 3 * sizeof(int), IPC_CREAT | 0666);

    if(shm_id == -1) printf("Blad przy tworzeniu pamieci wspoldzielonej\n");
    
    //Dodanie semaforow i bufferu do pamieci wspoldzielonej
    semP2_P3 = (sem_t *)shmat(shm_id,NULL,0);
    semP3_P2 = semP2_P3 + 1;
    buffer2 = (int*)(semP3_P2 + 1);
    msgid = (int*)(buffer2 + 1);
    running = (int*)(buffer2 + 1);

    *msgid = msgget((key_t)555, 0666 | IPC_CREAT); 
    if(*msgid == -1) perror("Blad przy tworzeniu kolejki");
    *running = 1;

    //Inicjalizacja semafora
    //Wartosci rowna 1 poniewaz pierwszy ma wejsc P2
    sem_init(semP2_P3, 1, 1); 
    sem_init(semP3_P2, 1, 0);

    int check = 0;
    //Tworzenie procesow P1 - P3
    if(check = fork()){
        if(check == -1 ) return 1;
        id = 0;
    }else{
        if(check = fork()){
            if(check == -1 ) return 1;
            id = 1;
        }else{
            if(check = fork()){
                if(check == -1 ) return 1;
                id = 2;
            }
            else
                id = 3;
        }
    }

    //Kod dla 1 procesu
    if(id == 1)
    {
        printf("(1) Jestem procesem:  %d\n", getpid());
        signal(SIGTSTP, &Nothing);
        signal(SIGUSR1, &handle_sigP1);
        signal(SIGQUIT, &Nothing);

        fd = open(fName, O_WRONLY);
        if(fd == -1){
            printf("cos poszlo nie tak!");
            return 1;
        }

        //Obsluga wejscia z klawiatury
        if(klawiatura == 1)
        {
            while(1)
            {
                while(!running){;}
                scanf("%s",buffer);
                write(fd, &buffer, sizeof(buffer));
            }
        }

        //Obsluga wejscia z pliku
        while(fgets(buffer, sizeof(buffer), wejscie))
        {
            write(fd, &buffer, sizeof(buffer));
            while(!running){;}
           
        }
        
        //Wyslanie komunikatu o zakonczeniu czytania z pliku
        strcpy(buffer,"Koniec");
        write(fd, &buffer, sizeof(buffer));
        close(fd);

    }

    // Kod dla 2 procesu
    if(id == 2)
    {

        printf("(2) Jestem procesem:  %d\n", getpid());
        signal(SIGTSTP, &Nothing);
        signal(SIGQUIT, &Nothing);
        signal(SIGUSR1, &handle_sigP2);
        int size = 0;
        fd = open(fName,O_RDONLY);

        if(fd == -1){
            printf("cos poszlo nie tak!");
            return 1;
        }
        while(1)
        {
            while(!running){;}
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
    if(id == 3)
    {
        int suma = 0;

        signal(SIGTSTP, &handle_sigstpP3);
        signal(SIGQUIT, &handle_sigstpP3);
        signal(SIGUSR1, &handle_sigP3);

        printf("(3) Jestem procesem:  %d\n", getpid());
        while(1)
        {
            while(!running){;}
            sem_wait(semP3_P2);
            printf("Odebralem od P2: %d\n", *buffer2);
            suma += *buffer2;
            printf("Odczytanych znakow lacznie: %d\n", suma);
            sem_post(semP2_P3);

        }
    }

    // Kod procesu macierzystego
    if(id == 0)
    {

        printf("(Macierzysty) Jestem procesem:  %d\n", getpid());
        signal(SIGTSTP, &Nothing);
        signal(SIGQUIT, &Nothing);
        signal(SIGUSR1, &handle_sigPM);
        signal(SIGUSR2, &handle_sigPM);
        while(1)
        {
            //Nieskonczona petla aby proces macierzysty sie nie skonczyl
        }

    }

    
    wait(NULL);

    // Zwalnianie uzytych zasobow

    // Usunięcie semaforów
    sem_close(semP2_P3);
    sem_close(semP3_P2);

    // Usunięcie pamięci współdzielonej
    shmctl(shm_id, IPC_RMID, NULL);

    // Usunięcie kolejki komunikatów
    msgctl(*msgid, IPC_RMID, NULL);

    // Usunięcie pliku FIFO
    unlink(fName);

    return 0;
}
