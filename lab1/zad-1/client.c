// Client side code
// https://www.geeksforgeeks.org/message-encryption-decryption-using-udp-server/?ref=oin_asr1

#include "inc/client_serverlib.h"
#include "inc/clientlib.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static void output_permissions(mode_t m) {
    putchar(m & S_IRUSR ? 'r' : '-');
    putchar(m & S_IWUSR ? 'w' : '-');
    putchar(m & S_IXUSR ? 'x' : '-');
    putchar(m & S_IRGRP ? 'r' : '-');
    putchar(m & S_IWGRP ? 'w' : '-');
    putchar(m & S_IXGRP ? 'x' : '-');
    putchar(m & S_IROTH ? 'r' : '-');
    putchar(m & S_IWOTH ? 'w' : '-');
    putchar(m & S_IXOTH ? 'x' : '-');
    putchar('\n');
}

// Assumes little endian
static void printBits(size_t const size, void const *const ptr) {
    unsigned char *b = (unsigned char *)ptr;
    unsigned char byte;
    int i, j;

    for (i = size - 1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

static int test_open(const char *pathname, char *mode) {
    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }
    return 0;
}

static int test_read_txt() {

    char buffer[MAX_MSG_LEN];
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/read_test.txt";
    char *mode = "r";

    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }

    ssize_t ret = rpc_read(file, buffer, MAX_MSG_LEN);
    if (ret < 0) {
        printf("Error in reading from file\n");
        return -1;
    }

    printf("Readed bytes from file: %ld\n", ret);
    printf("Text from file:\n[%s]\n", buffer);

    return 0;
}

static int test_read_bin() {

    void* buffer = malloc(MAX_MSG_LEN);
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/read_test.bin";
    char *mode = "rb";

    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }

    ssize_t ret = rpc_read(file, buffer, MAX_MSG_LEN);
    if (ret < 0) {
        printf("Error in reading from file\n");
        return -1;
    }

    printf("Readed bytes from file: %ld\n", ret);
    for (size_t i = 0; i < ret; i++) {
        printf("%x ", ((char*)buffer)[i]);
    }

    return 0;
}

static int test_write_txt() {
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/write_test_wtf.txt";
    char *mode = "w";
    char *msg = "Hello, this is a test message!\nYou can rpc_write anything you want here!\n";

    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }

    ssize_t ret = rpc_write(file, msg, strlen(msg));
    if (ret < 0) {
        printf("Error in writing to file\n");
        return -1;
    }

    return 0;
}

static int test_lseek() {
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/read_test.txt";
    char *mode = "r";

    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }
    /* should return XYZ */ 
    off_t ret = rpc_lseek(file, 24, SEEK_SET);
    if (ret < 0) {
        printf("Error in rpc_lseek\n");
        return -1;
    }
    printf("Lseeked to: %ld\n", ret);

    char buffer[MAX_MSG_LEN];
    ssize_t read_ret = rpc_read(file, buffer, 3);
    if (ret < 0) {
        printf("Error in reading from file\n");
        return -1;
    }

    printf("Readed bytes from file: %ld\n", read_ret);
    printf("Text readed from file after rpc_lseek:\n[%s]\n", buffer);

    return 0;
}

static int test_chmod() {
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/chmod_test.txt";

    FILE *file = rpc_open(pathname, "w");
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }
    
    int close_ret = rpc_close(file);
    if (close_ret < 0) {
        printf("Error in closing file\n");
        return -1;
    }

    int chmod_ret = rpc_chmod(pathname, S_IRUSR); /* Zmianna uprawnień na READ ONLY dla usera */
    if (chmod_ret < 0) {
        printf("Error in changing file permissions\n");
        return -1;
    }

    file = rpc_open(pathname, "w");
    if (file == NULL) {
        puts("Chmod works!");
        printf("Error in opening file\n");
        return 0;
    }

    return -1;
}

static int test_unlink() {
    char *pathname = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/unlink_test.txt";
    char *mode = "w";

    FILE *file = rpc_open(pathname, mode);
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }

    printf("Sleeping for 3 seconds: ");
    for (size_t i = 0; i < 3; i++) {
        printf("%zu..", i + 1);
        sleep(1);
    }
    puts("");

    int ret = rpc_unlink(pathname);
    if (ret < 0) {
        printf("Error in unlinking file\n");
        return -1;
    }

    return 0;
}

static int test_rename() {
    char *oldpath = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/rename_test_old.txt";
    char *newpath = "/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/rename_test_new.txt";
    
    int unlink_ret = rpc_unlink(newpath);
    if (unlink_ret < 0) {
        printf("File %s doesn't exist\n", newpath);
    }
    else {
        printf("File %s unlinked\n", newpath);
    }

    FILE *file = rpc_open(oldpath, "w");
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }

    printf("Sleeping for 3 seconds: ");
    for (size_t i = 0; i < 3; i++) {
        printf("%zu..", i + 1);
        sleep(1);
    }
    puts("");

    int ret = rpc_rename(oldpath, newpath);
    if (ret < 0) {
        printf("Error in renaming file\n");
        return -1;
    }

    return 0;
}

static int test_timeout() {
    int ret = rpc_test_timeout();
    if (ret < 0) {
        printf("Server didn't answered twice :(\n");
        return -1;
    }

    return 0;
}

int main(void) {

    char message[MAX_MSG_LEN], buffer[MAX_MSG_LEN];
    
    ssize_t ret = join_server(AF_INET, 5004, "127.0.0.1", PF_INET, SOCK_DGRAM, 0);
    if (ret < 0) {
        printf("Error in joining server!\n");
        return -1;
    }

    // int open_ret = test_open("/home/dominik/Studia/Magisterka/M-2-Semestr/AR/lab1/zad-1/files/test.txt", "w");
    // int read_ret = test_read_txt();
    // if (read_ret < 0) {
    //     printf("Error in reading from file!\n");
    //     return -1;
    // }

    // read_ret = test_read_bin();
    // if (read_ret < 0) {
    //     printf("Error in reading from file!\n");
    //     return -1;
    // }

    // int write_ret = test_write_txt();
    // if (write_ret < 0) {
    //     printf("Error in writing to file!\n");
    //     return -1;
    // }
    
    // int lseek_ret = test_lseek();
    // if (lseek_ret < 0) {
    //     printf("Error in rpc_lseek!\n");
    //     return -1;
    // }

    // int unlink_ret = test_unlink();
    // if (unlink_ret < 0) {
    //     printf("Error in unlinking file!\n");
    //     return -1;
    // }

    // int rename_ret = test_rename();
    // if (rename_ret < 0) {
    //     printf("Error in renaming file!\n");
    //     return -1;
    // }

    // int chmod_ret = test_chmod();
    // if (chmod_ret < 0) {
    //     printf("Error in changing file permissions!\n");
    //     return -1;
    // }

    int timeout_ret = test_timeout();
    if (timeout_ret < 0) {
        return -1;
    }

    return 0;
}

// while (1) {
//     printf("=>: ");
//     fgets(message, MAX_MSG_LEN, stdin);

//     send_msg(message, MAX_MSG_LEN);

//     if (message[0] == 'X') {
//         printf("I'm quitting!\n");
//         break;
//     }

//     receive_msg(buffer, MAX_MSG_LEN);

//     printf("Received from server:\n%s", buffer);

//     printf("\n");
// }
