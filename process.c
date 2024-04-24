#include "headers.h"


/* Modify this file as needed*/
int remainingtime;
int semId; // semaphore id
int memId; // memory id
int * remAddr;
bool started;

void handler(int signum);
void getSem();
void getMem();

int main(int argc, char * argv[])
{
    initClk();
    signal(SIGUSR1, handler);
    getMem();
    getSem();
    // TODO it needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]); // initialized when initializing process
    (*remAddr) = remainingtime;
    printf("I am here process with remaining time: %d\n", remainingtime);
    started = false;
    up(semId);
    // current thoughts -> wait for schedular to send a signal and decrease runtime
    while (remainingtime > 0 || !started)
    {
        // just waits for signals to update the remaining time
    }
    // last thing
    destroyClk(false);
    // It should notify the schedular 
    // current thoughts -> schedular binds SIGCHLD and if called it is terminated then
    printf("Dying.\n");
    return 0;
}
void handler(int signum) {
    started = true;
    remainingtime--;
    (*remAddr) = remainingtime;
    printf("Remaining time decremented %d\n", *remAddr);
    up(semId);
    // re-bind
    signal(SIGUSR1, handler);    
}

void getSem() {
    semId = semget(SEMKEY, 1, 0666);

}

void getMem() {
    memId = shmget(MEM_KEY,sizeof(int),0666);
    if(memId == -1){
        printf("Error in getting memory process.\n");
    }

    remAddr = shmat(memId, NULL, 0);
    if (remAddr == (void *)-1) {
        perror("shmat");
    }
}

void destroyMem() {
    shmdt(remAddr);
}