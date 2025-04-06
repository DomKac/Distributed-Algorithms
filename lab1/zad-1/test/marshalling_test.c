#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>


static uint64_t gen_rand_seq_num(void) {
    uint64_t r1 = (uint64_t)rand();
    uint64_t r2 = (uint64_t)rand();
    printf("r1: %lu, r2: %lu\n", r1, r2);
    uint64_t seq_num = r1 << 32 | r2;
    return seq_num;
}

int main(void) {

    const char *msg = "command: open\n\
                        pathname: test.txt\n\
                        mode: r\n";

    size_t msg_len = strlen(msg);

    enum request_type {
        OPEN,
        READ,
        WRITE,
        LSEEK,
        CHMOD,
        UNLINK,
        RENAME
    };

    char command[10];
    char pathname[128];
    char mode[4];

    // sscanf(msg, "command:%s\npathname: %s\nmode: %s\n", command, pathname, mode);
    printf("command: %s, pathname: %s, mode: %s\n", command, pathname, mode);

    sscanf(msg, "command: %s\n", command);
    printf("%s\n", command);

    srand(time(NULL));
    printf("RAND_MAX: %d\n", RAND_MAX);
    printf("sizeof(rand) = %lu\n", sizeof(rand()));

    uint64_t seq_num = gen_rand_seq_num();
    printf("seq_num: %lu\n", seq_num);

    printf("NULL adress: %p\n", NULL);

    return 0;
}
