#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAT_OPTION_LENGTH 128
#define MAT_CACHE_BUFFER_LENGTH 256

enum {
        STAGE_UNINITIALIZED = 0,
        STAGE_INITIALIZED,
        STAGE_RUNNING,
        STAGE_END
};

int g_status = 0;

char *g_usage = "Usage:\nAfter entering the HelloWorld program, pass in the actions you want to do:\n--help: Show usage again\n--init: initializes the application\n--inquire: Queries the status of the application\n--start: starts the application program\n--end: Ends the application";


int get_user_input(char *cmd, int len)
{
    int c;
    char input_buf[MAT_CACHE_BUFFER_LENGTH] = {0};
    printf("Please input:\n");
    while((c = getchar()) != EOF) {
        if(c == '\n'){
            snprintf(cmd, len, "%s", input_buf);
            return 0;
        }

        snprintf(input_buf, MAT_CACHE_BUFFER_LENGTH, "%s%c", input_buf, c);
    }

    return 0;
}

/**
 * Usage:
After entering the HelloWorld program, pass in the actions you want to do:
--help: Show usage again
--init: initializes the application
--inquire: Queries the status of the application
--start: starts the application program
--end: Ends the application
*/
int main(int argc, char **argv) {

    printf("Usage: %s \n", g_usage);

    char cmd_line[MAT_OPTION_LENGTH] = {0};
    int ret = 0;
    while (1) {
        ret = get_user_input(cmd_line, MAT_OPTION_LENGTH);
        if (ret != 0) {
            printf("Input error!\n");
            return -1;
        }

        printf("Get input option: %s\n", cmd_line);

        if (strcmp(cmd_line, "--help") == 0) {
            printf("Enter -- and bring the action you want to take");
        } else if (strcmp(cmd_line, "--init") == 0) {
            printf("Application initialization...");
            g_status = STAGE_INITIALIZED;
        } else if (strcmp(cmd_line, "--inquire") == 0) {
            printf("Application inquire...");
        } else if (strcmp(cmd_line, "--start") == 0) {
            printf("Start...");
            g_status = STAGE_RUNNING;
        } else if (strcmp(cmd_line, "--end") == 0) {
            printf("Start...");
            g_status = STAGE_END;
        } else {
            printf("Invalid option!");
            return -1;
        }
    }

    return 0;
}