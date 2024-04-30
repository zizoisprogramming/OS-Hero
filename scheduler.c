#include "headers.h"
#include "memory.h"
struct processData
{
    int id;
    int arrival;
    int runtime;
    int priority;
    int memsize;
};
struct Node
{
    struct processData data;
    struct Node *next;
    struct nodePCB * mirror; // points to its mirror in the process table
};
struct WTAnode
{
    double data;
    struct WTAnode * next;
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
    int startingIndex;
    int EndingIndex;
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

union Semun
{
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};

struct Node* head = NULL;
struct Node* tail = NULL;
struct nodePCB* headPcb = NULL;
struct nodePCB* tailPcb = NULL;
struct block* headMem = NULL;
struct Node* headWaitList = NULL;
struct Node* tailWaitList = NULL;
struct Node * run = NULL; // currently running
int prevTime = 0; // to hold time to calculate quantum
int quantum; // to hold the quantum
int runCache[6]; // realId / running / remaining / waiting / state / lastRun

int algo; // 1 -> HPF  2 -> SRTN  3 -> RR
int runningQuantum;
bool dead = false;

int remshmId;
int remSemId;
int * remAddr;

FILE * pFile;
FILE * memoryFile;
double WTA=0;
int WT=0;
int Pcount=0;
int used=0;
int totalRun=0;
int endTime=0;
struct WTAnode* WTAlist=NULL;

void InsertWaitList(struct Node* ptr);
void checkWait();
void initializeSem();
void initializeMem();
void detachMem();
bool callAlgo(int);
int recieveMSG(int, int);
bool RR(int);
void savePCB(struct nodePCB*);
void preempt();
struct Node * insertProcess(struct msgbuff*);
struct nodePCB * insertPcb(struct PCB *);
struct nodePCB * makeProcess(struct msgbuff*, int ,int,int,int);
void childDead();
void intHandler(int sigNum);
void loadCache(struct nodePCB* );
void perfWrite();
bool SRTN(int);

int main(int argc, char * argv[])
{
    initClk();
    signal(SIGINT, intHandler);
    initializeMem();
    initializeSem();
    pFile = fopen("scheduler.log", "w");
    memoryFile=fopen("memory.log","w");
    fprintf(pFile, "#At time x process y state arr w total z remain y wait k\n");
    fprintf(memoryFile,"#At time x allocated y bytes for process z from i to j\n");
    //TODO implement the scheduler :)
    //upon termination release the clock resources. "ZIZO, DON'T FORGET"
    //print arguments
    headMem = initializeBlocks(1024);
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
            RecievedID = recieveMSG(ProcessQ, time);             // Start Scheduling for this timestamp   
            callAlgo(time); // specific algo handles from here

            if(run)
            {
                used++;
            }
        }
    }
    printf("at Time %d Recieved LAST %d\n", time, RecievedID);
    bool finish = false; // the loop variable
    while(!finish) { // while algo still running
        int time2 = getClk();
        if (time2 > time)
        {
            // Choose Algo
            time = time2;
            finish = callAlgo(time); // specific algo handles from here
            if(run)
            {
                used++;
            }
        }
    }
    printf("at Time %d Finished\n", time);

    //endTime=time;
    perfWrite();

    fclose(pFile);
    fclose(memoryFile);
   
    shmctl(remshmId, IPC_RMID, NULL);
    printf("Terminating the shared memory!\n");
    semctl(remSemId,0, IPC_RMID);
    printf("Terminating the semaphore!\n");
    //destroyClk(true);
}

double rooting(double num)
{
    double x= num;
    double epsilon=0.000001;

    while((x-num/x)>epsilon)
    {
        x=(x+num/x)/2;
    }

    return x;
}

void perfWrite()
{
    FILE * ptr;
    ptr = fopen("scheduler.perf", "w");
    double avgWTA = WTA/Pcount;
    fprintf(ptr,"CPU utilization = %.2f%%\n",((double)totalRun/(double)(endTime-1))*100);
    fprintf(ptr,"Avg WTA = %.2f \n", avgWTA);
    fprintf(ptr,"Avg Waiting = %.2f \n",(double)WT/Pcount);
    struct WTAnode* temp=WTAlist;
    double sum=0.0;
    while(temp!=NULL)
    {
        sum+=((temp->data-avgWTA)*(temp->data-avgWTA));
        temp=temp->next;
    }
    double Std=sqrt(sum);
    fprintf(ptr,"Std WTA = %.2f \n",Std);
    fclose(ptr);
}

// The RR scheduling function
bool RR(int now) {
    if(run) { 
        // if something is running
        kill(runCache[0],SIGUSR1); // real ID 
        down(remSemId); // wait till the process decrements

        runCache[1]++; // running time
        runCache[2] = *remAddr; // remaining time
        if(runCache[2] == 0) {
            int status;
            if(waitpid(runCache[0],&status,0) == -1) // wait for the exit code
                printf("Error in wait.\n"); 
            else 
                childDead(now); // execute the dying algo
        } 
        runningQuantum--;    // a quantum is finished    
        if(runningQuantum == 0 && !dead) {
           
            //runCache[3] += now - run->mirror->data.lastRun; // update the waiting time
            runCache[4] = 0; // state = 0 aka. skeeping
            runCache[5] = now; // last run for waiting calcultions
            printf("Quantum finished for process %d.\n", runCache[0]);             
            savePCB(run->mirror);
            preempt(); // update the list
            fprintf(pFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            kill(runCache[0], SIGSTOP);
            run = NULL; // no current running
        }
        if(dead) {
            printf("Process %d terminating.\n", runCache[0]);
            dead = false;
        }
    }
    if(!run) {
        runCache[2] = 0;
        while (head && !runCache[2]) {
            runningQuantum = quantum; // reset the quantum
            run = head; // a new head
            head = head->next; // update
            if(head == NULL)
                tail = NULL;
            struct nodePCB * runPCB = run->mirror; // get the PCB
            
            // load the data into the running cache
            loadCache(runPCB);
            kill(runCache[0], SIGCONT);
            if(runCache[2] == 0) {
                kill(runCache[0], SIGUSR1);
                down(remSemId); // wait till the process decrements
                int status;
                if(waitpid(runCache[0],&status,0) == -1) // wait for the exit code
                    printf("Error in wait.\n"); 
                else {
                    savePCB(run->mirror);
                    run->mirror->data.waiting += now-run->data.arrival; // waiting time
                    fprintf(pFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
                    childDead(now); // execute the dying algo
                }
            }
            
            if(runCache[5] == 0 && run) // just for printing
            {
                run->mirror->data.waiting+=now-run->data.arrival;
                printf("Inserted process %d with remaining %d.\n", run->data.id, runCache[2]);
                fprintf(pFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            }
            else if(run)
            {
                run->mirror->data.waiting+=(now-run->mirror->data.lastRun);
                printf("Rescheduling process %d with remaining %d.\n", run->data.id, runCache[2]);
                fprintf(pFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            }
            if(run)
                runCache[3] = run->mirror->data.waiting;
        }
   
    }
    if(!head && !run) // if the list is empty
        return true; // finish
    return false;
}

struct Node * insertProcess(struct msgbuff* message) {
    // allocate a new node
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    // set the data
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

void extractHPF()
{
    struct Node* temp=head;
    struct Node* HP=head;
    struct Node* prev=NULL;
    while(temp->next!=NULL)
    {
        if(temp->next->data.priority < HP->data.priority)
        {
            HP=temp->next;
            prev=temp;
        }
        temp=temp->next;
    }
    run=HP;
    if(HP==head)
    {
        head=head->next;
        if(head==NULL)
            tail=NULL;
        return;
    }
    if(HP==tail)
    {
        tail=prev;
        tail->next=NULL;
        return;
    }
    prev->next=HP->next;
}

bool HPF(int time)
{
    if(run) { 
        // if something is running
        kill(runCache[0],SIGUSR1); // real ID 
        down(remSemId); // wait till the process decrements

        runCache[1]++; // running time
        runCache[2] = *remAddr; // remaining time
        if(runCache[2] == 0) {
            int status;
            if(waitpid(runCache[0],&status,0) == -1) // wait for the exit code
                printf("Error in wait.\n"); 
            else 
                childDead(time); // execute the dying algo
        } 
        if(dead) {
            printf("Process %d terminating.\n", runCache[0]);
            dead = false;
        }
    }
    if(!run) {
        if(head) 
        {
            extractHPF();
            run->mirror->data.waiting=time-run->data.arrival;
            fprintf(pFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",time,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);

            struct nodePCB * runPCB = run->mirror; // get the PCB
            // load the data into the running cache
            loadCache(runPCB);
            kill(runCache[0], SIGCONT); // resume
            if(runCache[5] == 0) // just for printing
                printf("Inserted process %d with remaining %d.\n", run->data.id, runCache[2]);
            else
                printf("el mfrod mfesh print ops");
        }
    }
    if(!head && !run) // if the list is empty
        return true; // finish
    return false;
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
        }
        RecievedID = message.data.id;
        printf("at Time %d Recieved ID: %d mem: %d\n", time, RecievedID, message.data.memsize);

        if (RecievedID != -1 && RecievedID != -2)
        {
            //////////////Check if Possible////////////
            int required = message.data.memsize;
            printf("at Time %d ID: %d Requiring mem: %d\n", time, RecievedID, message.data.memsize);

            struct block* mem = allocate_process(headMem, required);
            if(mem == NULL)
            {
                struct Node * ptr = (struct Node*)malloc (sizeof(struct Node));
                ptr->data = message.data;
                ptr->next = NULL;
                printf("Memory is full.\n");

                InsertWaitList(ptr);
            }
            else
            {
                int IndexStart = mem->start_index;
                int IndexEnd = IndexStart + mem->block_size - 1;
                Pcount++;
                int newProcessID = fork();
                
                if(newProcessID == -1) {
                    perror("Error in fork.\n");
                }
                else if(newProcessID == 0) {
                    printf("child\n"); // child code
                    char runtimechar[10];
                    sprintf(runtimechar,"%d",message.data.runtime); // just a function that converts int to char
                    char * args[] = {"./process", runtimechar,NULL}; // prepare the arguments
                    execvp(args[0], args); // run 
                    printf("Execute error.\n"); // if reached here then error
                }
                else {
                    // insert in Process Table
                    printf("parent\n"); // parent code
                    down(remSemId); // wait till creation
                    kill(newProcessID, SIGSTOP);
                    struct Node * ptr = insertProcess(&message); // insert in ready queue
                    fprintf(memoryFile,"At time %d allocated %d bytes for process %d from %d to %d\n",time,mem->block_size,RecievedID,IndexStart,IndexEnd);
                    ptr->mirror = makeProcess(&message, RecievedID, newProcessID, IndexStart, IndexEnd); // insert PCB
                }
            }
            /////////////////Insertion Waiting List////////////////////////


            ////////////////////////////////////////////////////////////////
            
        }
    } 
    printf("returning \n");
    return RecievedID;
}
void InsertWaitList(struct Node* ptr)
{
    if(headWaitList==NULL)
    {
        headWaitList=ptr;
        tailWaitList=ptr;
        return;
    }
    tailWaitList->next=ptr;
    tailWaitList=ptr;
    ptr->next=NULL;
    return;
}
void checkWait()
{
    if(headWaitList==NULL)
    {
        return;
    }
    struct Node* temp=headWaitList;
    int required = temp->data.memsize;
    struct block* mem = allocate_process(headMem, required);

    if(mem == NULL)
    {
        printf("Memory is full.\n");
        return;
    }
    else
    {
        headWaitList=headWaitList->next;
        if(headWaitList==NULL)
        {
            tailWaitList=NULL;
        }
        struct msgbuff message;
        message.data=temp->data;
        int RecievedID = message.data.id;
        int IndexStart = mem->start_index;
        int IndexEnd = IndexStart + mem->block_size - 1;
        int newProcessID = fork();
        
        if(newProcessID == -1) {
            perror("Error in fork.\n");
        }
        else if(newProcessID == 0) {
            printf("child\n"); // child code
            char runtimechar[10];
            sprintf(runtimechar,"%d",message.data.runtime); // just a function that converts int to char
            char * args[] = {"./process", runtimechar,NULL}; // prepare the arguments
            execvp(args[0], args); // run 
            printf("Execute error.\n"); // if reached here then error
        }
        else {
            // insert in Process Table
            printf("parent\n"); // parent code
            down(remSemId); // wait till creation
            kill(newProcessID, SIGSTOP);
            struct Node * ptr = insertProcess(&message); // insert in ready queue
            ptr->mirror = makeProcess(&message, RecievedID, newProcessID, IndexStart, IndexEnd); // insert PCB
        }
    }
    checkWait();
}


bool callAlgo(int time){
    if(algo == 1) {
        // call HPF
        return HPF(time);
    }
    else if(algo == 2) {
        // call SPF
        return SRTN(time);
    }
    else {
        // call RR
        return RR(time);
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
        tailPcb = tailPcb->next;
    }
    return newProcess;
}

void savePCB(struct nodePCB* ptr) {
    ptr->data.running = runCache[1];
    ptr->data.remaining = runCache[2];
    ptr->data.waiting = runCache[3];
    ptr->data.state = runCache[4];
    ptr->data.lastRun = runCache[5];
    printf("Saving PCB.\n");
}

struct nodePCB * makeProcess(struct msgbuff* message,int id, int newProcessID, int start, int end) {
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
    processBlock.realId = newProcessID; // el id bta3 el system
    processBlock.startingIndex = start;
    processBlock.EndingIndex = end;
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

void childDead(int time) {
    struct WTAnode* temp=(struct WTAnode*)malloc(sizeof(struct WTAnode));
    if(temp!=NULL)
    {
        temp->data=(double)(time-run->data.arrival)/(double)run->data.runtime;
        temp->next=NULL;
    }
    if(WTAlist==NULL)
    {
        WTAlist=temp;
    }
    else
    {
        temp->next=WTAlist;
        WTAlist=temp;
    }
    WTA=WTA + (double)(time-run->data.arrival)/(double)run->data.runtime;
    WT=WT + run->mirror->data.waiting;
    totalRun+=run->data.runtime;
    fprintf(pFile, "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f \n",time,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.waiting,time-run->data.arrival,(double)(time-run->data.arrival)/(double)run->data.runtime);
    endTime=time;
    fprintf(memoryFile,"At time %d freed %d bytes for process %d from %d to %d\n",time,run->data.memsize,run->data.id,run->mirror->data.startingIndex,run->mirror->data.EndingIndex);
    deallocate_process(headMem,run->mirror->data.startingIndex);
    checkWait();
    removeBlock();
    free(run->mirror);
    dead = true;
    free(run);
    run = NULL;
    printf("terminated...\n");
    // output file stuff
}

void initializeSem() {
    union Semun semun;

    remSemId = semget(SEMKEY, 1, 0666 | IPC_CREAT);

    if (remSemId == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(remSemId, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
}

void initializeMem() {
    remshmId = shmget(MEM_KEY, sizeof(int), IPC_CREAT | 0666);

    if (remshmId == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    else
        printf("\nShared memory ID = %d\n", remshmId);
            
    remAddr = shmat(remshmId, NULL, 0);
    if (remAddr == (void *)-1)
    {
        perror("Error in attach in writer");
        exit(-1);
    }
    *remAddr = -1;
}

void detachMem() {
    shmdt(remAddr);
}

void intHandler(int sigNum) {
    shmctl(remshmId, IPC_RMID, NULL);
    printf("Terminating the shared memory!\n");
    semctl(remSemId,0,IPC_RMID);
    printf("Terminating the semaphore!\n");
    exit(0);
}

void preempt() {
    if(tail) {
        tail->next = run;
        run->next = NULL;
        tail = tail->next;
    }
    else {
        head = run;
        tail = run;
        head->next = NULL;
    }
}

void loadCache(struct nodePCB* runPCB) {
    runCache[0] = runPCB->data.realId;
    runCache[1] = runPCB->data.running;
    runCache[2] = runPCB->data.remaining;
    runCache[3] = runPCB->data.waiting;
    runPCB->data.state = 1;
    runCache[4] = runPCB->data.state;
    runCache[5] = runPCB->data.lastRun;
}


struct Node* extractMin() {
    if(!(head))
    return NULL;

    struct Node* iter=head;
    struct Node* MinNode=head;
    struct Node* prev=NULL;

    printf("starting extract min..\n");
    printf("tail is %d",tail->data.id);

    //loop to find min node
    while(iter->next!=NULL)
    {
    if(iter->next->mirror->data.remaining < MinNode->mirror->data.remaining)
        {
            MinNode=iter->next;
            prev=iter;
        }
    printf("here2 %d\n",iter->data.id);
    iter=iter->next;
    }
    //extract it
    if(MinNode==head)
    {
        head=head->next;
        if(head==NULL)
        {
            tail=NULL;
        }
    printf("here3\n");

        return MinNode;
    }
    if(MinNode==tail)
    {
        tail=prev;
        tail->next=NULL;
    printf("here4\n");

        return MinNode;
    }
    //general case (no head and tail modifications)
    prev->next=MinNode->next;
    printf("here5\n");

    return MinNode;
}
bool SRTN(int now)
{
    printf("SRTN called \n");
    struct Node* min_node = NULL;

        // actual run
        if(run)
        {
            if(run->mirror->data.remaining > 0)
            {
                kill(run->mirror->data.realId,SIGUSR1); // sig usr1 decrements remaining time of process
                down(remSemId); // wait till the process decrements
                run->mirror->data.running++; // running time
                run->mirror->data.remaining = *remAddr; //read remaining time from shared memory
                run->mirror->data.lastRun = now;
                /////for debugging
                printf("at time %d process %d with rem time %d has run \n",
                now,run->data.id,run->mirror->data.remaining);
            }
            int debug_id = run->mirror->data.realId;
            if(run->mirror->data.remaining == 0) { //process finished
                int status;
                if(waitpid(run->mirror->data.realId,&status,0) == -1) // wait for the exit code
                    printf("Error in wait.\n"); 
                else 
                    //this sets run to null
                    childDead(now); // execute the dying algo
            }
            
            
            if(dead) {
                printf("Process %d terminating.\n", debug_id);
                dead = false;
            }
        }
        min_node = extractMin(); //extract node with minimum remaining time
        if(run)
        {
            printf("there is a running process\n");
            //if min node is better than already running process, switch
            if(min_node!=NULL && min_node->mirror->data.remaining < run->mirror->data.remaining && min_node!=run)
            { 
                // preempt
                printf("new run is loaded,switching \n");
                
                // old run
                run->mirror->data.state = 0; // state = 0 aka. skeeping
                // return run to ready q
                run->mirror->data.lastRun=now;
                fprintf(pFile, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);

                if(tail!=NULL)
                {
                    tail->next = run;
                    tail = run;
                    tail->next = NULL;
                }
                else
                {
                    head = run;
                    tail = run;
                }
                kill(run->mirror->data.realId, SIGSTOP);

                //load new run
                run = min_node;
                if(run->data.runtime==run->mirror->data.remaining)
                {
                    run->mirror->data.waiting+=(now-run->mirror->data.arrival);
                    fprintf(pFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
                }
            else
            {
                run->mirror->data.waiting+=(now-run->mirror->data.lastRun);
                fprintf(pFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            }
                kill(min_node->mirror->data.realId, SIGCONT);
            }
            else
            {
                printf("old run will not be replaced\n");
                if(min_node!=NULL) //return min_node to ready q
                {
                    if(tail!=NULL)
                    {
                        tail->next=min_node;
                        tail=min_node;
                        tail->next=NULL;
                    }
                    else
                    {
                        head= min_node;
                        tail=min_node;
                    }
                }
            }
        }
        else if(!run && min_node)
        {
            printf("new run is loaded,no switching");
            //load new run
            run=min_node;
            if(run->data.runtime==run->mirror->data.remaining)
            {
                run->mirror->data.waiting+=(now-run->mirror->data.arrival);
                fprintf(pFile, "At time %d process %d started arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            }
            else
            {
                run->mirror->data.waiting += (now-run->mirror->data.lastRun);
                fprintf(pFile, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",now,run->data.id,run->data.arrival,run->data.runtime,run->mirror->data.remaining,run->mirror->data.waiting);
            }
            kill(min_node->mirror->data.realId, SIGCONT);
        }
        if(!head&&!run)
        return true;

    return false;
    
}