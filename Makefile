CC      = gcc
CFLAGS  = -std=gnu11 -Wall -Wextra -g
TARGET  = ocsh
SRCS    = main.c lexer.c parser.c exec.c builtins.c history.c jobs.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c ocsh.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
