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
    struct nodePCB * mirror; // points to its mirror in the process table
};
struct PCB
{
    int id;
    int realId;
    int arrival; // arrival time
    int runtime; // time require to run
    int running; // time in execution
    int remaining; // time left
    int waiting; // time blocked or sleeped
    int priority;
    int state; // 0 -> sleep 1 -> running
    int lastRun;
};
struct nodePCB 
{
    struct PCB data;
    struct nodePCB *next;
    struct nodePCB *previous;
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

struct Node * run = NULL; // currently running
int prevTime = 0; // to hold time to calculate quantum
int quantum; // to hold the quantum
int runCache[6]; // realId / running / remaining / waiting / state / lastRun

int algo; // 1 -> HPF  2 -> SRTN  3 -> RR
int runningQuantum;
bool dead = false;

void callAlgo(int);
int recieveMSG(int, int);
int RR(int);
void savePCB(struct nodePCB*,int *);
struct Node * insertProcess(struct msgbuff*);
struct nodePCB * insertPcb(struct PCB *);
struct nodePCB * makeProcess(struct msgbuff*, int ,int);
void childDead(int signum);
int main(int argc, char * argv[])
{
    initClk();
    signal(SIGCHLD, childDead);
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
    if(algo == 3) {
        quantum = atoi(argv[2]);
        runningQuantum = quantum;
    }
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
            callAlgo(time); // specific algo handles from here
        }
    }
    while(1) {
        int time2 = getClk();
        if (time2 > time)
        {
            // Choose Algo
            time = time2;
            callAlgo(time); // specific algo handles from here
        }
    }
    printf("at Time %d Recieved LAST %d\n", time, RecievedID);
    exit(0);

    //destroyClk(true);
}
// The RR scheduling function
int RR(int now) {
    // Initialize the process
    // needs functions
    // -> Initialize() to start the process and set the PCB
    // -> saveState() to save in PCB
    // make process control block
    // deleteData() to clean up the process
    // report-atk
    if(run) { 
        // if something is running
        kill(runCache[0],SIGUSR1); // real ID
        runCache[1]++; // running time
        runCache[2]--; // remaining time
        
        runningQuantum--;
        printf("running");
        if(runningQuantum == 0 && !dead) {
            runCache[3] += now - run->mirror->data.lastRun;
            runCache[4] = 0; // state = 0 aka. skeeping
            runCache[5] = now; // last run for waiting calcultions
            savePCB(run->mirror, runCache); // save the state
            if(tail) {
                tail->next = run;
                run->next = NULL;
                tail = tail->next;
            }
            else {
                head = run;
                tail = run;
            }
            printf("Quantum finished for process %d.\n", runCache[0]);
            run = NULL; // no current running
        }
        if(dead) {
            printf("Process %d terminating.\n", runCache[0]);
            dead = false;
        }
    }
    if(!run) {
        if(head) {
            runningQuantum = quantum; // reset the quantum
            run = head; // a new head
            head = head->next;
            struct nodePCB * runPCB = run->mirror; // get the PCB
            // load the data into the running cache
            runCache[0] = runPCB->data.realId;
            runCache[1] = runPCB->data.running;
            runCache[2] = runPCB->data.remaining;
            runCache[3] = runPCB->data.waiting;
            runPCB->data.state = 1;
            runCache[4] = runPCB->data.state;
            runCache[5] = runPCB->data.lastRun;
            if(runCache[5] == 0)
                printf("Inserted process %d with remaining %d.\n", run->data.id, runCache[2]);
            else
                printf("Rescheduling process %d with remaining %d.\n", run->data.id, runCache[2]);
        }
    }
    return 0;
}

struct Node * insertProcess(struct msgbuff* message) {
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
    return newNode;
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
            struct Node * ptr = insertProcess(&message);
            int newProcessID = fork();
            
            if(newProcessID == -1) {
                perror("Error in fork.\n");
            }
            else if(newProcessID == 0) {
                printf("child\n");
                char runtimechar[10];
                sprintf(runtimechar,"%d",message.data.runtime);
                char * args[] = {"./process", runtimechar,NULL};
                execvp(args[0], args);
                printf("Execute error.\n");
            }
            else {
                // insert in Process Table
                printf("parent");
                ptr->mirror = makeProcess(&message, RecievedID, newProcessID);
            }
        }
    
    } 
    return RecievedID;
}

void callAlgo(int time){
    if(algo == 1) {
        // call SPF
    }
    else if(algo == 2) {
        // call HPF
    }
    else {
        // call RR
        RR(time);
    }
}

struct nodePCB * insertPcb(struct PCB * processBlock)
{
    struct nodePCB* newProcess = (struct nodePCB*)malloc(sizeof(struct nodePCB));
    newProcess->data = *processBlock;
    newProcess->next = NULL;
    newProcess->previous = NULL;
    if (headPcb == NULL)
    {
        headPcb = newProcess;
        tailPcb = newProcess;
    }
    else
    {
        tailPcb->next = newProcess;
        newProcess->previous = tailPcb;
        tailPcb = newProcess;
    }
    return newProcess;
}

void savePCB(struct nodePCB* ptr, int * cache) {
    ptr->data.running = cache[1];
    ptr->data.remaining = cache[2];
    ptr->data.waiting = cache[3];
    ptr->data.state = cache[4];
    ptr->data.lastRun = cache[5];
}

struct nodePCB * makeProcess(struct msgbuff* message,int id, int newProcessID) {
    // insert in Process Table
    struct PCB processBlock;
    processBlock.id = id; // save the id
    processBlock.arrival = message->data.arrival;
    processBlock.priority = message->data.priority; 
    processBlock.remaining = message->data.runtime; // still didn't run
    processBlock.running = 0; // not a single second yet
    processBlock.runtime = message->data.runtime; // run time m4 m7taga
    processBlock.state = 0; // sleep for now
    processBlock.waiting = 0; // set waiting time
    processBlock.lastRun = 0;
    processBlock.realId = newProcessID;
    return insertPcb(&processBlock);
}

void removeBlock() {
    struct nodePCB * prev = run->mirror->previous;
    if(prev) {
        prev->next = run->mirror->next;
    }
    else {
        headPcb = headPcb->next;
        if(headPcb) {
            headPcb->previous = NULL;
        }
    }
}

void childDead(int sigNum) {
    removeBlock();
    free(run->mirror);
    dead = true;
    free(run);
    run = NULL;
    // output file stuff
    signal(SIGCHLD, childDead);
}