# Project: pra

.PHONY: all clean

all: 
	g++ -c lemon-1.3.1/lemon/base.cc -o lemon-1.3.1/lemon/base.o -I lemon-1.3.1
	g++ multi_thread_fast.cpp lemon-1.3.1/lemon/base.o -o multi_thread_fast -I lemon-1.3.1 -pthread 
	g++ single_thread_stats.cpp lemon-1.3.1/lemon/base.o -o single_thread_stats -I lemon-1.3.1

clean: 
	rm -f single_thread_stats multi_thread_fast lemon-1.3.1/lemon/base.o 



