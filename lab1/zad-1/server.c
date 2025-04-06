// C server code
#include "inc/client_serverlib.h"
#include "inc/serverlib.h"
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// Driver code
int main() {

    start_server(AF_INET, 5004, "127.0.0.1", PF_INET, SOCK_DGRAM, 0);
    puts("Waiting for client :");

    while (1) {
        int ret = handle_request();
        if (ret == EXIT_CODE) {
            puts("SERVER CLOSED");
            break;
        }
        else if (ret == -1) {
            printf("Request failed\n");
            break;
        }
    }

    close_server();

    return 0;
}
