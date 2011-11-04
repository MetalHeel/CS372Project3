#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */

#include "ULT.h"


List_Links list;

static int yield = 1;
static int numThreads = -1;


int search(int i, List_Links *node)
{
  int x;

  for(x = 0; x < numThreads; x++)
  {
    if(*((int *)(node + 1)) == i)
      return i;
    node = node->nextPtr;
  }

  return -1;
}

int get_available_tid()
{
  if(numThreads == 1024)
    return -1;
  else
  {
    int x;
    for(x = 0; x < 1024; x++)
    {
      List_Links temp;
      List_InitElement(&temp);
      temp = *List_First(&list);
      if(search(x, &temp) < 0)
        return x;
    }
  }

  return -1;
}


Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  if(numThreads == -1)
  {
    List_InitElement(&list);
    List_Init(&list);
    numThreads++;
  }

  if(numThreads == 1024)
    return ULT_NOMORE;

  ucontext_t uc;
  getcontext(&uc);

  // Set up stack.
  void* stack = malloc(ULT_MIN_STACK * 4);
  if(stack == NULL)
    return ULT_NOMEMORY;
  uc.uc_stack.ss_size = ULT_MIN_STACK;
  uc.uc_stack.ss_sp = stack + (ULT_MIN_STACK * 4 - 4);

  // Set up root.
  uc.uc_mcontext.gregs[REG_EIP] = (unsigned int)&stub;
  uc.uc_stack.ss_sp = parg;
  uc.uc_stack.ss_sp = uc.uc_stack.ss_sp - 4;
  uc.uc_stack.ss_sp = fn;
  uc.uc_stack.ss_sp = uc.uc_stack.ss_sp - 4;
  uc.uc_stack.ss_sp = 0x0;

  // Set up ESP.
  uc.uc_mcontext.gregs[REG_ESP] = (unsigned int)&uc.uc_stack.ss_sp;
  uc.uc_stack.ss_sp = uc.uc_stack.ss_sp + 8;

  ThrdCtlBlk tcb;
  List_InitElement(&tcb.links);
  int newtid;

  if(List_IsEmpty(&list))
    newtid = 1;
  else
    newtid = get_available_tid();

  tcb.tid = newtid;
  tcb.status = READY;
  tcb.context = uc;

  if(numThreads == 0)
    List_Insert(&tcb.links, &list);
  else
    List_Insert(&tcb.links, LIST_ATREAR(&list));

  numThreads++;

  return newtid;
}


Tid ULT_Yield(Tid wantTid)
{
  if(numThreads == -1)
  {
    List_InitElement(&list);
    List_Init(&list);
    numThreads++;
  }

  if((wantTid < -2) || (wantTid > 1023))
    return ULT_INVALID;

  ThrdCtlBlk insert;
  ThrdCtlBlk next;
  List_InitElement(&insert.links);
  List_InitElement(&next.links);
  int newtid;

  if(wantTid == ULT_SELF)
  {
    if(List_IsEmpty(&list))
      newtid = 0;
    else
      newtid = get_available_tid();
  }
  else if(List_IsEmpty(&list))
  {
    if(wantTid == 0)
    {
      newtid = 0;
      wantTid = ULT_SELF;
    }
    else if(wantTid == ULT_ANY)
      return ULT_NONE;
    else
      return ULT_INVALID;
  } 
  else
  {
    if(wantTid >= 0)	// Specific tid.
    {
      next.links = *List_First(&list);
      if(search(wantTid, &next.links) < 0)
        return ULT_INVALID;
    }
    
    newtid = get_available_tid();
  }

  insert.tid = newtid;
  getcontext(&insert.context);       // Caller resumes here.
  insert.status = READY;

  if(yield)
  {
    yield = 0;
    if(wantTid == ULT_SELF)
      List_Insert(&insert.links, LIST_ATFRONT(&list));
    else if(wantTid == ULT_ANY)
      List_Insert(&insert.links, LIST_ATREAR(&list));
    else
    {
      List_Insert(&insert.links, LIST_ATREAR(&list));
      List_Remove(&next.links);
      setcontext(&nextOne.theTCB->context);
      return nextOne.theTCB->tid;
    }

    // Set next context.
    // nextOne = *List_First(&list);
    ucontext_t newcontext = List_First(&list)->theTCB->context;
    Tid returnTid = List_First(&list)->theTCB->tid;
    List_Remove(List_First(&list));
    setcontext(&newcontext);

    assert(0 <= returnTid);

    return returnTid;
  }
  else
  {
    yield = 1;
    return yl.theTCB->tid;
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





