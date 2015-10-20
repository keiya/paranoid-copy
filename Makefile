
CFLAGS ?= -O3
CFLAGS += -Wall
FLAGS  := $(CFLAGS) $(LDFLAGS)

default: all

all: cp

cp: cp.c crc32c.c
	$(CC) $(FLAGS) cp.c -lpthread -o $@
