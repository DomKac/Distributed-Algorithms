#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <memory.h>

#define EXIT_CODE 53

int start_server(sa_family_t sin_family, uint16_t hostshort, const char *cp, int domain, int type, int protocol);
int close_server(void);

int handle_request(void);

