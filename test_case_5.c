// Test for scheduler

#include <stdio.h>
#include "uthread.h"

int *retval;

uthread_mutex_t mutex;

void myThread(int);

int main() {
	uthread_init();

	uthread_create(myThread, 1, 1);
	uthread_create(myThread, 2, 2);
	uthread_create(myThread, 3, 3);
	uthread_create(myThread, 25, 25);
	uthread_create(myThread, 68, 68);
	uthread_create(myThread, 27, 27);
	
	printf("main thread done.\n");

	uthread_exit(&retval);

	return; // shouldn't get here because of uthread_exit()
}

void myThread(int val) {
  int count = 0;
	while (count < 10) {
	        int i;
		for (i = 1; i < 1000000 * val; i++);
		printf("thread with value %d has completed %d iteration(s)\n", val, count);
		count++;
	}
	uthread_exit(&retval);
}









