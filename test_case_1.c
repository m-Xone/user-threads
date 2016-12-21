// Test for mutex locks

#include <stdio.h>
#include "uthread.h"

uthread_mutex_t my_mutex;

int *retval;

void increment(int);

int myNum = 0;

int main() {
	uthread_init();

	uthread_mutex_init(&my_mutex);

	uthread_create(increment, 1, 1);
	uthread_create(increment, 2, 2);
	uthread_create(increment, 3, 3);
  
	printf("main thread done.\n");

	uthread_exit(&retval);

	return; // shouldn't get here because of uthread_exit()
}


void increment(int val) {
  	uthread_mutex_lock(&my_mutex);
	myNum += val;
	printf("myNum = %d\n", myNum);
	uthread_mutex_unlock(&my_mutex);
	uthread_exit(&retval);
}









