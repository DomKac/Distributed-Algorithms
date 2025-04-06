#include "../inc/serverlib.h"
#include "../inc/client_serverlib.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

/* Adres serwera */
static struct sockaddr_in serverAddr;
static int udpSocket;
static struct sockaddr_storage serverStorage;
static socklen_t addr_size;

static size_t clients_num = 0;
static uint64_t* clients = NULL;

static size_t files_num = 0;
static FILE** files = NULL;

static char msg[MAX_MSG_LEN]; /* buffor, w którym przechowujemy wysyłaną wiadomość */
static char buf[MAX_MSG_LEN]; /* buffor, w którym przechowujemy otrzymaną wiadomość */

// Handler for SIGINT, caused by
// Ctrl-C at keyboard
static void handle_sigint(int sig) {
    close_server();
    puts("Sever closed");
    exit(0);
}

static void set_serverAddr(sa_family_t sin_family, uint16_t hostshort, const char *cp) {
    serverAddr.sin_family = sin_family;
    serverAddr.sin_port = htons(hostshort);
    serverAddr.sin_addr.s_addr = inet_addr(cp);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
}

int start_server(sa_family_t sin_family, uint16_t hostshort, const char *cp, int domain, int type, int protocol) {
    signal(SIGINT, handle_sigint);
    udpSocket = socket(domain, type, protocol);
    set_serverAddr(sin_family, hostshort, cp);
    bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    addr_size = sizeof serverStorage;

    return 0;
}

int close_server(void) {
    // close(udpSocket);
    puts("\nKicking clients ...");
    for (size_t i = 0; i < clients_num; i++) {
        printf("Client %lx kicked from server\n", clients[i]);
    }
    free(clients);
    puts("Closing files ...");
    for (size_t i = 0; i < files_num; i++) {
        if (files[i] != NULL) {
            fclose(files[i]); /* stdio.h */
            printf("file[%zu]: %p closed\n", i, (void *)files[i]);
        }
    }
    free(files);
    
    return 0;
}

static int is_file_in_files(FILE* file) {
    for (size_t i = 0; i < files_num; i++) {
        if (files[i] == file) {
            return 1;
        }
    }
    return 0;
}

static size_t get_client_index(uint64_t auth) {
    for (size_t i = 0; i < clients_num; i++) {
        if (clients[i] == auth) {
            return i;
        }
    }
    return SIZE_MAX;
}


static int handle_join(uint64_t auth, uint64_t seq_num) {
    printf("Received join request\n");
    int req_succeeded = 1;
    clients_num++;
    clients = realloc(clients, clients_num * sizeof(int64_t));
    if (clients == NULL) {
        printf("Error in reallocating memory for clients\n");
        req_succeeded = 0;
    }
    else {
        clients[clients_num - 1] = auth;
        printf("Client %lx joined the server\n", auth);
    }
    
    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending join request confirm to client\n");
        return -1;
    }

    return 0;
}

static int handle_open(uint64_t auth, uint64_t seq_num) {

    printf("Received open request\n");
    char pathname[256];
    char mode[4];
    int req_succeeded = 1;

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\npathname: %s\nmode: %s\n", pathname, mode);
    printf("(%lx) Client %lx requested to open file %s with mode %s\n", seq_num, auth, pathname, mode);

    FILE *file = fopen(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file %s\n", pathname);
        req_succeeded = 0;
    }

    files_num++;
    files = realloc(files, files_num * sizeof(FILE *));
    files[files_num - 1] = file;
    printf("file: %p\n", (void *)file);

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nfile: %p\n", auth, seq_num, req_succeeded, (void *)file);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending OPEN REQUEST confirm to client\n");
        return -1;
    }

    return 0;
}

static int handle_close(uint64_t auth, uint64_t seq_num) {
    
        printf("Received close request\n");
        FILE *file = NULL;
        int req_succeeded = 1;
    
        sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\nfile: %p\n", (void **)&file);
        printf("(%lx) Client %lx requested to close file %p\n", seq_num, auth, (void *)file);
    
        if (file == NULL) {
            printf("Null file pointer\n");
            req_succeeded = 0;
        }
        if (!is_file_in_files(file)) {
            printf("File is not opened\n");
            req_succeeded = 0;
        }
    
        int ret = fclose(file);
        if (ret < 0) {
            printf("Error in closing file\n");
            req_succeeded = 0;
        }
    
        
        for (size_t i = 0; i < files_num; i++) {
            if (files[i] == file) {
                files[i] = NULL;
                break;
            }
        }
    
        memset(msg, '\0', MAX_MSG_LEN);
        snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
        printf("Sended msg:\n[%s]\n", msg);
    
        if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
            printf("Error in sending CLOSE REQUEST confirm to client\n");
            return -1;
        }
    
        return 0;
}

static int handle_read(uint64_t auth, uint64_t seq_num) {

    printf("Received read request\n");
    int req_succeeded = 1;
    FILE *file = NULL;
    size_t count = 0;

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\nfile: %p\ncount: %zu\n", (void **)&file, &count);
    printf("(%lx) Client %lx requested to read %zu bytes from file %p\n", seq_num, auth, count, (void *)file);

    if (file == NULL) {
        printf("Null file pointer\n");
        req_succeeded = 0;
    }
    if (!is_file_in_files(file)) {
        printf("File is not opened\n");
        req_succeeded = 0;
    }
    if (count == 0) {
        printf("Count is 0\n");
        req_succeeded = 0;
    }

    /* obtain file size: */
    // size_t fsize = 0;
    // fseek(file, 0, SEEK_END);
    // fsize = ftell(file);
    // rewind(file);

    char *buffer = malloc(count + 1);
    if (buffer == NULL) {
        printf("Error in allocating memory for buffer\n");
        req_succeeded = 0;
    }
    memset(buffer, '\0', count + 1);

    size_t bytes_read = fread(buffer, 1, count, file);
    if (ferror(file)) {
        printf("Error in reading from file\n");
        req_succeeded = 0;
    }

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nbytes_read: %zu\nbuffer: %s\n", auth, seq_num, req_succeeded, bytes_read, buffer);
    printf("Sended msg:\n[%s]\n", msg);

    free(buffer);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending READ REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_write(uint64_t auth, uint64_t seq_num) {
    
    printf("Received write request\n");
    int req_succeeded = 1;
    FILE *file = NULL;
    size_t count = 0;

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\nfile: %p\ncount: %zu\n", (void **)&file, &count);
    printf("(%lx) Client %lx requested to write %zu bytes to file %p\n", seq_num, auth, count, (void *)file);

    if (!is_file_in_files(file)) {
        printf("File is not opened\n");
        req_succeeded = 0;
    }

    char *buffer = calloc(count + 1, sizeof(char));
    if (buffer == NULL) {
        printf("Error in allocating memory for buffer\n");
        req_succeeded = 0;
    }

    char *buffer_start = strstr(buf, "buffer: ") + strlen("buffer: ");
    strncpy(buffer, buffer_start, count);
    printf("Buffer:\n[%s]\n", buffer);

    size_t bytes_wrote = fwrite(buffer, sizeof(char), count, file);
    if (ferror(file)) {
        printf("Error in writing to file\n");
        req_succeeded = 0;
    }
    fflush(file);
    free(buffer);

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\nbytes_wrote: %zu\n", auth, seq_num, req_succeeded, bytes_wrote);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending READ REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_lseek(uint64_t auth, uint64_t seq_num) {
    
    printf("Received lseek request\n");
    int req_succeeded = 1;
    FILE *file = NULL;
    off_t offset = 0;
    int whence = 0;

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\nfile: %p\noffset: %ld\nwhence: %d\n", (void **)&file, &offset, &whence);
    printf("(%lx) Client %lx requested to lseek file %p with offset %ld and whence %d\n", seq_num, auth, (void *)file, offset, whence);

    if (!is_file_in_files(file)) {
        printf("File is not opened\n");
        req_succeeded = 0;
    }

    int fd = fileno(file);
    off_t offset_ret = lseek(fd, offset, whence);
    if (ferror(file)) {
        printf("Error in fseek file\n");
        req_succeeded = 0;
    }

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\noffset_ret: %ld\n", auth, seq_num, req_succeeded, offset_ret);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending LSEEK REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_chmod(uint64_t auth, uint64_t seq_num) {
    
    printf("Received chmod request\n");
    int req_succeeded = 1;
    char pathname[256];
    mode_t mode = 0;

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\npathname: %s\nmode: %u\n", pathname, &mode);
    printf("(%lx) Client %lx requested to chmod file %s with mode %u\n", seq_num, auth, pathname, mode);

    int ret = chmod(pathname, mode);
    if (ret < 0) {
        printf("Error in chmod file %s\n", pathname);
        req_succeeded = 0;
    }

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending CHMOD REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_unlink(uint64_t auth, uint64_t seq_num) {
        
    printf("Received unlink request\n");
    int req_succeeded = 1;
    char pathname[256];

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\npathname: %[^\n]\n", pathname);
    printf("(%lx) Client %lx requested to unlink file %s\n", seq_num, auth, pathname);

    int ret = unlink(pathname);
    if (ret < 0) {
        printf("Error in unlinking file %s\n", pathname);
        req_succeeded = 0;
    }

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending UNLINK REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_rename(uint64_t auth, uint64_t seq_num) {
        
    printf("Received rename request\n");
    int req_succeeded = 1;
    char oldpath[256];
    char newpath[256];

    sscanf(buf, "auth: %*lx\nseqnum: %*lx\nrequest: %*d\noldpath: %[^\n]\nnewpath: %[^\n]\n", oldpath, newpath);
    printf("(%lx) Client %lx requested to rename file %s to %s\n", seq_num, auth, oldpath, newpath);

    int ret = rename(oldpath, newpath);
    if (ret < 0) {
        printf("Error in renaming file %s to %s\n", oldpath, newpath);
        req_succeeded = 0;
    }

    memset(msg, '\0', MAX_MSG_LEN);
    snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
    printf("Sended msg:\n[%s]\n", msg);

    if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        printf("Error in sending RENAME REQUEST to client\n");
        return -1;
    }

    return 0;
}

static int handle_test_timeout(uint64_t auth, uint64_t seq_num) {
        
        printf("Received test timeout request\n");

        /* I'm sorry i won't answer you stupid client */

        // memset(msg, '\0', MAX_MSG_LEN);
        // snprintf(msg, MAX_MSG_LEN, "auth: %lx\nseqnum: %lx\nreqsucceeded: %d\n", auth, seq_num, req_succeeded);
        // printf("Sended msg:\n[%s]\n", msg);

        // if (sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size) < 0) {
        //     printf("Error in sending TEST TIMEOUT REQUEST to client\n");
        //     return -1;
        // }
    
        return 0;
}

/* ---------------------- MAIN FUNCTION - HANDLE REQUEST ---------------------- */

int handle_request(void) {
    ssize_t nBytes = -1;
    uint64_t auth = 0;
    uint64_t seq_num = 0;
    int req_type = -1;

    memset(buf, '\0', MAX_MSG_LEN);
    nBytes = recvfrom(udpSocket, buf, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, &addr_size);
    printf("Received message:\n[%s]\n", buf);
    
    if (nBytes < 0) {
        return -1;
    }
    if (buf[0] == 'X') {
        return EXIT_CODE;
    }
    
    /* Get _auth, _seq_num and _command from message */
    sscanf(buf, "auth: %lx\nseqnum: %lx\nrequest: %d\n", &auth, &seq_num, &req_type);
    
    if (req_type == join_req) {
        if (handle_join(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == open_req) {
        if (handle_open(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == close_req) {
        if (handle_close(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == read_req) {
        if (handle_read(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == write_req) {
        if (handle_write(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == lseek_req) {
        if (handle_lseek(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == chmod_req) {
        if (handle_chmod(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == unlink_req) {
        if (handle_unlink(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == rename_req) {
        if (handle_rename(auth, seq_num) != 0) {
            return -1;
        }
    }
    else if (req_type == test_timeout_req) {
        if (handle_test_timeout(auth, seq_num) != 0) {
            return -1;
        }
    }
    else {
        printf("Unknown request type\n");
        return -1;
    }
    // else {
    //     memset(msg, '\0', MAX_MSG_LEN);
    //     snprintf(msg, MAX_MSG_LEN, "Server received message:\n%s", buf);
    //     printf("Received from client:\n  %s\n", buf);
    //     sendto(udpSocket, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&serverStorage, addr_size);
    // }
    
    return 0;
}
