#include "headers.h"

/*struct processBuffer
{
    int id;
    int arrivalTime;
    int runTime;
    int priority;
    enum processStatus status;
    Process p;
    long mtype;
    bool haveProcess;
}*/

void FCFS();
void SJF();
void HPF();
void SRTN();
void getArrivalProcessAndPushIt();
void clearResources(int signum);
void RR(int quantum);
void cleanup(int signum);
struct Queue *q = NULL;
struct priorityQueue *pq = NULL;
struct Process processRecv;
int chosenAlgorithm, paramter = -1, numOfProcesses = 3;
bool lastProcess = 0;
int msgQ;

int countProcess = 0;
int *shmId, shmid; //for the running process

FILE *schedulerLog;
int main(int argc, char *argv[])
{

    signal(SIGINT, cleanup);
    initClk();
    printf("hello i am scheduler...\n");
    FILE *f;
    f = fopen("key", "r");
    key_t key_id = ftok("key", 'a');
    msgQ = msgget(key_id, 0666 | IPC_CREAT);
    if (msgQ == -1)
    {
        perror("Error in creating message queue");
        return -1;
    }

    schedulerLog = fopen("schedular.log","w");
    fprintf(schedulerLog,"at time x process y started arrive time w running time z remning time y waiting time k\n");
    /*  struct processHeaders procHeaders;
    procHeaders.mtype = 1;
    int val = msgrcv(msgQ, &procHeaders, sizeof(procHeaders.algorithm) + sizeof(procHeaders.numOfProcesses) + sizeof(procHeaders.processParameter), 0, !IPC_NOWAIT);
    if (val == -1)
        perror("Error in Receiving");
    printf("ChosenAlgorithm: %ld\nnumOfProcesses: %ld\nprocessParam:%ld\n", procHeaders.algorithm, procHeaders.numOfProcesses, procHeaders.processParameter);*/

    /*chosenAlgorithm = atoi(argv[1]);
    numOfProcesses = atoi(argv[2]);
    if (chosenAlgorithm == 5)
        paramter = atoi(argv[3]);*/
    // to DO :!!!!!!!! replace it with argc
    chosenAlgorithm = 5;
    if (chosenAlgorithm == 5)
        paramter = 2;

    key_t sharedMemKey = ftok("Makefile", 65);
    shmid = shmget(sharedMemKey, 4000, 0666 | IPC_CREAT); // crete shared
    if (shmid == -1)
    {
        perror("Error in creating message queue");
        return -1;
    }
    shmId = (int *)shmat(shmid, (void *)0, 0);

    RR(paramter);
   // SJF();
    printf("Terminate msgQ from Scheduler\n");
    fclose(schedulerLog);
    msgctl(msgQ, IPC_RMID, (struct msqid_ds *)0);
    destroyClk(true);
}

int startProcess(Process p)
{

    // to do : fork the Process
    // to do print the log
    p.pid = fork();
    if (p.pid == 0)
    {
        //printf("hello form fork\n");
        //system("gcc process.c -o process.out");
        execl("./process.out", "process", NULL);
    }
    int waitingTime = getClk() - p.arrivalTime;
    fprintf(schedulerLog,"at time %d process %d started arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
    //printf("at time %d process %d started arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
    return p.pid;
}
void continueProcess(Process p)
{
    // to do : sigcont(p.pid)
    // to do print the log
    kill(p.pid, SIGCONT);
    int waitingTime = getClk() - p.arrivalTime;
    fprintf(schedulerLog,"at time %d process %d continued arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
    //printf("at time %d process %d continued arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
}
void stopProcess(Process p)
{
    // to do : sigstop(p.pid)
    // to do print the log
    kill(p.pid, SIGSTOP);
    int waitingTime = getClk() - p.arrivalTime;
    fprintf(schedulerLog,"at time %d process %d stoped arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
    //printf("at time %d process %d stop arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
}
void finishProcess(Process p)
{
    // to do : clac waiting , fininsh time..............
    // to do print the log
    int waitingTime = getClk() - p.arrivalTime;
    fprintf(schedulerLog,"at time %d process %d finished arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
    //printf("at time %d process %d finish arrive time %d running time %d remning time %d waiting time %d\n", getClk(), p.id, p.arrivalTime, p.runTime, p.remningTime, waitingTime);
}

void RR(int quantum)
{
    printf("Schuder: hello i started RR...\n");
    q = (Queue *)malloc(sizeof(Queue));
    queueConstructor(q);
    int cntQuantum = quantum; //conuter for track the quantum of the running Process
    struct Process running;
    *shmId = -1;
    bool isProcessRunNow=0;
    int timeWillEndHisQuantum=0;
    while (1)
    {
        

        if (isProcessRunNow && *shmId <= 0) //running process is finish
        {
            printf("Sche : finish process\n");
             running.remningTime = 0;
            finishProcess(running);
            isProcessRunNow = 0;
        }
        if (isProcessRunNow && timeWillEndHisQuantum <= getClk()) //no finish yet(stop it and push it pack to queue)
        {
            printf("Sche : stop process\n");
            running.remningTime -= quantum;
            if(running.remningTime<=0){
                finishProcess(running);
            }
            else{
                queuePush(q, running);
                stopProcess(running);
            }
            isProcessRunNow = 0;
        }
        if (!queueIsEmpty(q) && !isProcessRunNow ) //pick front of the q
        {
            printf("Sche : pick process\n");
            running = queuePop(q);
            *shmId = running.remningTime;
            timeWillEndHisQuantum = min(getClk() + quantum, getClk() + running.remningTime);
            isProcessRunNow = 1;
            if (running.remningTime < running.runTime) // it's contiue
            {
                printf("Sche : continue process\n");
                continueProcess(running);
            }
            else //first time to run
            {
                printf("Sche : start process will stop at %d\n",timeWillEndHisQuantum);
                running.pid = startProcess(running);
            }
        }
        if(isProcessRunNow)
        {
            printf("\nSche : running  id %d remning time %d curr time %d qunStop time %d\n", running.id,(*shmId),getClk(),timeWillEndHisQuantum);
            printQueue(q);
        }

        // struct Process p;
        /* revivce process*/
        processRecv.valid = 0; //clear prev recv mess
        processRecv.mtype = 1;
        if (countProcess < numOfProcesses)
        {
            int val = msgrcv(msgQ, &processRecv, 100 * sizeof(processRecv), 0, !IPC_NOWAIT);
            if (val == -1)
            {
                perror("Error in Receiving");
                break;
            }
            else
            {
                if (processRecv.valid == 1)
                {
                    countProcess++;
                    processRecv.remningTime = processRecv.runTime;
                    printf("%d %d %d %d RecvTime:%d\n", processRecv.id, processRecv.arrivalTime, processRecv.runTime, processRecv.priority, processRecv.sendTime);
                    queuePush(q, processRecv);
                    printQueue(q);
                    //if (processRecv.id == numOfProcesses)
                    //break;
                }
                else
                {
                    printf("Received a fake Process\n");
                    continue;
                }
            }
        }

        // to do : if it last process or the algorithm finish we will out from this loop , done
        if (countProcess == numOfProcesses && queueIsEmpty(q) && !isProcessRunNow)
            break;
    }
    printf("Schuder: hello i finished RR...\n");
}
void FCFS()
{
    printf("Schuder: hello i started FCFS...\n");
    q = (Queue *)malloc(sizeof(Queue));
    queueConstructor(q);
    Process running;
    *shmId = -1;
    bool isProcessRunNow=0;
    while (1)
    {
        // struct Process p;
        /* revivce process*/
        //printf("FCFS %d, sharmem %d\n",getClk(),*shmId);
        processRecv.valid = 0; //clear prev recv mess
        processRecv.mtype = 1;
        if (countProcess < numOfProcesses)
        {
            printf("FCFS : i will receive now\n");
            int val = msgrcv(msgQ, &processRecv, 100 * sizeof(processRecv), 0, !IPC_NOWAIT);
            if (val == -1)
            {
                perror("Error in Receiving");
                break;
            }
            else
            {
                if (processRecv.valid == 1)
                {
                    countProcess++;
                    processRecv.remningTime = processRecv.runTime;
                    printf("%d %d %d %d RecvTime:%d\n", processRecv.id, processRecv.arrivalTime, processRecv.runTime, processRecv.priority, processRecv.sendTime);
                    queuePush(q, processRecv);
                    printQueue(q);
                    //if (processRecv.id == numOfProcesses)
                    //break;
                }
                else
                {
                    printf("Received a fake Process\n");
                    continue;
                }
            }
        }

        if (*shmId <= 0&&isProcessRunNow) //running process is finish
        {
            printf("FCFS : i will end process now\n");
            finishProcess(running);
            isProcessRunNow=0;
        }
        // to do pick a new process
        printQueue(q);
        printf("run now :%d, queue status %d\n",isProcessRunNow,queueIsEmpty(q));
        if(!queueIsEmpty(q)&&!isProcessRunNow)
        {
            printf("FCFS : i will pick process now\n");
            struct Process front = queuePop(q);
            *shmId = front.remningTime;
            running = front;
            isProcessRunNow=1;
            running.pid = startProcess(running);
        }
        // to do : if it last process or the algorithm finish we will out from this loop done
        if (countProcess == numOfProcesses && queueIsEmpty(q) && !isProcessRunNow)
            break;
    }
    printf("Schuder: hello i finished FCFS...\n");
}
void SJF() //smallest running time first
{
    printf("Schuder: hello i started SJF...\n");
    pq = (priorityQueue *)malloc(sizeof(priorityQueue));
    priorityQueueConstructor(pq);
    Process running;
    *shmId = -1;
    bool isProcessRunNow=0;
    while (1)
    {
        // struct Process p;
        /* revivce process*/
        //printf("FCFS %d, sharmem %d\n",getClk(),*shmId);
        processRecv.valid = 0; //clear prev recv mess
        processRecv.mtype = 1;
        if (countProcess < numOfProcesses)
        {
            printf("SJF : i will receive now\n");
            int val = msgrcv(msgQ, &processRecv, 100 * sizeof(processRecv), 0, !IPC_NOWAIT);
            if (val == -1)
            {
                perror("Error in Receiving");
                break;
            }
            else
            {
                if (processRecv.valid == 1)
                {
                    countProcess++;
                    processRecv.remningTime = processRecv.runTime;
                    printf("%d %d %d %d RecvTime:%d\n", processRecv.id, processRecv.arrivalTime, processRecv.runTime, processRecv.priority, processRecv.sendTime);
                    priorityQueuePush(pq, processRecv,processRecv.runTime);
                    printPriorityQueue(pq);
                    //if (processRecv.id == numOfProcesses)
                    //break;
                }
                else
                {
                    printf("Received a fake Process\n");
                    continue;
                }
            }
        }

        if (*shmId <= 0&&isProcessRunNow) //running process is finish
        {
            printf("FCFS : i will end process now\n");
            finishProcess(running);
            isProcessRunNow=0;
        }
        // to do pick a new process
        //printQueue(q);
        //printf("run now :%d, queue status %d\n",isProcessRunNow,queueIsEmpty(q));
        if(!priorityQueueIsEmpty(pq)&&!isProcessRunNow)
        {
            printf("FCFS : i will pick process now\n");
            struct Process front = priorityQueuePop(pq);
            *shmId = front.remningTime;
            running = front;
            isProcessRunNow=1;
            running.pid = startProcess(running);
        }
        // to do : if it last process or the algorithm finish we will out from this loop done
        if (countProcess == numOfProcesses && priorityQueueIsEmpty(pq) && !isProcessRunNow)
            break;
    }
    printf("Schuder: hello i finished SJF...\n");
}
void cleanup(int signum)
{
    printf("Terminate shared memory from scheduler\n");
    shmctl(shmid, IPC_RMID, NULL);
    //shmctl(msgQ, IPC_RMID, NULL);
    msgctl(msgQ, IPC_RMID, (struct msqid_ds *)0);
    kill(getpid(), SIGKILL);
}