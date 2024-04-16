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
struct PCB
{
    int id;
    int arrival; // arrival time
    int runtime; // time require to run
    int running; // time in execution
    int remaining; // time left
    int waiting; // time blocked or sleeped
    int priority;
    int state;
    // 0 -> sleep
    // 1 -> running
};
struct nodePCB 
{
    struct PCB data;
    struct nodePCB *next;
};
struct msgbuff
{
    long mtype;
    struct processData data;
};

struct Node* head = NULL;
struct Node* tail = NULL;
struct nodePCB* headPcb = NULL;
struct nodePCB* tailPcb = NULL;

int algo; // 1 -> HPF  2 -> SRTN  3 -> RR

void callAlgo();
int recieveMSG(int, int);
int RR();
void savePCB(struct nodePCB*,int,int,int,int);
void insertProcess(struct msgbuff*);
void insertPcb(struct PCB *);
void makeProcess(struct msgbuff*, int);
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
    // Initialize the process
    // needs functions
    // -> Initialize() to start the process and set the PCB
    // -> saveState() to save in PCB
    // make process control block
    // deleteData() to clean up the process
    // report-atk
    return 0;
}

void insertProcess(struct msgbuff* message) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->data = message->data;
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
            insertProcess(&message);
            int newProcessID = fork();
            
            if(newProcessID == -1) {
                perror("Error in fork.\n");
            }
            else if(newProcessID == 0) {
                printf("child\n");
                char runtimechar[10];
                sprintf(runtimechar,"%d",message.data.runtime);
                char * args[] = {"./process", runtimechar,NULL};
                execvp(args[0], args) == -1;
                printf("Execute error.\n");
            }
            else {
                // insert in Process Table
                printf("parent");
                makeProcess(&message, RecievedID);
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
        RR();
    }
}

void insertPcb(struct PCB * processBlock)
{
    struct nodePCB* newProcess = (struct nodePCB*)malloc(sizeof(struct nodePCB));
    newProcess->data = *processBlock;
    newProcess->next = NULL;
    if (headPcb == NULL)
    {
        headPcb = newProcess;
        tailPcb = newProcess;
    }
    else
    {
        tailPcb->next = newProcess;
        tailPcb = newProcess;
    }
}

void savePCB(struct nodePCB* ptr, int remaining, int state, int waiting, int running) {
    ptr->data.remaining = remaining;
    ptr->data.running = running;
    ptr->data.state = state;
    ptr->data.waiting = waiting;
}

void makeProcess(struct msgbuff* message, int newProcessID) {
    // insert in Process Table
    struct PCB processBlock;
    processBlock.id = newProcessID; // save the id
    processBlock.arrival = message->data.arrival;
    processBlock.priority = message->data.priority; 
    processBlock.remaining = message->data.runtime; // still didn't run
    processBlock.running = 0; // not a single second yet
    processBlock.runtime = message->data.runtime; // run time m4 m7taga
    processBlock.state = 0; // sleep for now
    processBlock.waiting = 0; // set waiting time
    insertPcb(&processBlock);
}
