#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
void handler(int signum);
int main(int argc, char * argv[])
{
    initClk();
    signal(SIGUSR1, handler);
    // TODO it needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]); // initialized when initializing process
    printf("I am here process with remaining time: %d\n", remainingtime);
    // current thoughts -> wait for schedular to send a signal and decrease runtime
    while (remainingtime > 0)
    {
        // just waits for signals to update the remaining time
    }
    // last thing
    destroyClk(false);
    // It should notify the schedular 
    // current thoughts -> schedular binds SIGCHLD and if called it is terminated then
    return 0;
}
void handler(int signum) {

    remainingtime--;
    printf("Remaining time decremented %d", remainingtime);
    // re-bind
    signal(SIGUSR1, handler);    
}