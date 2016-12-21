// Test for semaphores

#include <stdio.h>
#include "uthread.h"

usem_t my_sem;

int *retval;

void increment(int);
void dummy(int);

int myNum = 0;

int main() {
	uthread_init();

	usem_init(&my_sem, 0, 1);

	uthread_create(increment, 1, 1);
	uthread_create(increment, 2, 2);
	uthread_create(increment, 3, 3);
  
	printf("main thread done.\n");

	uthread_exit(&retval);

	return; // shouldn't get here because of uthread_exit()
}

void increment(int val) {
  	usem_wait(&my_sem);
	myNum += val;
	printf("myNum = %d\n", myNum);
	usem_post(&my_sem);
	if (myNum == 6) {
		usem_destroy(&my_sem);
		uthread_create(dummy, 4, 4);
	}
	uthread_exit(&retval);
}

void dummy(int val) {
	usem_wait(&my_sem);
	val++;
	usem_post(&my_sem);
	uthread_exit(&retval);
}









