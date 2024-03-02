
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    int fd = open("test.txt", O_RDWR, 0644);
    if (fd < 0) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    char *buf = NULL;

    buf = (char *)malloc(100 * sizeof(char));
    if (buf == NULL) {
        printf("Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    int read_cnt = read(fd, buf, 100);
    if (read_cnt == -1) {
        perror("Error reading file");
        return EXIT_FAILURE;
    }

    printf("Read %d bytes: %s\n", read_cnt, buf);
    free(buf);

    while (1)
    {
        sleep(5);
    }
    return 0;
}


