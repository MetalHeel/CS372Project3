#include <assert.h>
#include <ucontext.h>
#include <stdlib.h>

#include "list.c"

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */


static List_Links list;

static int yield = 1;
static int numThreads = -1;


int search(int i, List_Links *node)
{
  if(node->theTCB->tid == i)
    return i;
  else if(List_IsAtEnd(&list, node))
    return -1;
  else
    return search(i, node->nextPtr);
}

int get_available_tid()
{
  if(numThreads == 1024)
    return -1;
  else
  {
    int x;
    for(x = 0; x < 1023; x++)
      if(search(x, List_First(&list)) < 0)
        return x;
  }

  return -1;
}

Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  if(numThreads == -1)
  {
    List_Init(&list);
    numThreads++;
  }
  assert(1); /* TBD */
  numThreads++;
  return ULT_FAILED;
}

Tid ULT_Yield(Tid wantTid)
{
  if(numThreads == -1)
  {
    List_Init(&list);
    numThreads++;
  }

  if((wantTid < -2) || (wantTid > 1023))
    return ULT_INVALID;

  ucontext_t uc;
  ThrdCtlBlk tcb;
  tcb.tid = -1;
  List_Links l;
  List_InitElement(&l);
  List_Links nextOne;
  List_InitElement(&nextOne);

  if(wantTid == ULT_SELF)
  {
    if(List_IsEmpty(&list))
      tcb.tid = 0;
    else
      tcb.tid = get_available_tid();
  }
  else if(List_IsEmpty(&list))
  {
    if(wantTid == 0)
    {
      tcb.tid = 0;
      wantTid = ULT_SELF;
    }
    else
      return ULT_NONE;
  } 
  else		// Either ULT_ANY or a specific tid.
  {
    if(wantTid >= 0)	// Specific tid.
    {
      nextOne = *List_First(&list);
      if(search(wantTid, &nextOne) < 0)
        return ULT_INVALID;
    }

    tcb.tid = get_available_tid();
  }

  tcb.status = READY;
  tcb.stack = uc.uc_stack;
  tcb.context = uc;
  l.theTCB = &tcb;

  getcontext(&l.theTCB->context);       // Caller resumes here.

  if(yield)
  {
    yield = 0;

    if(wantTid == ULT_SELF)
      List_Insert(&l, LIST_ATFRONT(&list));
    else if(wantTid == ULT_ANY)
      List_Insert(&l, LIST_ATREAR(&list));
    else
    {
      List_Insert(&l, LIST_ATREAR(&list));
      int check = search(wantTid, &nextOne);

      if(check >= 0)
      {
        List_Remove(&nextOne);
        setcontext(&nextOne.theTCB->context);
        return nextOne.theTCB->tid;
      }
      else
        return ULT_INVALID;
    }

    // Set next context.
    nextOne = *List_First(&list);
    List_Remove(List_First(&list));
    nextOne.theTCB->status = RUNNING;
    Tid returnTid = nextOne.theTCB->tid;
    setcontext(&nextOne.theTCB->context);

    assert(0 <= returnTid);

    return returnTid;
  }
  else
  {
    yield = 1;
    return l.theTCB->tid;
  }
}

Tid ULT_DestroyThread(Tid tid)
{
  assert(1); /* TBD */
  numThreads--;
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





