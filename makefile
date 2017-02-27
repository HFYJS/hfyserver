hfyhttpd: main.o hfyhttpd.o thread_pool.o
	gcc -o hfyhttpd main.o hfyhttpd.o thread_pool.o
thread_pool.o: thread_pool.h thread_pool.c
	gcc -c thread_pool.c
hfyhttpd.o: hfyhttpd.h hfyhttpd.c
	gcc -c hfyhttpd.c
main.o: main.c
	gcc -c main.c

clean:
	rm *.o