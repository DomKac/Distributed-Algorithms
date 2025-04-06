#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    
    FILE *file = fopen("files/cat.txt", "r");
    if (file == NULL) {
        printf("Error in opening file\n");
        return -1;
    }
    printf("*file: %p\n", (void *)file);
    printf("fileno(file): %d\n", fileno(file));
    fclose(file);

    //	Open the cat.txt file in read only mode
    int fd = open("files/cat.txt", O_RDONLY);
    if (fd == -1)
        return (1);
    printf("File descriptor: %d\n", fd);
    close(fd);

    return (0);
}
