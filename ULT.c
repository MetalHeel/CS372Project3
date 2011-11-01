#include <assert.h>
#include <ucontext.h>
#include <stdlib.h>

#include "ULT.h"

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */


static int yield = 0;
static int numThreads = 0;

Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  assert(1); /* TBD */
  return ULT_FAILED;
}

Tid ULT_Yield(Tid wantTid)
{
  yield = 1;
  ucontext_t uc;
  ThrdCtlBlk tcb;
  int tempTid;
  ListElement l;
  ListElement nextOne;

  getcontext(uc);

  // For just SELF.
  if(wantTid == ULT_SELF)
  {
    if(List_First(list) == List_Last(list))
    {
      tcb.tid = 0;
      tempTid = 0;
    }
    else
    {
      tcb.tid = List_Last(list).theTCB.tid + 1;
      tempTid = tcb.tid;
    }

    tcb.status = READY;
    tcb.stack = uc.uc_stack.ss_sp;
    tcb.context = uc;
    l.theTCB = tcb;

    List_Insert(l, LIST_ATFRONT(list));
  }

  // For ANY.
  if(wantTid == ULT_ANY)
  {
    if(numThreads <= 1024)
    {
      tcb.tid = List_Last(list).theTCB.tid + 1;  // DO CIRCULAR TIDs!
      tcb.status = READY;
      tcb.stack = uc.uc_stack.ss_sp;
      tcb.context = uc;
      l.theTCB = tcb;
      
      List_Insert(l, LIST_ATREAR(list)); 
    }
  }

  // Set next context.
  nextOne = List_First(list);
  nextOne.theTCB.status = RUNNING;
  setcontext(nextOne.theTCB.context);
  List_Remove(List_First(list));

  return nextOne.theTCB.tid;
}

Tid ULT_DestroyThread(Tid tid)
{
  assert(1); /* TBD */
  return ULT_FAILED;
}

void stub(void (*root)(void *), void *arg)
{
  // thread starts here
  Tid ret;
  root(arg); // call root function
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE); // we should only get here if we are the last thread.
  exit(0); // all threads are done, so process should exit
}





