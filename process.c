#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int argc, char * argv[])
{
    initClk();
    
    // TODO it needs to get the remaining time from somewhere
    remainingtime = argv[1]; // initialized when initializing process
    // current thoughts -> wait for schedular to send a signal and decrease runtime
    while (remainingtime > 0)
    {
        
    }
    
    destroyClk(false);
    
    return 0;
}
