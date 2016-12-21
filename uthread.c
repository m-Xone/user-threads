// =========================================== //
// Date:        25 October 2016                //
// File:        uthread.c                      //
// Assignment:  Project 3                      //
// Course:      OS6456 Operating Systems, F16  // 
// Name:        William Young                  //
// Description: User-level Thread Library      //
// =========================================== //
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "uthread.h"

// definitions
#define STACK_SIZE 1024 * 1024
#define SEM_VALUE_MAX 65536
#define SLOTS 50

// private custom types
struct uthread {
  int id, pri, exit, lck_blocked, sem_blocked, j_blocked;
  ucontext_t ct; };
struct queue {
  uthread *item;
  int head, tail, num_tot_history, num_queued_threads; };
struct uthread_lib {
  int num_nodes, num_lck_blocked_threads, num_sem_blocked_threads, num_proc_threads, caller, *rets, *sem_avail, *mutex_avail, *exits, *joins;
  queue *queues, *lck_queue, *sem_queue;
  uthread_mutex_t *local_mutex;
  uthread global_curr;
  usem_t *local_usem; };

// globals
int init = 0;
uthread_lib* threadlib;
int sem_id = 1;
int mtx_id = 1;

// internal methods
int select_highest_priority_queue(void);

int uthread_init(void) {
  // check/set init status
  if(init != 0) { return -1; }
  init = 1;
  // create threadlib object and initialize status variables
  threadlib = (uthread_lib*)malloc(sizeof(uthread_lib));
  if(threadlib == NULL) { init = 0; return -1; }
  // status registers
  threadlib->rets = (int*)malloc(50*sizeof(int));
  threadlib->exits = (int*)calloc(50,sizeof(int));
  threadlib->sem_avail = malloc(30*sizeof(int));
  threadlib->local_mutex = malloc(5*sizeof(uthread_mutex_t));
  threadlib->mutex_avail = malloc(30*sizeof(int));
  // counters
  threadlib->num_lck_blocked_threads = 0;
  threadlib->num_sem_blocked_threads = 0;
  threadlib->num_proc_threads = 1;
  threadlib->num_nodes = 0;
  // queue setup
  threadlib->queues = (queue*)malloc(10 * sizeof(queue));
  int i;
  for(i = 0; i < 10; i++) {
    threadlib->queues[i].item = malloc(SLOTS * sizeof(uthread));
    threadlib->queues[i].head = 0;
    threadlib->queues[i].tail = -1;
    threadlib->queues[i].num_queued_threads = 0;
    threadlib->queues[i].num_tot_history = 0; }
  // OTHER SETUP
  threadlib->caller = 0; // no valid ID
  srand((unsigned int)time(NULL));
  // initialize current global state
  threadlib->global_curr.id = 0;
  threadlib->global_curr.lck_blocked = 0;
  threadlib->global_curr.exit = 0;
  threadlib->global_curr.sem_blocked = 0;
  threadlib->global_curr.j_blocked = -1;
  threadlib->global_curr.pri = 99;
  // SCHEDULE TIMING
  struct sigaction sa;
  struct itimerval itimer;
  sa.sa_handler = (void*) &uthread_yield;
  sigaction(SIGVTALRM, &sa, NULL);
  itimer.it_value.tv_sec = 0;
  itimer.it_value.tv_usec = 1000;
  itimer.it_interval.tv_sec = 0;
  itimer.it_interval.tv_usec = 1000;
  setitimer(ITIMER_VIRTUAL, &itimer, NULL);
  return 0;
}

int uthread_create(void (*func)(int), int val, int pri) {
  // setup new thread
  uthread new_thread;
  new_thread.id = ++threadlib->num_nodes;
  new_thread.exit = 0;
  new_thread.pri = pri;
  new_thread.j_blocked = -1;
  new_thread.lck_blocked = 0;
  new_thread.sem_blocked = 0;
  threadlib->rets[new_thread.id] = -3091995;
  // add to queue
  int q = pri / 10; // queue
  threadlib->queues[q].item[threadlib->queues[q].num_tot_history++] = new_thread;
  threadlib->queues[q].tail++;
  int c = threadlib->queues[q].num_tot_history;
  int n = c-1;
  // generate new thread context
  getcontext(&threadlib->queues[q].item[n].ct);
  // prepare the context for makecontext() invocation
  char* func_stack = malloc(STACK_SIZE * sizeof(char)); // init stack
  threadlib->queues[q].item[n].ct.uc_stack.ss_sp = func_stack; // assign stack
  threadlib->queues[q].item[n].ct.uc_stack.ss_size = STACK_SIZE; // assign stack size
  makecontext(&threadlib->queues[q].item[n].ct, (void*)func, 1, val);
  // update counter and return
  threadlib->queues[q].num_queued_threads++;
  //threadlib->queues[q].num_tot_history++;
  threadlib->num_proc_threads++;
  // state for initialization
  if(threadlib->caller == 0) {
    uthread main_t;
    main_t.id = 0;
    threadlib->rets[main_t.id] = -3091995;
    int a = threadlib->global_curr.pri/10;
    int b = threadlib->queues[a].head;
    threadlib->queues[a].item[b] = main_t;
    char* func_stack = malloc(STACK_SIZE * sizeof(char)); // init stack
    threadlib->queues[a].item[b].ct.uc_stack.ss_sp = func_stack; // assign stack
    threadlib->queues[a].item[b].ct.uc_stack.ss_size = STACK_SIZE; // assign stack size
    threadlib->queues[a].num_tot_history++;
    threadlib->queues[a].num_queued_threads++;
    threadlib->joins = (int*)malloc(30*sizeof(int));
    int j;
    for(j = 0; j < 30; j++) {
      threadlib->joins[j] = -1;
    }
    threadlib->caller = 1; }
  return (threadlib->queues[q].item[n].id);
}

int uthread_yield(void) {
  // QUEUE SELECTION 
  int q = select_highest_priority_queue(); // returns high priority queue with available work
  // vars
  int s = threadlib->global_curr.pri / 10;
  int n = threadlib->queues[q].num_tot_history;
  int r = threadlib->queues[s].num_tot_history;
  // LOAD SLOT
  uthread next_up = threadlib->queues[q].item[(threadlib->queues[q].head++) % n];
  // ONLY ONE THREAD IN HIGH PRIORITY QUEUE? switch back to it on swapcontext
    if(threadlib->queues[q].num_queued_threads == 1 && threadlib->num_proc_threads > 1) {
      threadlib->queues[q].head--; }
  // LOCKS
  if(next_up.lck_blocked > 0 && threadlib->mutex_avail[next_up.lck_blocked] == 1) { 
    next_up.lck_blocked = 0; }
  // SEMAPHORES
  if(next_up.sem_blocked > 0 && threadlib->sem_avail[next_up.sem_blocked] == 1) {
    next_up.sem_blocked = 0; }
  // JOIN
  if(next_up.j_blocked > -1) {
    if(threadlib->joins[next_up.j_blocked] == -99) {
      next_up.j_blocked = -2; } }
  // NEXT THREAD CAN'T BE RUN?
  if(threadlib->queues[q].num_queued_threads > 0 && (next_up.exit == 1 || next_up.lck_blocked > 0 || next_up.sem_blocked > 0 || threadlib->exits[next_up.id] == 1 || next_up.j_blocked > -1)) {
    uthread_yield(); }

  if(next_up.exit != 1) {
  // SAVE SLOT
  uthread* save_slot = &threadlib->queues[s].item[(++threadlib->queues[s].tail) % r];  
  // SAVE THREAD METADATA
  save_slot->exit = threadlib->global_curr.exit;
  save_slot->pri = threadlib->global_curr.pri;
  save_slot->lck_blocked = threadlib->global_curr.lck_blocked;
  save_slot->sem_blocked = threadlib->global_curr.sem_blocked;
  save_slot->j_blocked = threadlib->global_curr.j_blocked;
  save_slot->id = threadlib->global_curr.id;
  // UPDATE GLOBAL METADATA
  threadlib->global_curr = next_up; // accurate after swapcontext() is called (allows us to save current context's thread-ID)
  // CONTEXT SWITCH
  swapcontext(&save_slot->ct,&next_up.ct);
  }
  return 0;
}

int select_highest_priority_queue(void) {
  // will ONLY select a queue with at least one active thread (not blocked)
  int i, p;
  for(i = 0; i < 10; i++) {
    if(threadlib->queues[i].num_queued_threads > 0) {
      p = i;
      break; } }
  return p; }

void uthread_exit(void *retval) {
  // helper vars
  int i;
  int id = threadlib->global_curr.id;
  int pri = threadlib->global_curr.pri;
  // mark thread for termination
  threadlib->global_curr.exit = 1;
  threadlib->queues[pri/10].num_queued_threads--;
  threadlib->num_proc_threads--;
  threadlib->exits[id] = 1;
  // check for joins
  if(threadlib->joins[id] > -1) {
    threadlib->joins[id] = -99; }
  // save return value in array (use threadID as index)
  retval = (void*)((rand() % 256) + 1); // retval between 1 and 256
  threadlib->rets[id] = (int)retval;
  // if at least one thread is still active, yield to it
  if(threadlib->num_proc_threads > 0) {
    uthread_yield(); }
  // last thread -> goodbye, cruel world
  else {
    // free all memory
    for(i = 0; i < 10; i++) { free(threadlib->queues[i].item); }
    free(threadlib->rets);
    free(threadlib->joins);
    free(threadlib->queues);
    free(threadlib);
    exit(0);
  } }

int usem_wait(usem_t *sem) {
  // fails if sem doesn't exist, or if sem has been destroyed
  if(!sem || sem->init == 0) { printf("usem_wait error: usem_t does not exist\n");return -1; }
  if(sem->value < 1) {
    threadlib->sem_avail[sem->sem_id] = 0;
    // block until possible to decrement
    threadlib->global_curr.sem_blocked = sem->sem_id;
    threadlib->num_sem_blocked_threads++;
    uthread_yield();
  }
  uthread_mutex_lock(&sem->mutex);
  sem->value--;
  uthread_mutex_unlock(&sem->mutex);
  return 0;
}

int usem_post(usem_t *sem) {
  // fails if sem doesn't exist, or if sem has been destroyed
  if(!sem || sem->init == 0) { printf("usem_post error: usem_t does not exist\n");return -1; }
  uthread_mutex_lock(&sem->mutex);
  if(++sem->value > 0) {
    threadlib->sem_avail[sem->sem_id] = 1;
  }
  uthread_mutex_unlock(&sem->mutex); 
  return 0;
}

int usem_init(usem_t *sem, int pshared, unsigned value) {
  // failure
  if(sem->init == 1 || value < 0 || value > SEM_VALUE_MAX || pshared != 0) { return -1; }
  else {
    usem_t temp;
    temp.value = (unsigned int) value;
    temp.pshared = pshared;
    temp.mutex = threadlib->local_mutex[sem->sem_id]; //(uthread_mutex_t*)malloc(sizeof(uthread_mutex_t));
    uthread_mutex_init(&temp.mutex);
    temp.sem_id = sem_id++;
    threadlib->sem_avail[temp.sem_id] = 1;
    temp.init = 1;
    *sem = temp;
  }
  return 0;
}

int usem_destroy(usem_t *sem) {
  // fails if sem doesn't exist, or if there are blocked threads
  if(!sem || threadlib->num_sem_blocked_threads > 0 || sem->init == 0) { return -1; }
  sem->init = 0;
  return 0; 
}

int uthread_join(uthread_tid_t tid, void *retval) {
  //printf("JOIN on thread %d\n",tid);
  if(tid < 0 || tid > threadlib->num_nodes) { printf("uthread_join error: invalid thread id\n");return -1; } // invalid tid
  if(threadlib->joins[tid] > 0) { printf("uthread_join error: thread already waiting to join\n");return -1; } // thread already waiting to join tid
  // if thread hasn't terminated, block
  if(threadlib->rets[tid] == -3091995) {
    threadlib->global_curr.j_blocked = tid;
    threadlib->joins[tid] = threadlib->global_curr.id; // indicate that someone is waiting to join this thread
    uthread_yield(); }
  // thread terminated; return value
  int* ret = &(threadlib->rets[tid]);
  *((int**)retval) = (void*)ret;
  
  return 0;
}

int uthread_mutex_init(uthread_mutex_t *mutex) {
  if(mutex->init == 1) { printf("uthread_mutex_init error: mutex already initialized\n"); return -1; } // already initialized
  else {
    uthread_mutex_t temp;
    temp.lock = 0;
    temp.init = 1;
    temp.mtx_id = mtx_id++;
    *mutex = temp;
  }
  return 0;
}

int uthread_mutex_lock(uthread_mutex_t *mutex) {
  if(!mutex) { printf("uthread_mutex_init error: mutex does not exist\n");return -1; } // invalid mutex
  // block and lock
  if(mutex->lock == 1) {
    threadlib->global_curr.lck_blocked = mutex->mtx_id;
    threadlib->num_lck_blocked_threads++;
    uthread_yield();
  }
  else {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    mutex->lock = 1;
    threadlib->mutex_avail[mutex->mtx_id] = 0;
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
  }
  return 0;
}

int uthread_mutex_unlock(uthread_mutex_t *mutex) {
  if(!mutex || mutex->lock == 0) { printf("uthread_mutex_init error: mutex does not exist\n");return -1; } // invalid mutex, or already unlocked
  if(mutex->lock == 1) {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &blocked, NULL);
    mutex->lock = 0;
    threadlib->mutex_avail[mutex->mtx_id] = 1;
    sigprocmask(SIG_UNBLOCK, &blocked, NULL);
  }
  return 0;
}
