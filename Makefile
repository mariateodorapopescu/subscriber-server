CC = g++
CFALGS = -Wall -Wextra -Werror -pedantic -std=c++17
OBJ_SERVER = server.o
OBJ_SUBSCRIBER = subscriber.o

.PHONY: all clean

all: server subscriber

server: $(OBJ_SERVER)
	$(CC) $(CFLAGS) $^ -o $@

subscriber: $(OBJ_SUBSCRIBER)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_SERVER) $(OBJ_SUBSCRIBER) server subscriber
