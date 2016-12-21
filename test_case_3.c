// Test for join and exit functions

#include <stdio.h>
#include "uthread.h"

uthread_mutex_t my_mutex;

int *retval;

void increment(int);

int myNum = 0;

uthread_tid_t tid1, tid2, tid3;

int main() {
	uthread_init();

	uthread_mutex_init(&my_mutex);

	tid1 = uthread_create(increment, 1, 1);
	tid2 = uthread_create(increment, 2, 2);
	tid3 = uthread_create(increment, 3, 3);

	uthread_join(tid1, &retval);
	printf("joined tid1 with return code %d\n", *retval);
	uthread_join(tid2, &retval);
	printf("joined tid2 with return code %d\n", *retval);
	uthread_join(tid3, &retval);
	printf("joined tid3 with return code %d\n", *retval);
  
	printf("main thread done.\n");

	uthread_exit(&retval);

	return; // shouldn't get here because of uthread_exit()
}

void increment(int val) {
  	uthread_mutex_lock(&my_mutex);
	myNum += val;
	uthread_mutex_unlock(&my_mutex);
	uthread_exit(&retval);
}









