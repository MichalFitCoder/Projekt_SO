#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdlib.h>

int id, fd;
#define fName "P1_P2_fifo"

int main() {
    char buffer[100];
    mkfifo(fName, 0666);

    int shmid;
    sem_t *mutex;

    // Utwórz segment pamięci współdzielonej
    shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    // Przypisz pamięć współdzieloną do semafora
    mutex = (sem_t*)shmat(shmid, 0, 0);
    if (mutex == (sem_t*)-1) {
        perror("shmat");
        return 1;
    }

    // Zainicjuj semafor
    sem_init(mutex, 1, 1);

    if (fork()) {
        id = 0;
    } else {
        if (fork()) {
            id = 1;
        } else {
            if (fork()) {
                id = 2;
            } else {
                id = 3;
            }
        }
    }

    // Kod dla 1 procesu
    if (id == 1) {
        printf("(1) Jestem procesem:  %d\n", getpid());
        fd = open(fName, O_WRONLY);
        if (fd == -1) {
            perror("Blad przy otwieraniu potoku do zapisu");
            return 1;
        }

        sem_wait(mutex); // Wejście do sekcji krytycznej

        printf("Podaj cos: \n");
        scanf("%s", buffer);
        printf("zapisano: %ld\n", write(fd, buffer, sizeof(buffer)));

        sem_post(mutex); // Wyjście z sekcji krytycznej

        close(fd);
    }

    // Kod dla 2 procesu
    if (id == 2) {
        printf("(2) Jestem procesem:  %d\n", getpid());

        fd = open(fName, O_RDONLY);
        if (fd == -1) {
            perror("Blad przy otwieraniu potoku do odczytu");
            return 1;
        }

        sem_wait(mutex); // Wejście do sekcji krytycznej

        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        printf("Odczytano: %ld\n", bytesRead);

        sem_post(mutex); // Wyjście z sekcji krytycznej

        if (bytesRead > 0) {
            printf("Otrzymano: %s\n", buffer);
        } else {
            perror("Blad przy odczycie z potoku");
        }

        close(fd);
    }

    // Kod dla 3 procesu
    if (id == 3) {
        printf("(3) Jestem procesem:  %d\n", getpid());
    }

    // Kod procesu macierzystego
    if (id == 0) {
        printf("(Macierzysty) Jestem procesem:  %d\n", getpid());
    }

    // Usuń semafor i pamięć współdzieloną
    sem_destroy(mutex);
    shmdt(mutex);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
