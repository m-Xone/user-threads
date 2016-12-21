#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>

// public custom types
typedef int uthread_tid_t;
typedef struct uthread uthread;
typedef struct uthread_lib uthread_lib;
typedef struct queue queue;
typedef struct uthread_mutex_t {
  int lock, init, mtx_id; } uthread_mutex_t;
typedef struct usem_t {
  unsigned int value, sem_id;
  int pshared, init;
  uthread_mutex_t mutex; } usem_t;

int uthread_init(void);
int uthread_create(void(*func)(int), int val, int pri);
int uthread_yield(void);
void uthread_exit(void *retval);
int uthread_join(uthread_tid_t, void *retval);
int uthread_mutex_init(uthread_mutex_t *mutex);
int uthread_mutex_lock(uthread_mutex_t *mutex);
int uthread_mutex_unlock(uthread_mutex_t *mutex);
int usem_init(usem_t *sem, int pshared, unsigned value);
int usem_destroy(usem_t *sem);
int usem_wait(usem_t *sem);
int usem_post(usem_t *sem);
