#include "headers.h"

void clearResources(int);
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
};
struct msgbuff
{
    long mtype;
    struct processData data;
};
int ProcessQ;
void readFile(struct Node** head, struct Node** tail, char * path) {
    FILE *fptr;
    
    fptr = fopen(path, "r");
    if (fptr == NULL) {
        perror("Error in opening file");
        exit(1);
    }
    
    
    int id, arrival, runtime, priority;
    char line[100]; // Assuming each line has at most 100 characters
    while (fgets(line, sizeof(line), fptr) != NULL) {
        if (line[0] == '#') {
            // Ignore comment lines
            continue;
        }
        
        struct processData process;
        if (sscanf(line, "%d %d %d %d %d", &process.id, &process.arrival, &process.runtime, &process.priority, &process.memsize) != 5) {
            fprintf(stderr, "Error parsing line: %s\n", line);
            continue;
        }
        
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->data = process;
        newNode->next = NULL;
        
        if (*head == NULL) {
            *head = newNode;
            *tail = newNode;
        } else {
            (*tail)->next = newNode;
            *tail = newNode;
        }
    }
    
    fclose(fptr);
}
void printProcesses(struct Node* head)
{
    //print all processes
    struct Node* current = head;
    while (current != NULL)
    {
        printf("id: %d, arrival: %d, runtime: %d, priority: %d, memsize: %d\n", current->data.id, current->data.arrival, current->data.runtime, current->data.priority, current->data.memsize);
        current = current->next;
    }
}
int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    //read file processes.txt
    struct Node* head = NULL;
    struct Node* tail = NULL;
    readFile(&head, &tail, argv[3]);
    printProcesses(head);

    key_t key_id;


    key_id = 17;
    ProcessQ = msgget(key_id, 0666 | IPC_CREAT);
    printf("ProcessQ: %d\n", ProcessQ);
    // 3. Initiate and create the scheduler and clock processes.
    int pidScheduler = fork();
    int pidClk;
    if (pidScheduler == -1)
    {
        perror("error in fork");
    }
    else if (pidScheduler == 0)
    {
        
            char algorithm [10];
            sprintf(algorithm, "%d", atoi(argv[1]));
            char quantumStr[10];
            sprintf(quantumStr, "%d", atoi(argv[2]));
            char* args[] = {"/home/ziad/Project/Phase 2/OS-Hero/scheduler", algorithm, quantumStr, NULL};

            execvp(args[0], args);

    }
    else
    {
        pidClk = fork();
        if (pidClk == -1)
        {
            perror("error in fork");
        }
        else if (pidClk == 0)
        {
            char * args[] = {"/home/ziad/Project/Phase 2/OS-Hero/clk", NULL};
            execvp(args[0], args);
        }
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    int x = 0;
    while(head != NULL)
    {
        if (getClk() > x)
        {
        
        struct msgbuff message = {1, head->data};
        //printf("at Time %d Head ID: %d\n", getClk(), head->data.id);
        x = getClk();
            while(head != NULL && head->data.arrival == x)
            {
                printf("at Time %d Sent ID: %d\n", x, head->data.id);
                struct processData process = head->data;
                struct msgbuff message;
                message.mtype = 1;
                message.data = process;
                int send_val = -1;
                while (send_val == -1)
                {
               
                    send_val = msgsnd(ProcessQ, &message, sizeof(message.data), !IPC_NOWAIT);
                }
                //int send_val = msgsnd(ProcessQ, &message, sizeof(message.data), !IPC_NOWAIT);
                struct Node* temp = head;
                head = head->next;
                free(temp);
            }
            message.data.id = -1;
            //printf("at Time %d Sent ID: %d\n", getClk(), -1);      
            int send_val = -1;      
            while (send_val == -1)
            {
                send_val = msgsnd(ProcessQ, &message, sizeof(message.data), !IPC_NOWAIT);
                if(send_val != -1)
                {
                    printf("at Time %d Sent ID: %d mem: %d\n", x, message.data.id, message.data.memsize);
                }
            }
        }
        
    }
    struct msgbuff message = {1, {-2, -1, -1, -1, -1}};
    int send_val = -1;
    printf("Sent Last %d\n", getClk());      
    while (send_val == -1)
    {
             
        send_val = msgsnd(ProcessQ, &message, sizeof(message.data), !IPC_NOWAIT);
    }
    printf("All processes sent\n");


    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    waitpid(pidScheduler, NULL, 0);
    msgctl(ProcessQ, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
    msgctl(ProcessQ, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
    exit(0);
}
