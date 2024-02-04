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

int przerwac;
int id,fd;
#define fName "P1_P2_fifo"
sem_t *semP2_P3;
sem_t *semP3_P2;
int* buffer2;
int running = 1;
int klawiatura;
FILE *wejscie;
int PM_PID;
int P1_PID;
int P2_PID;
int P3_PID;

struct message {
    long mtype;
    int val;
};


// Przeslanie sygnalu z P3 do PM
void handle_sigstpP3(int sig)
{
    FILE *pidFile;
    char PMid[10];

        pidFile = fopen("pids.txt", "r");  // Otwórz plik do odczytu

        if (pidFile == NULL) {
            perror("Błąd przy otwieraniu pliku");
            exit(1);
        }

        // Odczytywanie pierwszej lini z pliku (pid PM)
        if (fgets(PMid, sizeof(PMid), pidFile) != NULL) {
            sscanf(PMid, "%d", &PM_PID);
        }
        fclose(pidFile); 

        if(sig == SIGTSTP){
            kill(PM_PID, SIGUSR1);
        }
        if(sig == SIGCONT){
            kill(PM_PID, SIGUSR2);
        }

}


void handle_end(int sig)
{
    //Odczytanie id procesu 1
    FILE *pidFile;
    char P1id[10];
    pidFile = fopen("pids.txt", "r");
        
    if (pidFile == NULL) 
    {
        perror("Błąd przy otwieraniu pliku");
        exit(1);
    }

    for(int i = 0 ; i < 4; i ++)
    {
        if (fgets(P1id, sizeof(P1id), pidFile) != NULL) {
            sscanf(P1id, "%d", &P1_PID);
        }
        if(i == 1) kill(P1_PID, SIGUSR2); // P1
        if(i == 0) kill(P1_PID, SIGTERM); // PM
        //if(i == 2) kill(P1_PID, SIGINT); // P2
        //if(i == 3) kill(P1_PID, SIGINT); // P3
    }

    fclose(pidFile);
}


// Tworzenie kolejki komunikatow (PM)
void handle_sigPM(int sig)
{   
    if(sig == SIGTERM){
        przerwac = 1;
        return;
    }

    //Odczytanie id procesu 1
    FILE *pidFile;
    char P1id[10];
    pidFile = fopen("pids.txt", "r");
        
    if (pidFile == NULL) 
    {
        perror("Błąd przy otwieraniu pliku");
        exit(1);
    }

    for(int i = 0 ; i < 2; i ++)
    {
        if (fgets(P1id, sizeof(P1id), pidFile) != NULL) {
            sscanf(P1id, "%d", &P1_PID);
        }
    }
    fclose(pidFile);

    struct message odczytP1;
    odczytP1.mtype = M_ODCZYT;

    if(sig == SIGUSR1){
        odczytP1.val = 0;
    }
    if(sig == SIGUSR2){ 
        odczytP1.val = 1;
    } 

    
    int msgid = msgget((key_t)1200, 0666 | IPC_CREAT); 
    if(msgid == -1) perror("Blad przy tworzeniu kolejki");

    if((msgsnd(msgid, 
    (void *)&odczytP1, sizeof(struct message), 0)) == -1) perror("msg not sent");
    kill(P1_PID, SIGUSR1); 

    running = odczytP1.val;

}


void Nothing(int sig){}


void handle_sigP1(int sig)
{   
    //Obsluga zakanczania programu
    if(sig == SIGUSR2)
    {
        przerwac = 1;
        return;
    }

    //Odczytanie id procesu 2
    FILE *pidFile;
    char P2id[10];
    pidFile = fopen("pids.txt", "r");
        
    if (pidFile == NULL) 
    {
        perror("Błąd przy otwieraniu pliku");
        exit(1);
    }

    for(int i = 0 ; i < 3; i ++)
    {
        if (fgets(P2id, sizeof(P2id), pidFile) != NULL) {
            sscanf(P2id, "%d", &P2_PID);
        }
    }
    fclose(pidFile);
    


    long int msg_to_rec = 0;
    //przerwac = 1;
    struct message odczytZPM;
    int msgid = msgget((key_t)1200, 0666 | IPC_CREAT); 
    msgrcv(msgid, (void*)&odczytZPM, sizeof(struct message), msg_to_rec, 0);
    
    if((msgsnd(msgid, (void *)&odczytZPM, sizeof(struct message), 0)) == -1) perror("msg not sent");

    kill(P2_PID, SIGUSR1);

    running = odczytZPM.val;
}


void handle_sigP2(int sig)
{   
    //Odczytanie id procesu 3
    FILE *pidFile;
    char P3id[10];
    pidFile = fopen("pids.txt", "r");
        
    if (pidFile == NULL) 
    {
        perror("Błąd przy otwieraniu pliku");
        exit(1);
    }

    for(int i = 0 ; i < 4; i ++)
    {
        if (fgets(P3id, sizeof(P3id), pidFile) != NULL) {
            sscanf(P3id, "%d", &P3_PID);
        }
    }
    fclose(pidFile);

    long int msg_to_rec2 = 0;
    struct message odczytZP1;
    int msgid = msgget((key_t)1200, 0666 | IPC_CREAT);
    msgrcv(msgid, (void*)&odczytZP1, sizeof(struct message), msg_to_rec2, 0);

    if((msgsnd(msgid, (void *)&odczytZP1, sizeof(struct message), 0)) == -1) perror("msg not sent");
    kill(P3_PID, SIGUSR1);

    running = odczytZP1.val;
}


void handle_sigP3(int sig)
{   

    int msgid = msgget((key_t)1200, 0666 | IPC_CREAT); 
    long int msg_to_rec3 = 0;
    struct message odczytP3;
    msgrcv(msgid, (void*)&odczytP3, sizeof(struct message), msg_to_rec3, 0);

    running = odczytP3.val;
    
}


int main()
{
    char buffer[200];
    mkfifo(fName, 0666);
  
    //Tworzenie pamieci wspoldzielonej
    int shm_id = shmget(IPC_PRIVATE, sizeof(sem_t) * 2 +  sizeof(int), IPC_CREAT | 0666);

    if(shm_id == -1) printf("Blad przy tworzeniu pamieci wspoldzielonej\n");
    
    //Dodanie semaforow i bufferu do pamieci wspoldzielonej
    semP2_P3 = (sem_t *)shmat(shm_id,NULL,0);
    semP3_P2 = semP2_P3 + 1;
    buffer2 = (int*)(semP3_P2 + 1);


    //Wartosci rowna 1 poniewaz pierwszy ma wejsc P2
    sem_init(semP2_P3, 1, 1); 
    sem_init(semP3_P2, 1, 0);

    int check = 0;
    //Tworzenie procesow P1 - P3
    FILE *pidFile;
    pidFile = fopen("pids.txt", "w");  // Otwórz plik do zapisu

    if (pidFile == NULL) {
        perror("Błąd przy otwieraniu pliku");
        return 1;
    }

    if (check = fork()) {
        if (check == -1)
            return 1;
        id = 0;

        PM_PID = getpid();  // Zapisz PID procesu macierzystego
        fprintf(pidFile, "%d\n", PM_PID);
    } else {
        if (check = fork()) {
            if (check == -1)
                return 1;
            id = 1;
            P1_PID = getpid();  // Zapisz PID procesu P1
            fprintf(pidFile, "%d\n", P1_PID);
        } else {
            if (check = fork()) {
                if (check == -1)
                    return 1;
                id = 2;
                P2_PID = getpid();  // Zapisz PID procesu P2
                fprintf(pidFile, "%d\n", P2_PID);
            } else {
                id = 3;
                P3_PID = getpid();  // Zapisz PID procesu P3
                fprintf(pidFile, "%d\n", P3_PID);
            }
        }
    }

    fclose(pidFile);

    //Kod dla 1 procesu
    if(id == 1)
    {
        
        signal(SIGCONT, &Nothing);
        signal(SIGTSTP, &Nothing);
        signal(SIGUSR1, &handle_sigP1);
        signal(SIGUSR2, &handle_sigP1);
        signal(SIGQUIT, &Nothing);

        fd = open(fName, O_WRONLY);
        if(fd == -1){
            printf("cos poszlo nie tak!");
            return 1;
        }

        int wybor; // opcja wyboru klawiatura vs odczyt z pliku
        char wyborstr[10];
        char name[50];
        while(1)
        {
            if(przerwac) break;
            
            //Ogarnac wyswietlanie
            printf("#################  MENU  #################\n");
            printf("#  1 - klawiatura | 2 - odczyt z pliku  #\n");
            printf("==========================================\n");

            fgets(wyborstr, sizeof(wyborstr), stdin);
            wybor = atoi(wyborstr);

            if(wybor == 2) 
            {
                klawiatura = 0;

                printf("Prosze podac nazwe pliku: ");
                fgets(name, sizeof(name), stdin);
                size_t len = strlen(name);
                if (len > 0 && name[len - 1] == '\n') {
                    name[len - 1] = '\0';
                }

                wejscie = fopen(name , "r");

                if (wejscie == NULL) {
                    perror("bład odczytu pliku\n");
                    exit(0);
                }

            }else if(wybor == 1)
            {
                klawiatura = 1;
            }else
            {
                printf("Podano nie prawidlowa wartosc\n");
                exit(0);
            }

            //Obsluga wejscia z klawiatury
            if(klawiatura)
            {
                while (1)
                {
                    while(!running){;}

                    fgets(buffer, sizeof(buffer), stdin);
                    if(strcmp(buffer,".\n") == 0) break;
                    write(fd, &buffer, sizeof(buffer));

                    while(!running){;}
                }
                

            }else{

                //Obsluga wejscia z pliku
                while(fgets(buffer, sizeof(buffer), wejscie))
                {   
                    while(!running){;}
                    write(fd, &buffer, sizeof(buffer));
                    while(!running){;}
                
                }
            }

        }
        
        printf("Wykryto sygnal SIGTERM, zakanczam program\n");
        //Wyslanie komunikatu o zakonczeniu czytania z pliku
        strcpy(buffer,"Koniec123");
        write(fd, &buffer, sizeof(buffer));
        close(fd);

    }

    // Kod dla 2 procesu
    if(id == 2)
    {
        signal(SIGCONT, &Nothing);
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
            if(strcmp(buffer,"Koniec123") == 0) break;
            size = strlen(buffer);
            sem_wait(semP2_P3);
            *buffer2 = size - 1;
            sem_post(semP3_P2);  
        }
        close(fd);
    }

    //Kod dla 3 procesu
    if(id == 3)
    {
        int suma = 0;

        signal(SIGTSTP, &handle_sigstpP3);
        signal(SIGCONT, &handle_sigstpP3);
        signal(SIGUSR1, &handle_sigP3);
        signal(SIGTERM, &handle_end);

        while(1)
        {
            while(!running){;}
            sem_wait(semP3_P2);
            printf("Odebralem od P2: %d\n", *buffer2);
            suma += *buffer2;
            printf("Odczytanych znakow lacznie: %d\n",suma);
            sem_post(semP2_P3);

        }
    }


    // Kod procesu macierzystego
    if(id == 0)
    {
        signal(SIGCONT, &Nothing);
        signal(SIGTSTP, &Nothing);
        signal(SIGQUIT, &Nothing);
        signal(SIGUSR1, &handle_sigPM);
        signal(SIGUSR2, &handle_sigPM);
        signal(SIGTERM, &handle_sigPM);
        while(1)
        {
            if(przerwac) break;
        }
        
        wait(NULL);
    }
    
    
    //wait(&P2_PID);
    // Zwalnianie uzytych zasobow

    // Usunięcie semaforów
    sem_close(semP2_P3);
    sem_close(semP3_P2);
    // Odłączenie pamięci współdzielonej
    shmdt(semP2_P3);
    // Usunięcie pamięci współdzielonej
    shmctl(shm_id, IPC_RMID, NULL);

    // Usunięcie pliku FIFO
    unlink(fName);

    printf("Program zakonczyl dzialanie! \n");
    
    return 0;
}
