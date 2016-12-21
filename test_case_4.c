// Test of semaphores for the prodcuer-consumer problem

#include <stdio.h>
#include "uthread.h"

#define buffer_size 10
int buffer[buffer_size];

usem_t empty, full;

int *retval;

void producer(int);
void consumer(int);

int next_in = 0;
int next_out = 0;

int myNum = 0;

int main() {
	uthread_init();

	usem_init(&empty, 0, 10);
	usem_init(&full, 0, 0);

	uthread_create(producer, 1, 5);
	uthread_create(consumer, 2, 2);
	uthread_create(producer, 3, 8);
  
	printf("main thread done.\n");

	uthread_exit(&retval);

	return; // shouldn't get here because of uthread_exit()
}

void producer(int val) {
	while (1) {
		usem_wait(&empty);
		buffer[next_in] = val;
		printf("producer with value %d deposited an item into slot %d\n ",val, next_in);
		next_in++;
		if (next_in == buffer_size)
			next_in = 0;
		usem_post(&full);
		sleep(val / 10.0);
	}
}

void consumer(int val) {
	int item;
	while (1) {
		usem_wait(&full);
		item = buffer[next_out];
		printf("consumer with value %d consumed an item from slot %d\n ",val, next_out);
		next_out++;
		if (next_out == buffer_size)
			next_out = 0;
		usem_post(&empty);
	}
}









