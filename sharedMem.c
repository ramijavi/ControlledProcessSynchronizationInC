#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>

#define SIZE 16
#define PATH_SIZE 128

int main (int argc, char *argv[]) {
    int status, shmId, semId;
    long int i, loop, temp, *shmPtr;
    pid_t pid;
    key_t semKey;
    char path[PATH_SIZE];
    struct sembuf w = {0, -1, 0};
    struct sembuf s = {0, 1, 0};

    if(getcwd(path, PATH_SIZE) == NULL)
        printf("Failed to get directory\n");

    if(argc < 2) {
        printf("needs a number argument\n");
        exit(1);
    }

    loop = atoi(argv[1]);

    if ((shmId = shmget (IPC_PRIVATE, SIZE,
                    IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
        perror ("Couldn't allocate shared memory\n");
        exit (1);
    }
    if ((shmPtr = shmat (shmId, 0, 0)) == (void *) -1) {
        perror ("Couldn't attach to shared memory\n");
        exit (1);
    }

    if((semKey = ftok(path,1)) < 0){
        perror("Ftok failed\n");
    }
    printf("semKey: %d\n", semKey);
    if((semId = semget(semKey, 1, S_IRUSR | S_IWUSR | IPC_CREAT)) < 0){
        perror ("Couln't create semaphore\n");
        exit(1);
    }

    if(semctl(semId, 0, SETVAL, 1) < 0){
        perror("Couldn't initialize semaphore\n");
        exit(1);
    }

    shmPtr[0] = 0;
    shmPtr[1] = 1;

    if (!(pid = fork ())) {
        //CHILD PROCESS
        for (i = 0; i < loop; i++) {
            //wait
            if (semop(semId, &w, 1) == -1) {
                //printf("failed to wait in child: %s\n");
                perror("failed to wait in child");
                exit(2);
            }

            temp = shmPtr[0];
            shmPtr[0] = shmPtr[1];
            shmPtr[1] = temp;

            //signal
            if (semop(semId, &s, 1) == -1) {
                perror("failed to signal in child");
                exit(2);
            }

        }
        if (shmdt (shmPtr) < 0) {
            perror ("Child couldn't dettach from shared memory\n");
            exit (1);
        }

        exit (0);
    }
    else {
        //PARENT PROCESS
        for (i = 0; i < loop; i++) {
            if (semop(semId, &w, 1) == -1) {

                perror("failed to wait in child");
                exit(2);
            }

//            printf("Semval parent critical section: %d\n", semctl(semId, 0, GETVAL));
            temp = shmPtr[1];
            shmPtr[1] = shmPtr[0];
            shmPtr[0] = temp;

            if (semop(semId, &s, 1) == -1) {
                perror("failed to signal in child");
                exit(2);
            }
        }
    }
    wait(&status);
    printf ("Values: %li\t%li\n", shmPtr[0], shmPtr[1]);
    
    if (shmdt (shmPtr) < 0) {
        perror ("Couldn't dettach from shared memory\n");
        exit (1);
    }
    if (shmctl (shmId, IPC_RMID, 0) < 0) {
        perror ("Couldn't deallocate shared memory segment\n");
        exit (1);
    }
    if (semctl (semId, 0, IPC_RMID) < 0){
        perror("Couldn't remove semaphore\n");
    }

    return 0;
}
