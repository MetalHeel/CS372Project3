#ifndef _ULT_H_
#define _ULT_H_
#include <ucontext.h>
#include <stdlib.h>


typedef int Tid;
#define ULT_MAX_THREADS 1024
#define ULT_MIN_STACK 32768



typedef struct ThrdCtlBlk{
  Tid mTid; // tid for this thread
  struct ucontext mContext; // context for this thread
  void* topOfStack; // pointer to the top of the stack (bottom from x86 perspective, what malloc returns so we can free
  struct ThrdCtlBlk *next;  //if a part of a list the next member of the list, null if the last member
} ThrdCtlBlk;


ThrdCtlBlk* currentThread;
ThrdCtlBlk* readyList;
int readyListLength; // the next tid is the length of the readyList
ThrdCtlBlk* killList;
int killListLength;
// if zero not used, if 1 used
short tids[ULT_MAX_THREADS];

/*
 * Tids between 0 and ULT_MAX_THREADS-1 may
 * refer to specific threads and negative Tids
 * are used for error codes or control codes.
 * The first thread to run (before ULT_CreateThread
 * is called for the first time) must be Tid 0.
 */
static const Tid ULT_ANY = -1;
static const Tid ULT_SELF = -2;
static const Tid ULT_INVALID = -3;
static const Tid ULT_NONE = -4;
static const Tid ULT_NOMORE = -5;
static const Tid ULT_NOMEMORY = -6;
static const Tid ULT_FAILED = -7;

static inline int ULT_isOKRet(Tid ret){
  return (ret >= 0 ? 1 : 0);
}

Tid ULT_CreateThread(void (*fn)(void *), void *parg);
Tid ULT_Yield(Tid tid);
Tid ULT_DestroyThread(Tid tid);

// gets the TLB assoctiated with that Tid
ThrdCtlBlk* getTlb(Tid tid);
// gets the Tid of the next TCB in the ready list
ThrdCtlBlk* nextReady();
// switches context to the TCB
void switchTo(ThrdCtlBlk*);

 




#endif

