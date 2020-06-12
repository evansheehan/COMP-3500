
/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 6/5/2017 to distribute to students to redo Lab 1                    *
* Updated 5/9/2017 for COMP 3500 labs                                         *
* Date  : February 20, 2009                                                   *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common2.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT, WTJQ} Metric;
typedef enum {FREEHOLES, PARKING} MemoryQueue;

/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define RR              3 

#define OMAP            0
#define PAGING          1
#define BESTFIT         2
#define WORSTFIT        3

#define MAXMETRICS      5 



/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/

typedef struct FreeMemoryHoleTag {
  Memory AddressFirstElement; //Address of first element
  Memory Size; //Size of the hole
  struct FreeMemoryHoleTag *previous; //Previous element in linked list
  struct FreeMemoryHoleTag *next; //Next element in linked list
} FreeMemoryHole;

typedef struct MemoryQueueParmsTag {
  FreeMemoryHole *Head;
  FreeMemoryHole *Tail;
  Quantity NumberOfHoles;
} MemoryQueueParms;

/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics 
MemoryQueueParms MemoryQueues[2];

int memPolicy = PAGING;
int pageSize = 256;
int numFrames = 0;
FreeMemoryHole *InitialMemoryHole;

/*****************************************************************************\
*                               Function prototypes                           *
\*****************************************************************************/

void                 ManageProcesses(void);
void                 NewJobIn(ProcessControlBlock whichProcess);
void                 BookKeeping(void);
Flag                 ManagementInitialization(void);
void                 LongtermScheduler();
void                 IO();
void                 CPUScheduler(Identifier whichPolicy);
ProcessControlBlock *SRTF();
void                 Dispatcher();
void                 EnqueueMemoryHole(MemoryQueue whichQueue, FreeMemoryHole *whichProcess);
FreeMemoryHole       *DequeueMemoryHole(MemoryQueue whichQueue);
void                 initializeMemoryHoles();

/*****************************************************************************\
* function: main()                                                            *
* usage:    Create an artificial environment operating systems. The parent    *
*           process is the "Operating Systems" managing the processes using   *
*           the resources (CPU and Memory) of the system                      *
*******************************************************************************
* Inputs: ANSI flat C command line parameters                                 *
* Output: None                                                                *
*                                                                             *
* INITIALIZE PROGRAM ENVIRONMENT                                              *
* START CONTROL ROUTINE                                                       *
\*****************************************************************************/

int main (int argc, char **argv) {
   if (Initialization(argc,argv)){
     ManageProcesses();
   }
} /* end of main function */

/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function: Monitor Sources and process events (written by students)    *
\***********************************************************************/

void ManageProcesses(void){
  ManagementInitialization();
  if (memPolicy == PAGING) {numFrames = ceil(AvailableMemory/pageSize);} //If running paging, calculate the number of frames in the physical space
  if (memPolicy == BESTFIT || memPolicy == WORSTFIT) {initializeMemoryHoles();}
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher();
  }
}

/***********************************************************************\
* Input : none                                                          *          
* Output: None                                                          *        
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (RR) return to rReady Queue                         *                           
*    2) scan Waiting Queue to find processes with complete I/O          *
*           and move them to Ready Queue                                *         
\***********************************************************************/
void IO() {
  ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE); 
  if (currentProcess){
    if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
      currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
      EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
      currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
      currentProcess->state = WAITING;
    } else { // Must return to Ready Queue                
      currentProcess->JobStartTime = Now();                                               
      EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
      currentProcess->state = READY; // Update PCB state 
    }
  }

  /* Scan Waiting Queue to find processes that got IOs  complete*/
  ProcessControlBlock *ProcessToMove;
  /* Scan Waiting List to find processes that got complete IOs */
  ProcessToMove = DequeueProcess(WAITINGQUEUE);
  if (ProcessToMove){
    Identifier IDFirstProcess =ProcessToMove->ProcessID;
    EnqueueProcess(WAITINGQUEUE,ProcessToMove);
    ProcessToMove = DequeueProcess(WAITINGQUEUE);
    while (ProcessToMove){
      if (Now()>=ProcessToMove->TimeIOBurstDone){
	ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
	ProcessToMove->JobStartTime = Now();
	EnqueueProcess(READYQUEUE,ProcessToMove);
      } else {
	EnqueueProcess(WAITINGQUEUE,ProcessToMove);
      }
      if (ProcessToMove->ProcessID == IDFirstProcess){
	break;
      }
      ProcessToMove =DequeueProcess(WAITINGQUEUE);
    } // while (ProcessToMove)
  } // if (ProcessToMove)
}

/***********************************************************************\    
 * Input : whichPolicy (1:FCFS, 2: SRTF, and 3:RR)                      *        
 * Output: None                                                         * 
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  if ((whichPolicy == FCFS) || (whichPolicy == RR)) {
    selectedProcess = DequeueProcess(READYQUEUE);
  } else{ // Shortest Remaining Time First 
    selectedProcess = SRTF();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running                                     
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
  }
}

/***********************************************************************\                         
 * Input : None                                                         *                                     
 * Output: Pointer to the process with shortest remaining time (SRTF)   *                                     
 * Function: Returns process control block with SRTF                    *                                     
\***********************************************************************/
ProcessControlBlock *SRTF() {
  /* Select Process with Shortest Remaining Time*/
  ProcessControlBlock *selectedProcess, *currentProcess = DequeueProcess(READYQUEUE);
  selectedProcess = (ProcessControlBlock *) NULL;
  if (currentProcess){
    TimePeriod shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
    Identifier IDFirstProcess =currentProcess->ProcessID;
    EnqueueProcess(READYQUEUE,currentProcess);
    currentProcess = DequeueProcess(READYQUEUE);
    while (currentProcess){
      if (shortestRemainingTime >= (currentProcess->TotalJobDuration - currentProcess->TimeInCpu)){
	EnqueueProcess(READYQUEUE,selectedProcess);
	selectedProcess = currentProcess;
	shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
      } else {
	EnqueueProcess(READYQUEUE,currentProcess);
      }
      if (currentProcess->ProcessID == IDFirstProcess){
	break;
      }
      currentProcess =DequeueProcess(READYQUEUE);
    } // while (ProcessToMove)
  } // if (currentProcess)
  return(selectedProcess);
}

/***********************************************************************\  
 * Input : None                                                         *   
 * Output: None                                                         *   
 * Function:                                                            *
 *  1)If process in Running Queue needs computation, put it on CPU      *
 *              else move process from running queue to Exit Queue      *     
\***********************************************************************/
void Dispatcher() {
  double start;
  ProcessControlBlock *processOnCPU = Queues[RUNNINGQUEUE].Tail; // Pick Process on CPU
  if (!processOnCPU) { // No Process in Running Queue, i.e., on CPU
    return;
  }
  if(processOnCPU->TimeInCpu == 0.0) { // First time this process gets the CPU
    SumMetrics[RT] += Now()- processOnCPU->JobArrivalTime;
    NumberofJobs[RT]++;
    processOnCPU->StartCpuTime = Now(); // Set StartCpuTime
  }
  
  if (processOnCPU->TimeInCpu >= processOnCPU-> TotalJobDuration) { // Process Complete
    printf(" >>>>>Process # %d complete, %d Processes Completed So Far <<<<<<\n",
	   processOnCPU->ProcessID,NumberofJobs[THGT]);   
    processOnCPU=DequeueProcess(RUNNINGQUEUE);
    EnqueueProcess(EXITQUEUE,processOnCPU);

    if (memPolicy == OMAP) {AvailableMemory += processOnCPU->MemoryRequested;} //Add back the memory this process took from available memory
    else if (memPolicy == PAGING) {numFrames += ceil(processOnCPU->MemoryRequested/pageSize);} //If using paging policy, add the freed frames back to the pool
    else if (memPolicy == BESTFIT) {
      AvailableMemory += processOnCPU->MemoryRequested; //Put total memory back in available
      FreeMemoryHole *backIntoFree = DequeueMemoryHole(PARKING);
      int i = 0;
      while (i < MemoryQueues[PARKING].NumberOfHoles)  {
        if (backIntoFree->AddressFirstElement == processOnCPU->TopOfMemory) {
          printf("Found the parked memory hole!\n");
          break;
        }
        EnqueueMemoryHole(PARKING, backIntoFree);
        backIntoFree = DequeueMemoryHole(PARKING);
        i++;
      }
    }
    processOnCPU->MemoryAllocated = 0; //The process no longer has any memory allocated to it

    NumberofJobs[THGT]++;
    NumberofJobs[TAT]++;
    NumberofJobs[WT]++;
    NumberofJobs[CBT]++;
    SumMetrics[TAT]     += Now() - processOnCPU->JobArrivalTime;
    SumMetrics[WT]      += processOnCPU->TimeInReadyQueue;


    // processOnCPU = DequeueProcess(EXITQUEUE);
    // XXX free(processOnCPU);

  } else { // Process still needs computing, out it on CPU
    TimePeriod CpuBurstTime = processOnCPU->CpuBurstTime;
    processOnCPU->TimeInReadyQueue += Now() - processOnCPU->JobStartTime;
    if (PolicyNumber == RR){
      CpuBurstTime = Quantum;
      if (processOnCPU->RemainingCpuBurstTime < Quantum)
	CpuBurstTime = processOnCPU->RemainingCpuBurstTime;
    }
    processOnCPU->RemainingCpuBurstTime -= CpuBurstTime;
    // SB_ 6/4 End Fixes RR 
    TimePeriod StartExecution = Now();
    OnCPU(processOnCPU, CpuBurstTime); // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTime
    processOnCPU->TimeInCpu += CpuBurstTime; // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTimeu
    SumMetrics[CBT] += CpuBurstTime;
  }
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: This routine is run when a job is added to the Job Queue    *
\***********************************************************************/
void NewJobIn(ProcessControlBlock whichProcess){
  ProcessControlBlock *NewProcess;
  /* Add Job to the Job Queue */
  NewProcess = (ProcessControlBlock *) malloc(sizeof(ProcessControlBlock));
  memcpy(NewProcess,&whichProcess,sizeof(whichProcess));
  NewProcess->TimeInCpu = 0; // Fixes TUX error
  NewProcess->RemainingCpuBurstTime = NewProcess->CpuBurstTime; // SB_ 6/4 Fixes RR
  EnqueueProcess(JOBQUEUE,NewProcess);
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); // Define which policy to run here.
}


/***********************************************************************\                                                   
* Input : None                                                         *                                                    
* Output: None                                                         *                                                    
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *                                                     
\***********************************************************************/
void BookKeeping(void){
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  // Compute averages and final results
  if (NumberofJobs[TAT] > 0){
    SumMetrics[TAT] = SumMetrics[TAT]/ (Average) NumberofJobs[TAT];
  }
  if (NumberofJobs[RT] > 0){
    SumMetrics[RT] = SumMetrics[RT]/ (Average) NumberofJobs[RT];
  }
  SumMetrics[CBT] = SumMetrics[CBT]/ Now();

  if (NumberofJobs[WT] > 0){
    SumMetrics[WT] = SumMetrics[WT]/ (Average) NumberofJobs[WT];
  }

  if (NumberofJobs[WTJQ] > 0) {
    SumMetrics[WTJQ] = SumMetrics[WTJQ]/ (Average) NumberofJobs[WTJQ];
  }
  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f AWTJQ=%f\n", 
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT], 
	 NumberofJobs[THGT]/Now(), SumMetrics[WT], SumMetrics[WTJQ]);
  printf("Memory policy %d finished using the %d CPU policy. If paging was used, the page size is: %d\n",
    memPolicy, PolicyNumber, pageSize);

  exit(0);
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: Decides which processes should be admitted in Ready Queue   *
*           If enough memory and within multiprogramming limit,         *
*           then move Process from Job Queue to Ready Queue             *
\***********************************************************************/
void LongtermScheduler() {
  ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);
  while (currentProcess) {
    int address = getStartAddress(currentProcess, memPolicy);
    if (address != -1) { //If there is memory available, add the process to the ready queue.
      currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
      currentProcess->JobStartTime = Now(); // Set JobStartTime

      SumMetrics[WTJQ] = SumMetrics[WTJQ] + currentProcess->TimeInJobQueue; //Add to the job queue waiting time sum
      printf("Time in Job Queue: %f\n", currentProcess->TimeInJobQueue);
      NumberofJobs[WTJQ]++; //Increment number of processes taken out of job queue
      printf("Sum WTJQ so far: %f...# Jobs WTJQ so far %d...\n", SumMetrics[WTJQ], NumberofJobs[WTJQ]);

      EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
      currentProcess->state = READY; // Update process state
      currentProcess = DequeueProcess(JOBQUEUE);
      }
    else { //If there is not memory available, put the process back on the job queue and return
      EnqueueProcess(JOBQUEUE, currentProcess);
      printf("**********There is not enough memory available for this process**********");
      return;
    }
    
  }
}


/***********************************************************************\
* Input : None                                                          *
* Output: TRUE if Intialization successful                              *
\***********************************************************************/
Flag ManagementInitialization(void){
  Metric m;
  for (m = TAT; m < MAXMETRICS; m++){
     NumberofJobs[m] = 0;
     SumMetrics[m]   = 0.0;
  }
  return TRUE;
}

int getStartAddress (ProcessControlBlock *whichProcess, int memPolicy) {

  if (memPolicy == OMAP) { //OMAP
    printf("Process ID %d is requesting %d units of memory. There are %d units of memory available.\n", whichProcess->ProcessID, whichProcess->MemoryRequested, AvailableMemory);
    if (AvailableMemory >= whichProcess->MemoryRequested) {
      AvailableMemory -= whichProcess->MemoryRequested;
      whichProcess->MemoryAllocated = whichProcess->MemoryRequested;
      return 0;
    }
    else {
      return -1;
    }
  }

  else if (memPolicy == PAGING) { //Paging
   int numPages = ceil(whichProcess->MemoryRequested/pageSize); //Calculate number of pages to divide this process into
   printf("Process ID %d is requesting %d frames. There are %d frames available.\n", whichProcess->ProcessID, numPages, numFrames);
   if (numPages <= numFrames) { //If there are frames available 
     numFrames -= numPages; // Allocate pages
     //AvailableMemory -= whichProcess->MemoryRequested; //Decrease total memory
     //whichProcess->MemoryAllocated = whichProcess->MemoryRequested;
     return 0;
   }
   else {
     return -1;
   }
  }

  else if (memPolicy == BESTFIT) { //Best-fit
    printf(">>>>>>BestFit policy entered...\n");
    if (AvailableMemory >= whichProcess->MemoryRequested) {
      printf(">>>>>>Total Memory is available...\n");
      FreeMemoryHole *currentHole = DequeueMemoryHole(FREEHOLES);
      if (!currentHole) {
        EnqueueMemoryHole(FREEHOLES, currentHole);
        return -1;
      }
      printf(">>>>>>currentHole is not NULL...\n");
      FreeMemoryHole *bestHole = currentHole;
      int i = 0;

      printf("Number of FreeHoles at the moment is...%d\n", MemoryQueues[FREEHOLES].NumberOfHoles);
      if (MemoryQueues[FREEHOLES].NumberOfHoles > 0) {
        printf(">>>>>>There are holes available!!!\n");
        while (i < MemoryQueues[FREEHOLES].NumberOfHoles) {
          currentHole = DequeueMemoryHole(FREEHOLES);
          if (!currentHole) {
            EnqueueMemoryHole(FREEHOLES, currentHole);
            return -1;
          }
          printf("currentHole is not NULL...\n");
          printf(">>>>>>Successfully dequeued a hole...\n");
          printf(">>>>>>Best hole size is: %d\n", bestHole->Size);
          printf(">>>>>>Current hole size is: %d\n", currentHole->Size);
          printf(">>>>>>Memory requested is: %d\n", whichProcess->MemoryRequested);
          if (bestHole->Size > currentHole->Size && whichProcess->MemoryRequested <= currentHole->Size) {
            EnqueueMemoryHole(FREEHOLES, bestHole);
            bestHole = currentHole;
            printf(">>>>>>New best hole being created...\n");
          }
          else {
            EnqueueMemoryHole(FREEHOLES, currentHole);
            printf(">>>>>>This hole is being put back in the queue...\n");
          }
          i++;
        }
      }
      //At this point, we SHOULD have the best free hole to allocate our processes' memory to.
      //Now we need to place this memory hole in parking and adjust the memory holes around it.
      if (bestHole->Size >= whichProcess->MemoryRequested) { //If the bestHole is large enough tos allocate our process

        AvailableMemory -= whichProcess->MemoryRequested; //Allocate memory
        whichProcess->TopOfMemory = bestHole->AddressFirstElement; //Set process memory address
        FreeMemoryHole *addToParking = bestHole; //Create another variable to be the consumed part of the hole
        addToParking->Size = whichProcess->MemoryRequested; //Hole added to parking is the size of the the mem req
        EnqueueMemoryHole(PARKING, addToParking); //Add to parking

        bestHole->AddressFirstElement += whichProcess->MemoryRequested; //Change starting address of the hole remainder
        bestHole->Size -= whichProcess->MemoryRequested; //Adjust size of the remainder hole
        EnqueueMemoryHole(FREEHOLES, bestHole);//Add remainder of bestHole back to free holes
        return 0;
      }
      else { //There is no hole large enough for our process RUN COMPACTION?
        EnqueueMemoryHole(PARKING, bestHole);
        return -1;
      }
    }
    else {
      return -1;
    }
  }

  else if (memPolicy == WORSTFIT) { //Worst-fit
    printf(">>>>>>WorstFit policy entered...\n");
    if (AvailableMemory >= whichProcess->MemoryRequested) {
      printf(">>>>>>Total Memory is available...\n");
      FreeMemoryHole *currentHole = DequeueMemoryHole(FREEHOLES);
      if (!currentHole) {
        EnqueueMemoryHole(FREEHOLES, currentHole);
        return -1;
      }
      printf(">>>>>>currentHole is not NULL...\n");
      FreeMemoryHole *bestHole = currentHole;
      int i = 0;

      printf("Number of FreeHoles at the moment is...%d\n", MemoryQueues[FREEHOLES].NumberOfHoles);
      if (MemoryQueues[FREEHOLES].NumberOfHoles > 0) {
        printf(">>>>>>There are holes available!!!\n");
        while (i < MemoryQueues[FREEHOLES].NumberOfHoles) {
          currentHole = DequeueMemoryHole(FREEHOLES);
          if (!currentHole) {
            EnqueueMemoryHole(FREEHOLES, currentHole);
            return -1;
          }
          printf("currentHole is not NULL...\n");
          printf(">>>>>>Successfully dequeued a hole...\n");
          printf(">>>>>>Best hole size is: %d\n", bestHole->Size);
          printf(">>>>>>Current hole size is: %d\n", currentHole->Size);
          printf(">>>>>>Memory requested is: %d\n", whichProcess->MemoryRequested);
          if (bestHole->Size < currentHole->Size && whichProcess->MemoryRequested <= currentHole->Size) {
            EnqueueMemoryHole(FREEHOLES, bestHole);
            bestHole = currentHole;
            printf(">>>>>>New best hole being created...\n");
          }
          else {
            EnqueueMemoryHole(FREEHOLES, currentHole);
            printf(">>>>>>This hole is being put back in the queue...\n");
          }
          i++;
        }
      }
      if (bestHole->Size >= whichProcess->MemoryRequested) { //If the bestHole is large enough tos allocate our process
        AvailableMemory -= whichProcess->MemoryRequested; //Allocate memory
        whichProcess->TopOfMemory = bestHole->AddressFirstElement; //Set process memory address
        FreeMemoryHole *addToParking = bestHole; //Create another variable to be the consumed part of the hole
        addToParking->Size = whichProcess->MemoryRequested; //Hole added to parking is the size of the the mem req
        EnqueueMemoryHole(PARKING, addToParking); //Add to parking

        bestHole->AddressFirstElement += whichProcess->MemoryRequested; //Change starting address of the hole remainder
        bestHole->Size -= whichProcess->MemoryRequested; //Adjust size of the remainder hole
        EnqueueMemoryHole(FREEHOLES, bestHole);//Add remainder of bestHole back to free holes
        return 0;
      }
      else { //There is no hole large enough for our process RUN COMPACTION?
        EnqueueMemoryHole(PARKING, bestHole);
        return -1;
      }
    }
    else {
      return -1;
    }
  }

  return -1;
}

void EnqueueMemoryHole(MemoryQueue whichQueue, FreeMemoryHole *whichMemoryHole){
  if (whichMemoryHole == (FreeMemoryHole *) NULL) {
    // printf("Tried to enqueue a null pointer in the %s \n",QueuesNames[whichQueue]);                               
    return;;
  }

  MemoryQueues[whichQueue].NumberOfHoles++;

  /* Enqueue the process in the queue */
  if (MemoryQueues[whichQueue].Head)
    MemoryQueues[whichQueue].Head->previous   = whichMemoryHole;
  whichMemoryHole->next      = MemoryQueues[whichQueue].Head;
  whichMemoryHole->previous  = NULL;
  MemoryQueues[whichQueue].Head = whichMemoryHole;
  if (MemoryQueues[whichQueue].Tail == NULL)
    MemoryQueues[whichQueue].Tail = whichMemoryHole;
}

FreeMemoryHole *DequeueMemoryHole(MemoryQueue whichQueue){
  FreeMemoryHole *HoleToRemove;

  HoleToRemove = MemoryQueues[whichQueue].Tail;
  if (HoleToRemove != (FreeMemoryHole *) NULL) {
    MemoryQueues[whichQueue].NumberOfHoles--;

    HoleToRemove->next = (FreeMemoryHole *) NULL;
    MemoryQueues[whichQueue].Tail = MemoryQueues[whichQueue].Tail->previous;

    HoleToRemove->previous =(FreeMemoryHole *) NULL;
    if (MemoryQueues[whichQueue].Tail == (FreeMemoryHole *) NULL){
      MemoryQueues[whichQueue].Head = (FreeMemoryHole *) NULL;
    } else {
      MemoryQueues[whichQueue].Tail->next = (FreeMemoryHole *) NULL;
    }
  }

  return(HoleToRemove);
}

void initializeMemoryHoles() {
  int i;
  //Initialize the queues
  for (i = 0; i < 2; i++) {
    MemoryQueues[i].Tail = (FreeMemoryHole *) NULL;
    MemoryQueues[i].Head = (FreeMemoryHole *) NULL;
    MemoryQueues[i].NumberOfHoles = 0;
  }
 
  // Create initial big memory holes containing the full memory
  InitialMemoryHole = (FreeMemoryHole *) malloc(sizeof(FreeMemoryHole));
  if (InitialMemoryHole) { // malloc successful
    InitialMemoryHole->AddressFirstElement = 0;
    InitialMemoryHole->Size = MAXMEMORYSIZE;
  }

  EnqueueMemoryHole(FREEHOLES, InitialMemoryHole);
  MemoryQueues[FREEHOLES].NumberOfHoles++;
}