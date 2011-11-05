#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "basicThreadTests.h"
#include "interrupt.h"
#include "ULT.h"
/*
 *  Using posix mutex for some tests of phases 1 and 2
 */
#include <pthread.h> 

static const int verbose = 0;
static const int vverbose = 0;

static void hello(char *msg);
static int fact(int n);
static void suicide();
static void finale();
static int setFlag(int val);



void basicThreadTests()
{
  Tid ret;
  Tid ret2;

  printf("Starting tests...\n");

  /*
   * Initial thread yields
   */
  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Initial thread returns from Yield(SELF)\n");
  ret = ULT_Yield(0); /* See ULT.h -- initial thread must be Tid 0 */
  assert(ULT_isOKRet(ret));
  printf("Initial thread returns from Yield(0)\n");
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("Initial thread returns from Yield(ANY)\n");
  ret = ULT_Yield(0xDEADBEEF);
  assert(ret == ULT_INVALID);
  printf("Initial thread returns from Yield(INVALID)\n");
  ret = ULT_Yield(16);
  assert(ret == ULT_INVALID);
  printf("Initial thread returns from Yield(INVALID2)\n");
  
  /*
   * Create a thread
   */
  ret = ULT_CreateThread((void (*)(void *))hello, "World");
  assert(ULT_isOKRet(ret));
  ret2 = ULT_Yield(ret);
printf("ret: %d\tret2: %d\n",ret,ret2);
  assert(ret2 == ret);

  /*
   * Create 10 threads
   */
  int ii;
  static const int NTHREAD = 10;
  Tid children[NTHREAD];
  char msg[NTHREAD][1024];
  for(ii = 0; ii < NTHREAD; ii++){
    ret = snprintf(msg[ii], 1023, "Hello from thread %d\n", ii);
    assert(ret > 0);
    children[ii] = ULT_CreateThread((void (*)(void *))hello, msg[ii]);
    assert(ULT_isOKRet(children[ii]));
  }
  for(ii = 0; ii < NTHREAD; ii++){
    ret = ULT_Yield(children[ii]);
    assert(ret == children[ii]);
  }


  /*
   * Destroy 11 threads we just created
   */
  ret = ULT_DestroyThread(ret2);
  assert(ret == ret2);
  for(ii = 0; ii < NTHREAD; ii++){
    ret = ULT_DestroyThread(children[ii]);
    assert(ret == children[ii]);
  }

  /*
   * Create maxthreads-1 threads
   */
  printf("Creating %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS-1; ii++){
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ULT_isOKRet(ret));
  }
  /*
   * Now we're out of threads. Next create should fail.
   */
  ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
  assert(ret == ULT_NOMORE);
  /*
   * Now let them all run.
   */
  printf("Running %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS; ii++){
    ret = ULT_Yield(ii);
    if(ii == 0){ 
      /* 
       * Guaranteed that first yield will find someone. 
       * Later ones may or may not depending on who
       * stub schedules  on exit.
       */
      assert(ULT_isOKRet(ret));
    }
  }
  /*
   * They should have cleaned themselves up when
   * they finished running. Create maxthreads-1 threads.
   */
  printf("Creating %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS-1; ii++){
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ULT_isOKRet(ret));
  }
  /*
   * Now destroy some explicitly and let the others run
   */
  printf("Destorying %d threads, running the rest\n",
         ULT_MAX_THREADS/2);
  for(ii = 0; ii < ULT_MAX_THREADS; ii+=2){
    ret = ULT_DestroyThread(ULT_ANY);
    assert(ULT_isOKRet(ret));
  }
  for(ii = 0; ii < ULT_MAX_THREADS; ii++){
    ret = ULT_Yield(ii);
  }
  printf("Trying some destroys even though I'm the only thread\n");
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ret == ULT_NONE);
  ret = ULT_DestroyThread(42);
  assert(ret == ULT_INVALID);
  ret = ULT_DestroyThread(-42);
  assert(ret == ULT_INVALID);
  ret = ULT_DestroyThread(ULT_MAX_THREADS + 1000);
  assert(ret == ULT_INVALID);

  /*
   * Create a tread that destroys itself. 
   * Control should come back here after
   * that thread runs.
   */
  printf("Testing destroy self\n");
  int flag = setFlag(0);
  ret = ULT_CreateThread((void (*)(void *))suicide, NULL);
  assert(ULT_isOKRet(ret));
  ret = ULT_Yield(ret);
  assert(ULT_isOKRet(ret));
  flag = setFlag(0);
  assert(flag == 1); /* Other thread ran */
  /* That thread is gone now */
  ret = ULT_Yield(ret);
  assert(ret == ULT_INVALID);

}

void 
grandFinale()
{
  int ret;
  printf("For my grand finale, I will destroy myself\n");
  printf("while my talented assistant prints Done.\n");
  ret = ULT_CreateThread((void (*)(void *))finale, NULL);
  assert(ULT_isOKRet(ret));
  ULT_DestroyThread(ULT_SELF);
  assert(0);

}


static void
hello(char *msg)
{
  Tid ret;

  printf("Made it to hello() in called thread\n");
  printf("Message: %s\n", msg);
  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from first yield\n");

  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from second yield\n");

  while(1){
    ULT_Yield(ULT_ANY);
  }
  
}

static int
fact(int n){
  if(n == 1){
    return 1;
  }
  return n*fact(n-1);
}

static void suicide()
{
  int ret = setFlag(1);
  assert(ret == 0);
  ULT_DestroyThread(ULT_SELF);
  assert(0);
}

/*
 *  Using posix mutex for some tests of phases 1 and 2
 */
static pthread_mutex_t posix_mutex = PTHREAD_MUTEX_INITIALIZER;
static int flagVal;

static int setFlag(int val)
{
  int ret;
  int err;
  err = pthread_mutex_lock(&posix_mutex);
  assert(!err);
  ret = flagVal;
  flagVal = val;
  err = pthread_mutex_unlock(&posix_mutex);
  assert(!err);
  return ret;
}


static void finale()
{
  int ret;
  printf("Finale running\n");
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("Done.\n");
  /* 
   * Stub should exit cleanly if there are no threads left to run.
   */
  return; 
}
