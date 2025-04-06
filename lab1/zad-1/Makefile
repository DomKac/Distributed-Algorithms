CC=clang
HEADERS=-Iinc
CFLAGS=-Weverything -Wpedantic -Wno-declaration-after-statement $(HEADERS)

SERVERFILES=src/serverlib.c
CLIENTFILES=src/clientlib.c

SERVEROBJFILES=$(SERVERFILES:.c=.o)
CLIENTOBJFILES=$(CLIENTFILES:.c=.o)

all: server client

server: $(SERVEROBJFILES)
	$(CC) $(SERVEROBJFILES) server.c -o server.out

client: $(CLIENTOBJFILES)
	$(CC) $(CLIENTOBJFILES) client.c -o client.out

src/%.o: src/%.c inc/%.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(CLIENTOBJFILES) $(SERVEROBJFILES) *.out
