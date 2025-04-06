#include "../inc/clientlib.h"
#include "../inc/client_serverlib.h"
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>

#define ERR_AUTH_FAILED -10
#define ERR_REQ_FAILED -11
#define ERR_SEQ_NUM_DIFF -12

#define TIMEOUT_SEC 3
#define MAX_RETRIES 2

static struct sockaddr_in serverAddrClient;
static socklen_t addr_size;
static int clientSocket;
static uint64_t client_id = 0;

static char msg[MAX_MSG_LEN];   /* buffor, w którym przechowujemy zapisaną wiadomość */
static char buf[MAX_MSG_LEN];   /* buffor, w którym przechowujemy otrzymaną wiadomość */


static uint64_t gen_rand_seq_num(void) {
    uint64_t r1 = (uint64_t)rand();
    uint64_t r2 = (uint64_t)rand();
    uint64_t ret = r1 << 32 | r2;
    return ret;
}

static void set_serverAddr(sa_family_t sin_family, uint16_t hostshort, const char *cp) {
    serverAddrClient.sin_family = sin_family;
    serverAddrClient.sin_port = htons(hostshort);
    serverAddrClient.sin_addr.s_addr = inet_addr(cp);
    memset(serverAddrClient.sin_zero, '\0', sizeof serverAddrClient.sin_zero);

    addr_size = sizeof(serverAddrClient);
}

static int set_socket(int domain, int type, int protocol) {
    clientSocket = socket(domain, type, protocol);
    if (clientSocket < 0) {
        perror("Error in creating socket\n");
        return -1;
    }

    // Ustawienie limitu czasu na odbiór danych (3 sekundy)
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    if (setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error in setting timeout\n");
        // close(clientSocket);
        return -1;
    }

    return 0;
}

ssize_t join_server(sa_family_t sin_family, uint16_t hostshort, const char *cp, int domain, int type, int protocol) {
    
    set_serverAddr(sin_family, hostshort, cp);
    int socket_ret = set_socket(domain, type, protocol);
    if (socket_ret < 0) {
        return -1;
    }

    /* Generate client_id == authentication code */
    srand(time(NULL));
    do {
        client_id = (uint64_t)rand();
    } while (client_id == 0);

    /* Sending message */
    const uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\n", client_id, seq_num_sended, join_req);
    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending JOINING request\n");
        return -1;
    }

    /* Receiving message */
    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);

    /* Check authentication code correct */
    if (auth != client_id) {
        printf("Authentication codes difference: %lx and %lx\n", auth, client_id);
        return ERR_AUTH_FAILED;
    }
    /* Check sequence number correct */
    if (seq_num_sended != seq_num_received) {
        printf("Sended and received sequence numbers difference: %lx and %lx\n", seq_num_sended, seq_num_received);
        return ERR_SEQ_NUM_DIFF;
    }
    if (req_succeeded == false) {
        puts("Request Failed");
        return ERR_REQ_FAILED;
    }

    return 0;
}
    
uint64_t get_id(void) {
    return client_id;
}

ssize_t send_msg(const char* message, size_t msg_len) {
    if (msg_len < MAX_MSG_LEN) {
        return sendto(clientSocket, message, msg_len, 0, (struct sockaddr *)&serverAddrClient, addr_size);
    }
    return sendto(clientSocket, message, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size);
}

ssize_t receive_msg(char *buffer, size_t buf_len) {
    memset(buffer, '\0', buf_len);
    return recvfrom(clientSocket, buffer, buf_len, 0, NULL, NULL);
}


FILE* rpc_open(const char *pathname, char *mode) {

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\npathname: %s\nmode: %s\n", client_id, seq_num_sended, open_req, pathname, mode);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return NULL;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return NULL;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    FILE* file = NULL;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nfile: %p\n", &auth, &seq_num_received, &req_succeeded, (void**)&file);

    if (auth != client_id) {
        return NULL;
    }
    if (seq_num_received != seq_num_sended) {
        return NULL;
    }
    if (req_succeeded == false) {
        return NULL;
    }    

    return file;
}

int rpc_close(FILE *file) {
    
        /* Sending message */
        uint64_t seq_num_sended = gen_rand_seq_num();
        memset(msg, '\0', MAX_MSG_LEN);
        snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\nfile: %p\n", client_id, seq_num_sended, close_req, (void *)file);
    
        if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
            printf("Error in sending the message\n");
            return -1;
        }
    
        /* Receiving message */
        memset(buf, '\0', MAX_MSG_LEN);
        if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
            printf("Error in receiving the message\n");
            return -1;
        }
    
        uint64_t auth = 0;
        uint64_t seq_num_received = 0;
        int req_succeeded = 0;
        sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);
    
        if (auth != client_id) {
            return -1;
        }
        if (seq_num_received != seq_num_sended) {
            return -1;
        }
        if (req_succeeded == false) {
            return -1;
        }
    
        return 0;
}

ssize_t rpc_read(FILE *file, void *buffer, size_t count) {

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\nfile: %p\ncount: %zu\n", client_id, seq_num_sended, read_req, (void *)file, count);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    memset(buffer, '\0', count);
    size_t bytes_read = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nbytes_read: %zu\nbuffer: %[^\n]\n", &auth, &seq_num_received, &req_succeeded, &bytes_read, (char *)buffer);

    /* sscanf ma problem z parsowanie wielolinijkowych stringów, dlatego robimy to samemu */
    char *buffer_start = strstr(buf, "buffer: ") + strlen("buffer: ");
    strncpy(buffer, buffer_start, count);

    // printf("Received buf: %s\n", buf);
    // printf("Buffer: \"%s\"\n", (char*)buffer);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return (ssize_t)bytes_read;
}

ssize_t rpc_write(FILE *file, void *buffer, size_t count) {
    
    /* Sending message */
    if (file == NULL) {
        printf("Null file pointer\n");
        return -1;
    }
    if (count == 0) {
        printf("Count is 0\n");
        return -1;
    }
    if (buffer == NULL) {
        printf("Null buffer pointer\n");
        return -1;
    }

    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\nfile: %p\ncount: %zu\nbuffer: %s\n", client_id, seq_num_sended, write_req, (void *)file, count, (char*)buffer);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    size_t bytes_wrote = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nbytes_wrote: %zu\n", &auth, &seq_num_received, &req_succeeded, &bytes_wrote);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return (ssize_t)bytes_wrote;
}

off_t rpc_lseek(FILE *file, off_t offset, int whence) {
    
    if (file == NULL) {
        printf("Null file pointer\n");
        return -1;
    }
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
        printf("Invalid whence value\n");
        return -1;
    }

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\nfile: %p\noffset: %ld\nwhence: %d\n", client_id, seq_num_sended, lseek_req, (void *)file, offset, whence);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    off_t offset_ret = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\noffset_ret: %ld\n", &auth, &seq_num_received, &req_succeeded, &offset_ret);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return offset_ret;
}

int rpc_chmod(const char *pathname, mode_t mode) {

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\npathname: %s\nmode: %u\n", client_id, seq_num_sended, chmod_req, pathname, mode);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return 0;
}

/* Deleting file */
int rpc_unlink(const char *pathname) {

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\npathname: %s\n", client_id, seq_num_sended, unlink_req, pathname);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return 0;
}

int rpc_rename(const char *oldpath, const char *newpath) {

    /* Sending message */
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\noldpath: %s\nnewpath: %s\n", client_id, seq_num_sended, rename_req, oldpath, newpath);

    if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
        printf("Error in sending the message\n");
        return -1;
    }

    /* Receiving message */
    memset(buf, '\0', MAX_MSG_LEN);
    if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) < 0) {
        printf("Error in receiving the message\n");
        return -1;
    }

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return 0;
}

int rpc_test_timeout(void) {
    
    uint64_t seq_num_sended = gen_rand_seq_num();
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nrequest: %d\n", client_id, seq_num_sended, test_timeout_req);

    for (size_t retries = 0; retries < MAX_RETRIES; retries++) {
        /* Sending message */
        printf("Try number: %zu\n", retries+1);
        if (sendto(clientSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverAddrClient, addr_size) < 0) {
            printf("Error in sending the message\n");
            return -1;
        }

        /* Receiving message */
        memset(buf, '\0', MAX_MSG_LEN);
        if (recvfrom(clientSocket, buf, MAX_MSG_LEN, 0, NULL, NULL) >= 0) {
            printf("Received message: %s\n", buf);
            break;
        }
        
        // Jeśli nie otrzymano odpowiedzi i wystąpił błąd czasu
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Timeout (brak odpowiedzi w czasie %d sekund)\n", TIMEOUT_SEC);
            fflush(stdout);
        }
        if (retries == MAX_RETRIES - 1) {
            printf("Max retries reached. Request Failed!\n");
            return -1;
        }
    }
    

    uint64_t auth = 0;
    uint64_t seq_num_received = 0;
    int req_succeeded = 0;
    sscanf(buf, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", &auth, &seq_num_received, &req_succeeded);

    if (auth != client_id) {
        return -1;
    }
    if (seq_num_received != seq_num_sended) {
        return -1;
    }
    if (req_succeeded == false) {
        return -1;
    }

    return 0;
}
