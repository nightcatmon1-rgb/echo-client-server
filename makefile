CFLAGS += -Wall
LDLIBS += -pthread

all: echo-server echo-client

echo-server: echo-server.c
	gcc $(CFLAGS) -o echo-server echo-server.c $(LDLIBS)

echo-client: echo-client.c
	gcc $(CFLAGS) -o echo-client echo-client.c $(LDLIBS)

clean:
	rm -f echo-server echo-client *.o
