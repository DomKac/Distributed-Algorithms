#include <arpa/inet.h>
#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#define MAX_MSG_LEN 1024

#define REQUEST_SUCCESS 1
#define REQUEST_FAILURE 0

enum request_type { join_req,
                    open_req,
                    close_req,
                    read_req,
                    write_req,
                    lseek_req,
                    chmod_req,
                    unlink_req,
                    rename_req,
                    test_timeout_req,
                    types_num,
};
