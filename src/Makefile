
CC=gcc
CFLAGS=--std=c99 -Wall -pedantic -Wmissing-prototypes -I../include -g

VTE_LIBS=$(shell pkg-config --libs vte)
VTE_CFLAGS=$(shell pkg-config --cflags vte)

INCLUDES=../include/cterm.h

SRC_FILES=main.c actions.c events.c routines.c config.c
OBJECT_FILES=$(SRC_FILES:.c=.o)

../$(BIN_NAME): $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(VTE_LIBS) $(OBJECT_FILES) -o ../$(BIN_NAME)

%.o: %.c $(INCLUDES) 
	$(CC) $(CFLAGS) $(VTE_CFLAGS) -c $< -o $@
