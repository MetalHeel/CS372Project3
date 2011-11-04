#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */

#include "list.c"


static List_Links list;

static int yield = 1;
static int numThreads = -1;


int search(int i, List_Links *node)
{
  int x;

  for(x = 0; x < numThreads; x++)
  {
    if(node->theTCB->tid == i)
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
    List_Init(&list);
    numThreads++;
  }

  if(numThreads == 1024)
    return ULT_NOMORE;

  List_Links l;
  List_InitElement(&l);
  int newtid;

  if(List_IsEmpty(&list))
    newtid = 1;
  else
    newtid = get_available_tid();

  l.theTCB->tid = newtid;
  l.theTCB->status = READY;

  getcontext(&(l.theTCB->context));

  // Set up stack.
  void* stack = malloc(ULT_MIN_STACK * 4);
  if(stack == NULL)
    return ULT_NOMEMORY;

  printf("here: %d\n", (unsigned int)l.theTCB->context.uc_stack.ss_sp);
  l.theTCB->context.uc_stack.ss_size = ULT_MIN_STACK;
  l.theTCB->context.uc_stack.ss_sp = stack + (ULT_MIN_STACK * 4 - 4);
  printf("broken\n");

  // Set up root.
  l.theTCB->context.uc_mcontext.gregs[REG_EIP] = (unsigned int)&stub;
  l.theTCB->context.uc_stack.ss_sp = parg;
  l.theTCB->context.uc_stack.ss_sp = l.theTCB->context.uc_stack.ss_sp - 4;
  l.theTCB->context.uc_stack.ss_sp = fn;
  l.theTCB->context.uc_stack.ss_sp = l.theTCB->context.uc_stack.ss_sp - 4;
  l.theTCB->context.uc_stack.ss_sp = 0x0;
  l.theTCB->context.uc_stack.ss_sp = l.theTCB->context.uc_stack.ss_sp + 8;

  // Set up ESP.
  l.theTCB->context.uc_stack.ss_sp = l.theTCB->context.uc_stack.ss_sp - 8;
  l.theTCB->context.uc_mcontext.gregs[REG_ESP] = (unsigned int)&l.theTCB->context.uc_stack.ss_sp;
  l.theTCB->context.uc_stack.ss_sp = l.theTCB->context.uc_stack.ss_sp + 8;

  if(numThreads == 0)
    List_Insert(&l, &list);
  else
    List_Insert(&l, LIST_ATREAR(&list));
  numThreads++;

  return l.theTCB->tid;
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

  List_Links l;
  List_InitElement(&l);
  List_Links nextOne;
  List_InitElement(&nextOne);
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
      nextOne = *List_First(&list);
      if(search(wantTid, &nextOne) < 0)
        return ULT_INVALID;
    }
    
    newtid = get_available_tid();
  }

  l.theTCB->tid = newtid;

  getcontext(&l.theTCB->context);       // Caller resumes here.
  l.theTCB->status = READY;

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
      List_Remove(&nextOne);
      setcontext(&nextOne.theTCB->context);
      return nextOne.theTCB->tid;
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





