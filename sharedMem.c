#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/sem.h>

#define SIZE 16

int main (int argc, char *argv[]) {
	int status, shmId, semId;
	long int i, loop, temp, *shmPtr;
    	sem_t semvar;
	pid_t pid;
	key_t semKey;
	char *path = "/home/ramijavi/CIS452/lab6/sampleProgram1.c";

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
	if((semId = semget(semKey, 1, 0600|IPC_CREAT)) < 0){
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
			temp = shmPtr[0];
			shmPtr[0] = shmPtr[1];
			shmPtr[1] = temp;
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
			temp = shmPtr[1];
			shmPtr[1] = shmPtr[0];
			shmPtr[0] = temp;
        	}
    	}
	#Just for debugging
    	#wait (&status);
    	printf ("Values: %li\t%li\n", shmPtr[0], shmPtr[1]);

	sleep(16);
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
