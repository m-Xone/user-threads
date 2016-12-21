all:	test_case_1 test_case_2 test_case_3 test_case_4 test_case_5
	rm -f *~ *.o *.h.gch

test_case_1:	test_case_1.o uthread.o
	gcc -o test_case_1 test_case_1.o uthread.o

test_case_2:	test_case_2.o uthread.o
	gcc -o test_case_2 test_case_2.o uthread.o

test_case_3:	test_case_3.o uthread.o
	gcc -o test_case_3 test_case_3.o uthread.o

test_case_4:	test_case_4.o uthread.o
	gcc -o test_case_4 test_case_4.o uthread.o

test_case_5:	test_case_5.o uthread.o
	gcc -o test_case_5 test_case_5.o uthread.o

test_case_1.o: test_case_1.c uthread.h
	gcc -c test_case_1.c uthread.h

test_case_2.o: test_case_2.c uthread.h
	gcc -c test_case_2.c uthread.h

test_case_3.o: test_case_3.c uthread.h
	gcc -c test_case_3.c uthread.h

test_case_4.o: test_case_4.c uthread.h
	gcc -c test_case_4.c uthread.h

test_case_5.o: test_case_5.c uthread.h
	gcc -c test_case_5.c uthread.h

uthread.o: uthread.c uthread.h
	gcc -c uthread.c uthread.h -Wall
