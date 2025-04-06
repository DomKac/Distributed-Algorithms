#include <arpa/inet.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdio.h>

ssize_t join_server(sa_family_t sin_family, uint16_t hostshort, const char *cp, int domain, int type, int protocol);

uint64_t get_id(void);

FILE* rpc_open(const char *pathname, char *mode);   /* done */
int rpc_close(FILE *file);

ssize_t rpc_read(FILE *file, void *buf, size_t count); /* done */
ssize_t rpc_write(FILE *file, void *buf, size_t count); /* done */
off_t rpc_lseek(FILE *file, off_t offset, int whence);  /* done */

int rpc_chmod(const char *pathname, mode_t mode); /* done */
int rpc_unlink(const char *pathname); /* done */
int rpc_rename(const char *oldpath, const char *newpath); /* done */

ssize_t send_msg(const char *msg, size_t msg_len);
ssize_t receive_msg(char *buf, size_t buf_len);

int rpc_test_timeout(void);
