
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#define MAX_BUF_SIZE 2048


int main(int argc, char const *argv[])
{
    
    int fd = open("myfile.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open error");
        return EXIT_FAILURE;
    }

    char *buf = NULL;
    buf = malloc(1024);
    if (buf == NULL) {
        printf("malloc error\n");
        return EXIT_FAILURE;
    }

    snprintf(buf, 1024, "hello world\n");
    int write_size = write(fd, buf, 1024);
    if (write_size < 0) {
        printf("write error\n");
        free(buf);
        return EXIT_FAILURE;
    }

    // reopen file
    sync();
    close(fd);
    fd = -1;
    fd = open("myfile.txt", O_RDONLY);
    if (fd < 0)
    {
        perror("open error");
        free(buf);
        return EXIT_FAILURE;
    }

    char *read_buf = NULL;
    read_buf = malloc(MAX_BUF_SIZE);
    if (read_buf == NULL) {
        printf("malloc error\n");
        free(buf);
        return EXIT_FAILURE;
    }

    //  read data
    int read_size = read(fd, read_buf, MAX_BUF_SIZE);
    if (read_size < 0) {
        printf("read error\n");
        free(buf);
        free(read_buf);
        return EXIT_FAILURE;
    }
    printf("read data: %s\n", read_buf);

    // check
    if (strcmp(buf, read_buf) != 0) {
        printf("data not equal\n");
        free(buf);
        free(read_buf);
        return EXIT_FAILURE;
    }
    close(fd);
    fd = -1;
    free(buf);
    free(read_buf);
    return 0;
}
