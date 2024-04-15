#include "headers.h"
struct processData
{
    int id;
    int arrival;
    int runtime;
    int priority;
};
struct Node
{
    struct processData data;
    struct Node *next;
};
struct msgbuff
{
    long mtype;
    struct processData data;
};

struct Node* head = NULL;
struct Node* tail = NULL;
int algo;

void callAlgo();
int recieveMSG();
int RR();
int main(int argc, char * argv[])
{
    initClk();
    
    //TODO implement the scheduler :)
    //upon termination release the clock resources. "ZIZO, DON'T FORGET"
    //print arguments
    for(int i=0; i<argc; i++)
    {
        printf("arg %d: %s\n", i, argv[i]);
    }
    //arg 1 = choosen algorithm (HPF, SRTN, RR) -> 1, 2, 3
    //arg 2 = quantum for RR (if RR is choosen)
    algo = atoi(argv[1]);

    key_t key_id = 17;
    int ProcessQ = msgget(key_id, 0666 | IPC_CREAT);
    
    int RecievedID = 0;
    int time = 0;
    while(RecievedID != -2)     // -2 is the last process           //will be changed
    {
        int time2 = getClk();
        if (time2 > time)
        {
            time = time2;
            RecievedID = 0;
            // Recieve the message in RecievedID
            RecievedID = recieveMSG(ProcessQ, time); 
            // Start Scheduling for this timestamp   
            // Choose Algo
            callAlgo(); // specific algo handles from here
        }
        
    }
    printf("at Time %d Recieved LAST %d\n", time, RecievedID);
    exit(0);


    //destroyClk(true);
}
// The RR scheduling function
int RR() {

}

int recieveMSG(int ProcessQ, int time)
{
    int RecievedID = 0;
    while (RecievedID != -1 && RecievedID != -2)    // -1 is the last process at this timestamp
    {
        struct msgbuff message;
        int rec_val = -1;
        while (rec_val == -1)
        {
            rec_val = msgrcv(ProcessQ, &message, sizeof(message.data), 0, !IPC_NOWAIT);
            if(rec_val == -1)
                printf("-1 here");
        }
        RecievedID = message.data.id;
        printf("at Time %d Recieved ID: %d\n", time, RecievedID);

        if (RecievedID != -1 && RecievedID != -2)
        {
            struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
            newNode->data = message.data;
            newNode->next = NULL;
            if (head == NULL)
            {
                head = newNode;
                tail = newNode;
            }
            else
            {
                tail->next = newNode;
                tail = newNode;
            }
        }
    
    } 
    return RecievedID;
}

void callAlgo(){
    if(algo == 1) {
        // call SPF
    }
    else if(algo == 2) {
        // call HPF
    }
    else {
        // call RR
    }
}
