#include <assert.h>
#include <stdio.h>
/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>

#include "ULT.h"

int first = 0; 

void clearKillList()
{
  while(killList != NULL)
  {
    ThrdCtlBlk* temp = killList;
    killList = killList->next;
    free(temp->topOfStack);
    free(temp);
  }
}

Tid nextTid()
{
  int q;
  for (q = 0; q < ULT_MAX_THREADS; ++q)
    if (tids[q] == 0)
    {
      tids[q] = 1;
      return (Tid)q;
    }
  return ULT_NOMORE;
}
void firstTime()
{
  if (first == 0)
  {
    // have now run for the first time
    first = 1;

    currentThread = (ThrdCtlBlk*)malloc(sizeof(ThrdCtlBlk));
    
    // save the current threads context
    getcontext(&currentThread->mContext);
    
    int q = 0;
    for (q = 0; q < ULT_MAX_THREADS; ++q)
      tids[q] = 0;
    
    // set up this threads tid and the next one's
    currentThread->mTid = nextTid();
    
    // initialize lists
    readyList = NULL;//(ThrdCtlBlk*)(malloc(10*sizeof(ThrdCtlBlk)));
    killList = NULL;//(int*)(malloc(10*sizeof(int)));
    readyListLength = 0;
    killListLength = 0;
  }
  else
  {
    clearKillList();
  }

  assert (first != 0);
}

// gets the TLB assoctiated with that Tid
ThrdCtlBlk* getTlb(Tid tid)
{
  //printf("getTlb(%i)\n", tid);
  if (tid < 0 || tid >= ULT_MAX_THREADS)
    return NULL;
  else  if (currentThread->mTid == tid)
    return currentThread;
  else if(readyList == NULL)
    return NULL;
  else if (tids[(int)tid] == 0)
    return NULL;

  int count = 1;
  ThrdCtlBlk *temp = readyList;
  if (temp->next == NULL)
    readyList = NULL;
  else if(temp->mTid == tid)
  {
    readyList = temp->next;
    temp->next = NULL;
  }
  else
  {

    //printf("In else\n");
    while (temp->next != NULL)
    {
      //if (count <=10)
      //printf("temp->mTid : %i == tid : %i\n", temp->mTid, tid);
      if (temp->next->mTid == tid)
      {
        ThrdCtlBlk *result = NULL;
        result = temp->next;
        temp->next = temp->next->next;
        result->next = NULL;
        temp = result;

        break;
      }
      temp = temp->next;
      ++count;
    }
  }
  //printf("count : %i\n", count);
  assert (temp->mTid == tid);
  return temp;
}
// gets the Tid of the next TCB in the ready list
ThrdCtlBlk* nextReady()
{
  return readyList;
}

volatile int doneThat = 0;
void switchTo(ThrdCtlBlk* newTlb)
{
  doneThat = 0;
  
  // use the switch in the notes!!!
#if 0
  printf("switchTo\n");
  printf("switchingTo %i\n", newTlb->mTid);
  printf("switchingFrom %i\n", currentThread->mTid);
#endif

  // save current context 
  getcontext(&currentThread->mContext);

  currentThread->next = NULL;
  //printf("readyListLength : %i\n", readyListLength);
  if (!doneThat)
  {
    //readyListLength = 14;
    doneThat = 1;
    //printf("Before if statement\n");
    if (newTlb != currentThread)
    {
      ThrdCtlBlk *temp = readyList;
      if (temp == NULL)
      {
        //printf("Ready list is null\n");
        // ready list is empty so just shove currentThread in there
        readyList = currentThread;
      }
      else
      {
        //printf("going through list\n");
        // loop to the end of the readylist and stick the current thread there
        while (temp->next != NULL)
          temp = temp->next;
        //printf("after loop\n");
        temp->next = currentThread;
        //printf("end of else\n");
      }
      
      assert(newTlb != NULL);
      assert(currentThread != NULL);

      currentThread = newTlb;
      //printf("before setcontext\n");
      setcontext(&currentThread->mContext);
      //printf("after setcontext\n");
    }
  }
  //printf("endswitchTo\n");
}

void addToKillList(ThrdCtlBlk* deadTlb)
{
  --readyListLength;
  tids[(int)deadTlb->mTid] = 0;
  deadTlb->next = killList;
  killList = deadTlb;
}



void stub (void (*root)(void *), void *arg)
{
  //printf("In Stubby\n");
  //printf("roots address 0x%x\n", (unsigned int)root);
  //printf("arg address 0x%x\n", (unsigned int)arg);
  Tid ret;
  root(arg);
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE);
  exit(0);
}

/*
Tid ULT_CreateThread(void (*fn)(void *), void *arg):   This function creates a thread whose starting point is the function fn. . Arg is a pointer that will be passed to the function when the thread starts executing; arg thus allows arguments to be passed to the function. Upon success, the function returns a thread identifier of type Tid. If the function fails, it returns a value that indicates the reason of failure as follows:

    * ULT_NOMORE alerts the caller that the thread package cannot create more threads.
    * ULT_NOMEMORY alerts the caller that the thread package could not allocate memory to create a stack of the desired size.

The created thread is put on a ready queue but does not start execution yet. The caller of the function continues to execute after the function returns.
*/
Tid 
ULT_CreateThread(void (*fn)(void *), void *parg)
{
  Tid result = ULT_NOMORE;
  firstTime();

  if (readyListLength + 1 < 1024)
  {
    ThrdCtlBlk* newThread = (ThrdCtlBlk*)malloc(sizeof(ThrdCtlBlk));
    if (newThread == NULL)
      return ULT_NOMEMORY;
    getcontext(&newThread->mContext); // initialize new threads context
    newThread->mTid = nextTid(); // set it to a new tid
    newThread->next = NULL;

    unsigned int* newStack = (unsigned int*)malloc(ULT_MIN_STACK);
    // LEAVE A POINTER TO THE TOP HERE WHEN WE FREE STUFF
    newThread->topOfStack = newStack;
    if (newStack == NULL)
      return ULT_NOMEMORY;
    newStack += (ULT_MIN_STACK - 1) / 4;
    
    //printf("fn address 0x%x\n", (unsigned int)fn);
    //printf("parg address 0x%x\n", (unsigned int)parg);

    *newStack = (unsigned int)parg; // put parg onto stack
    newStack -= sizeof(parg)/4;
    
    *newStack = (unsigned int)fn; // put parg onto stack
    newStack -= sizeof(fn)/4;
    
#if 0
    *newStack = (unsigned int)newThread->mContext.uc_mcontext.gregs[REG_EIP]; // put EIP onto stack
    newStack -= sizeof(newThread->mContext.uc_mcontext.gregs[REG_EIP]);
    *newStack = (unsigned int)newThread->mContext.uc_mcontext.gregs[REG_EBP]; // put EBP onto stack
    newThread->mContext.uc_mcontext.gregs[REG_EBP] = (int)newStack; // update EBP
    newStack -= sizeof(newThread->mContext.uc_mcontext.gregs[REG_EBP]);
#endif

    newThread->mContext.uc_mcontext.gregs[REG_EIP] = (int)&stub; // update EIP
    newThread->mContext.uc_mcontext.gregs[REG_ESP] = (int)newStack;

    if (readyList == NULL)
      readyList = newThread;
    else
    {
      ThrdCtlBlk *temp = readyList;
      while (temp->next != NULL)
        temp = temp->next;
      temp->next = newThread; // put the new thread on the ready list
    }

    ++readyListLength;
    result = newThread->mTid;
  }

  return result;
}



/*
Tid ULT_Yield(Tid tid):  This function suspends the caller and activates the thread given by the identifier tid. The caller is put on the ready queue and can be invoked later in a similar fashion. The value of tid may take the identifier of any available thread. It also can take any of the following constants:

    * ULT_ANY tells the thread system to invoke any thread on the ready queue. A sane policy is to run the thread at the head of the ready queue.
    * ULT_SELF tells the thread system to continue the execution of the caller.

The function returns the identifier of the thread that took control as a result of the function call. NOTE that the caller does not get to see the result until it gets its turn to run (later). The function also may fail and the caller resumes immediately. To indicate the reason for failure, the call returns one of these constants:

    * ULT_INVALID alerts the caller that the identifier tid does not correspond to a valid thread.
    * ULT_NONE alerts the caller that there are no more threads (other than the caller) available to run (in response to a call with tid set to ULT_SELF) or available to destroy (in response to a call wtih tid set to ULT_ANY.)

possibly, we should have the thread return 

*/
Tid ULT_Yield(Tid wantTid)
{
  //printf("ULT_YIELD : wantTid is %i\n", wantTid);
  Tid result = ULT_NONE;
  
  firstTime();
  
  if (ULT_ANY == wantTid)
  {
    // get the next TLB in the ready list
    if (readyListLength == 0)
      result = ULT_NONE;
    else
    {
      // might run into problems choosing the next thread before entering switchTo
      ThrdCtlBlk* newTlb = nextReady();
      readyList = readyList->next;
      result = newTlb->mTid;
      switchTo(newTlb);
    }
  }
  else if (ULT_SELF == wantTid || currentThread->mTid == wantTid)
  {
    result = currentThread->mTid;
  }
  else
  { 
    // other wise the thread should be in the ready list get the TLB of this tid
    //printf("YIELD : Start of else\n");
    ThrdCtlBlk* newTlb = getTlb(wantTid);

    if (newTlb == NULL)
        result = ULT_INVALID;
    else
    {
      //printf("YIELD(%i) : after getTlb : newTlb->mTid = %i\n", wantTid, newTlb->mTid);
      switchTo(newTlb);
      result = newTlb->mTid;
    }
  }
  return result;
}

/*
# Tid ULT_DestroyThread(Tid tid):  This function destroys the thread whose identifier is tid. The caller continues to execute and receives the result of the call. The value of tid may take the identifier of any available thread. It also can take any of the following constants:

    * ULT_ANY tells the thread system to destroy any thread except the caller.
    * ULT_SELF tells the thread system to destroy the caller.

Upon success, the function returns the identifier of the destroyed thread. The function also may fail and returns one of these constants:

    * The constant ULT_INVALID alerts the caller that the identifier tid does not correspond to a valid thread.
    * The constant ULT_NONE alerts the caller that there are no more other threads available to destroy (in response to a call with tid set to ULT_ANY).

*/
Tid ULT_DestroyThread(Tid tid)
{
  Tid result = ULT_NONE;
  
  firstTime();
  
  if (ULT_ANY == tid)
  {
    // get the next TLB in the ready list
    if (readyListLength == 0)
      result = ULT_NONE;
    else
    {
      ThrdCtlBlk* deadTlb = nextReady();
      readyList = readyList->next;
      deadTlb->next = NULL;
      result = deadTlb->mTid;
      addToKillList(deadTlb);
      //--readyListLength;
    }
  }
  else if (ULT_SELF == tid || currentThread->mTid == tid)
  {
    addToKillList(currentThread);
    // ASSUMES THERE IS ALWAYS SOMETHING ON THE READY LIST WHEN DESTROYING ITSELF 
    if (readyList != NULL)
    {
      //--readyListLength;
      currentThread = readyList;
      readyList = readyList->next;
      setcontext(&currentThread->mContext);
    }
  }
  else
  { 
    // other wise the thread should be in the ready list get the TLB of this tid
    ThrdCtlBlk* deadTlb = getTlb(tid);
    if (deadTlb == NULL)
        result = ULT_INVALID;
    else
    {
      deadTlb->next = NULL;
      result = deadTlb->mTid;
      addToKillList(deadTlb);
      //--readyListLength;
    }
  }
  return result;
}

