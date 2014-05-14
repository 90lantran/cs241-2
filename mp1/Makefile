#
# Machine Problem #1
# CS 241
#

CC = gcc
INC = -I.
FLAGS = -Wall -Wextra -Werror -Wno-unused -Wno-sign-compare -g -std=c99

all: main

doc/html: main.c libdictionary/libdictionary.c libdictionary/libdictionary.h
	doxygen doc/Doxyfile

main: main.o libdictionary/libdictionary.o
	$(CC) $^ -o $@

main.o: main.c libdictionary/libdictionary.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@

libdictionary/libdictionary.o: libdictionary/libdictionary.c libdictionary/libdictionary.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@

.PHONY : clean
clean:
	-rm -f *.o libdictionary/*.o main
