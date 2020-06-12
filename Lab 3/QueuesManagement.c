
/*****************************************************************************\
* Laboratory Help      COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Date  : June 9, 2017                                                        *
* Help to manage queue of free memory holes                                   *
* This help create two queues : Free Memory Holes and a "Parking" queue       *
* The parking  queue can be handy when manipulating the main queue            *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/

#include "common2.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/
typedef enum {FREEHOLES, PARKING} MemoryQueue;

/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/


/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/
typedef struct FreeMemoryHoleTag{
  Memory       AddressFirstElement; // Address of first element 
  Memory       Size;                // Size of the hole
  struct FreeMemoryHoleTag *previous; /* previous element in linked list */
  struct FreeMemoryHoleTag *next;     /* next element in linked list */
} FreeMemoryHole;

typedef struct MemoryQueueParmsTag{
  FreeMemoryHole *Head;
  FreeMemoryHole *Tail;
  Quantity       NumberOfHoles; // Number of Holes in the queue
} MemoryQueueParms;





/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/
MemoryQueueParms    MemoryQueues[2]; // Free Holes and Parking

/*****************************************************************************\
*                               Function prototypes                           *
\*****************************************************************************/
void            EnqueueMemoryHole(MemoryQueue whichQueue,
				    FreeMemoryHole *whichProcess);
FreeMemoryHole *DequeueMemoryHole(MemoryQueue whichQueue);


/*****************************************************************************\
* function: main()                                                            *
* usage:    just to help students by prividing them with Enqueue/Dequeue      *
*******************************************************************************
* Inputs: None                                                                *
* Output: None                                                                *
\*****************************************************************************/

int main (int argc, char **argv) {
  FreeMemoryHole *NewMemoryHole;
  int i;
  //Initialize the queues
  for (i = 0; i < 2; i++){
    MemoryQueues[i].Tail          = (FreeMemoryHole *) NULL;
    MemoryQueues[i].Head          = (FreeMemoryHole *) NULL;
    MemoryQueues[i].NumberOfHoles = 0;
  }
 
  // Create initial big memory holes containing the full memory
  NewMemoryHole = (FreeMemoryHole *) malloc(sizeof(FreeMemoryHole));
  if (NewMemoryHole){ // malloc successful
    NewMemoryHole->AddressFirstElement = 0;
    NewMemoryHole->Size                = MAXMEMORYSIZE;
    
    // Just for the fun, put it in the parking
    EnqueueMemoryHole(PARKING,NewMemoryHole);

    // Move what was in the parking into the Queue of free holes
    FreeMemoryHole *aFreeMemoryHole;
    aFreeMemoryHole = DequeueMemoryHole(PARKING);
    EnqueueMemoryHole(FREEHOLES,aFreeMemoryHole);

    // Testing:
    printf("Number of holes in Parking = %d\n",MemoryQueues[PARKING].NumberOfHoles);
    if (MemoryQueues[PARKING].NumberOfHoles)
      printf("Parking should be empty");

                                                                                  
    printf("Number of holes in Free Holes = %d\n",MemoryQueues[FREEHOLES].NumberOfHoles);
    if (MemoryQueues[FREEHOLES].NumberOfHoles){
      printf("Starting Address %d\n",MemoryQueues[FREEHOLES].Tail->AddressFirstElement);
      printf("Size in hexadecimal 0x%x\n",MemoryQueues[FREEHOLES].Tail->Size);
    }

  }
} /* end of main function */



/***********************************************************************\                                            
 * Input : Queue where to enqueue and Element to enqueue                 *                                            
 * Output: Updates Head and Tail as needed                               *                                            
 * Function: Enqueues FIFO element in queue and updates tail and head    *                                            
\***********************************************************************/
void                 EnqueueMemoryHole(MemoryQueue whichQueue,
                                    FreeMemoryHole *whichMemoryHole){
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


/***********************************************************************\                                            
 * Input : Queue where to enqueue and Element to enqueue                *                                            
 * Output: Returns tail of queue                                        *                                            
 * Function: Removes tail elelemnt and updates tail and head as needed  *                                            
\***********************************************************************/
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
