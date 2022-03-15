CC = gcc
CFLAGS = -g -I -std=gnu99 -lpthread -lm -lrt
all: master slave

master: config.h master.c
	$(CC) -o $@ $^ $(CFLAGS)

slave: config.h slave.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm master slave cstest logfile.*