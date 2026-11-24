CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g
TARGET  = ocsh
SRCS    = main.c parser.c exec.c builtins.c alias.c vars.c signals.c
OBJS    = $(SRCS:.c=.o)
HDRS    = $(wildcard *.h)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $<

test: $(TARGET)
	./tests/run_tests.sh

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean test
