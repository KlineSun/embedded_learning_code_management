
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "process_table.h"

char *get_time_str() {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    static char time_str[128] = {0};

    sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    return time_str;
}

static char g_table_type[64] = {0};
static int g_rows = 0;
static int g_columns = 0;
static student_t *g_student_info = NULL;
static fruit_t *g_fruit_ptr = NULL;


/**
 * @brief: Read the contents of the entire file
 * 
 * @param path: file path
 * 
 * return: Return buf that save the contents of the file on sucess. The buf must gree by caller!
 *         Returns a null pointer if it fails!
 * 
*/
char *common_read_all_file_contents(const char *path)
{
    if (NULL == path) {
        LOG_DEBUG("invalid parameter!");
        return NULL;
    }

    FILE *fp = fopen(path, "r");
    if (NULL == fp) {
        LOG_DEBUG("open parameter!");
        return NULL;
    }

    // get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    LOG_DEBUG("The size of %s is: %ld", path, fileSize);

    // malloc buf
    char *buf = (char *)malloc(sizeof(char) * (fileSize + 1));
    if (NULL == buf) {
        LOG_DEBUG("malloc buffer failed!");
        fclose(fp);
        return NULL;
    }
    memset(buf, 0, (fileSize + 1));

    // get file content
    rewind(fp);
    long read_size = fread(buf, 1, fileSize, fp);
    if (read_size != fileSize) {
        LOG_DEBUG("malloc buffer failed!");
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[fileSize] = '\0';

    //LOG_DEBUG("File: %s, contents:\n %s", path, buf);
    fclose(fp);
    return buf;
}

/**
 * 
 * fd: input file
 * buf: output buf
 * max_len: maximum length
 * 
 * return: Length of content read
*/
int read_line(int fd, char *buf, int max_len)
{
    if (fd < 0 || buf == NULL || max_len <= 0 || max_len > MAX_BUF_SIZE) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    char line_buf[MAX_BUF_SIZE] = {0};
    char ch = '\0';
    int cnt = -1;
    int i = 0;
    while (1) {
        cnt = read(fd, &ch, 1);
        if (cnt <= 0) {
            break;
        }
        if (ch == '\n' || ch == '\r' || ch == '\0') {
            // 每当遇到换行、结束符等特殊字符后，文件index向后偏移一个字符
            lseek(fd, 1, SEEK_CUR);
            break;
        }
        if (i < max_len - 1) {
            line_buf[i++] = ch;
        } else {
            LOG_DEBUG("line too long!");
            break;
        }
    }

    if (i <= 0) {
        return -1;
    }

    if (i <= max_len - 1) {
        line_buf[i++] = '\0';
        strncpy(buf, line_buf, max_len);
        return i;
    }
    return -1;
}

int parse_table_type(const char* table_path)
{
    if (table_path == NULL) {
       LOG_DEBUG("invalid parameter!");
       return -1;
    }

    char *file_buf = common_read_all_file_contents(table_path);
    if (NULL == file_buf) {
        LOG_DEBUG("read %s failed!", table_path);
        return -1;
    }

    char *temp = strstr(file_buf, "table_type");
    if (NULL == temp) {
        LOG_DEBUG("No type defined in the table");
        free(file_buf);
        return -1;
    }

    char *type = strtok(temp, ",");
    type = strtok(NULL, ",");
    if (NULL == type || strlen(type) < 1) {
        LOG_DEBUG("No type defined in the table");
        free(file_buf);
        return -1;
    }

    LOG_DEBUG("get type: %s", type);
    if (0 == strcasecmp(type, "course_table")) {
        strncpy(g_table_type, "course_table", sizeof(g_table_type));
    } else if (0 == strcasecmp(type, "sale_table")) {
        strncpy(g_table_type, "sale_table", sizeof(g_table_type));
    } else {
        LOG_DEBUG("Unsupport info type!");
        free(file_buf);
        return -1;
    }

    free(file_buf);
    return 0;
}

int count_sub_str(const char* str, const char *sub_str)
{
    if (NULL == str || NULL == sub_str) {
        LOG_DEBUG("invalid parameter!");
        return 0;
    }

    int cnt = 0;
    char *buf, *p;
    buf = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    if (NULL == buf) {
        LOG_DEBUG("malloc error!");
        return 0;
    }
    strcpy(buf, str);
    buf[strlen(str)] = '\0';

    p = buf;
    while (NULL != (p = strstr(p, sub_str))){
        cnt++;
        p++;
    }

    //LOG_DEBUG("Found %d \"%s\" in  \"%s\"", cnt, sub_str, buf);
    free(buf);
    return cnt;
}

int parse_csv_line(const char *line, char ***result_list)
{
    if (NULL == line) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    int param_cnt = 0;
    char **items_list = NULL;
    param_cnt = count_sub_str(line, ",");
    if (0 < param_cnt) {
        // 为二维字符数组创建 n + 1个指针
        items_list = (char **)malloc((param_cnt + 1) * sizeof(char *));
        if (NULL == items_list) {
            LOG_DEBUG("malloc error!");
            return -1;
        }
    } else {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    //拷贝line字符串
    char *line_buf;
    line_buf = (char *)malloc(sizeof(char) * (strlen(line) + 1));
    if (NULL == line_buf) {
        LOG_DEBUG("malloc error!");
        return -1;
    }
    strcpy(line_buf, line);
    line_buf[strlen(line)] = '\0';

    //提取字符串中的item
    char temp_buf[MAX_BUF_SIZE] = {0};
    char ch = '\0';
    char *ptr = line_buf;
    int i = 0, j = 0, sub_str_size = 0;
    for (j = 0; j < strlen(line) + 1; j++, ptr++) {
        ch = ptr[0];
        //LOG_DEBUG("ch: %s", &ch);
        if (',' != ch && '\0' != ch) {
            strncat(temp_buf, &ch, 1);
            sub_str_size++;
            if (sub_str_size >= MAX_BUF_SIZE) {
                LOG_DEBUG("content is too long!");
                free(line_buf);
                return -1;
            }
            continue;
        }

        items_list[i] = (char *)malloc(sub_str_size + 1);
        if (NULL == items_list[i]) {
            LOG_DEBUG("malloc items_list[%d] error, skip to next!", i);
            sub_str_size = 0;
            continue;
        }
        
        memset(items_list[i], 0, sub_str_size + 1);
        if (sub_str_size > 0) {
            strcpy(items_list[i], temp_buf);
        } else {
            strcpy(items_list[i], "");
        }
        items_list[i][sub_str_size] = '\0';
        //LOG_DEBUG("column[%d]: %s", i, items_list[i]);
        i++;
        if ('\0' == ch) {
            break;
        }
        sub_str_size = 0;
        memset(temp_buf, 0, sizeof(temp_buf));
    }

    g_columns = i;
    *result_list = items_list;
    free(line_buf);
    return 0;
}

void free_2d_array(char **array_ptr)
{
    if (NULL == array_ptr) {
        LOG_DEBUG("array_ptr is NULL, no need to free");
        return;
    }

    int i;
    for (i = 0; i < g_columns; i++) {
        free(array_ptr[i]);
        array_ptr[i] == NULL;
    }
    free(array_ptr);
    array_ptr = NULL;
    return;
}


int get_data_type_emum(char *str)
{
    if (NULL == str) {
        LOG_DEBUG("invalid parameter!");
        return INVALID_TYPE;
    }

    if (0 == strcmp(str, "Name")) {
        return NAME;
    } else if (0 == strcmp(str, "Total")) {
        return TOTAL;
    } else if (0 == strcmp(str, "Rank")) {
        return RANK;
    } else if (0 == strcmp(str, "Mark")) {
        return MARK;
    } else if (0 == strcmp(str, "Chinese")) {
        return CHINESE;
    } else if (0 == strcmp(str, "Math")) {
        return MATH;
    } else if (0 == strcmp(str, "English")) {
        return ENGLISH;
    } else if (0 == strcmp(str, "Physics")) {
        return PHRSICS;
    } else if (0 == strcmp(str, "Chemistry")) {
        return CHEMMISTR;
    } else if (0 == strcmp(str, "Biology")) {
        return BIOLOGY;
    } else if (0 == strcmp(str, "Politics")) {
        return POLITICS;
    } else if (0 == strcmp(str, "History")) {
        return HISTORY;
    } else if (0 == strcmp(str, "Geography")) {
        return GEOGRAPHY;
    } else {
        LOG_DEBUG("unsupport type: %s", str);
        return INVALID_TYPE;
    }
    return INVALID_TYPE;
}

int parse_student_info(char **items_list, char **data_list, student_t *student_info)
{
    if (NULL == items_list || NULL == data_list || NULL == student_info) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }

    int i = 0;
    course_t *course_ptr;
    student_info->course_count = 0;
    student_info->total_score = 0;
    student_info->rank = 1;
    float score = 0.0;
    for (; i < g_columns; i++) {
        LOG_DEBUG("get student info: %s = %s", items_list[i], data_list[i]);

        switch (get_data_type_emum(items_list[i]))
        {
            case NAME:
            case TOTAL:
            case RANK:
            case MARK:
                strcpy(student_info->name, data_list[i]);
                break;
            case CHINESE:
            case MATH:
            case ENGLISH:
            case PHRSICS:
            case CHEMMISTR:
            case BIOLOGY:
            case POLITICS:
            case HISTORY:
            case GEOGRAPHY:
                // malloc next course
                course_ptr = (course_t *)malloc(sizeof(course_t));
                if (NULL == course_ptr) {
                    LOG_DEBUG("malloc course struct failed!");
                    return -1;
                }

                // set link
                if (NULL == student_info->course) {
                    student_info->course = course_ptr;
                    student_info->next = NULL;
                } else {
                    course_ptr->next = student_info->course;
                    student_info->course = course_ptr;
                }

                // set current course value to course struct
                strcpy(course_ptr->name, items_list[i]);
                score = atof(data_list[i]);
                if (data_list[i][0] != '0' && score <= 0) {
                    LOG_DEBUG("score abnormal: %s, get float value: %f", data_list[i], score);
                    score = 0.0;
                }
                course_ptr->score = score;
                student_info->total_score += course_ptr->score;
                student_info->course_count++;
                break;
            default:
                LOG_DEBUG("unsupport item: %s", items_list[i]);
                break;
        }
        course_ptr = NULL;
    }
    LOG_DEBUG("The student scored total of %f points in %d courses", student_info->total_score, student_info->course_count);

    return 0;
}

int student_info_handler(const char *input_file, const char *output_file)
{
    if (NULL == input_file || NULL == output_file) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }
    LOG_DEBUG("Enter!");

    //handle input file
    int is_success = 0;
    int raw_file_fd = -1, output_file_fd = -1;
    char line_buf[1024] = {0};
    int read_cnt = -1, ret = 0;
    char **items_list = NULL;
    char **data_list = NULL;
    raw_file_fd = open(input_file, O_RDONLY);
    if (raw_file_fd < 0) {
        LOG_DEBUG("open error");
        is_success = -1;
        goto exit_deal;
    }

    output_file_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_file_fd < 0) {
        close(raw_file_fd);
        LOG_DEBUG("open error");
        is_success = -1;
        goto exit_deal;
    }

    // 逐行解析
    while (1)
    {
        LOG_DEBUG("=================================================");
        read_cnt = read_line(raw_file_fd, line_buf, sizeof(line_buf));
        if (read_cnt <= 0){
            break;
        }

        LOG_DEBUG("line_buf is: %s", line_buf);
        // 解析item行
        if (NULL == items_list && 0 < count_sub_str(line_buf, "Name,") && 0 < count_sub_str(line_buf, "Total")) {
            ret = parse_csv_line(line_buf, &items_list);
            if (0 != ret) {
                LOG_DEBUG("Parse items failed!");
                is_success = -1;
                goto exit_deal;
            }
            continue;
        } else if (NULL != strstr(line_buf, "table_type")) {
            LOG_DEBUG("skip line of table_type");
            continue;
        }
        if (NULL == items_list) {
            LOG_DEBUG("Parse items failed!");
            is_success = -1;
            goto exit_deal;
        }

        // 解析数据行
        ret = parse_csv_line(line_buf, &data_list);
        if (NULL == data_list) {
            LOG_DEBUG("Parse data failed!");
            is_success = -1;
            goto exit_deal;
        }
        g_rows++;
        LOG_DEBUG("For %d student, there are %d pieces of information", g_rows, g_columns);

        // 初始化当前学生的结构体
        student_t *tmp = (student_t *)malloc(sizeof(student_t));
        if (NULL == tmp) {
            LOG_DEBUG("malloc student struct failed!");
            goto exit_deal;
        }
        if (NULL == g_student_info) {
            g_student_info = tmp;
        } else {
            tmp->next = g_student_info;
            g_student_info = tmp;
        }
        memset(g_student_info, 0, sizeof(student_t));

        // 写入数据到结构体中
        ret = parse_student_info(items_list, data_list, g_student_info);
        if (0 != ret) {
            LOG_DEBUG("parse student info failed!");
            goto exit_deal;
        }

        memset(line_buf, 0, sizeof(line_buf));
    }

exit_deal:
    free_2d_array(items_list);
    free_2d_array(data_list);
    if (raw_file_fd > 0) {
        close(raw_file_fd);
    }
    if (output_file_fd > 0) {
        close(output_file_fd);
    }
    while (NULL != g_student_info) {
        while (NULL != g_student_info->course) {
            LOG_DEBUG("Debug");
            course_t *temp1 = g_student_info->course;
            g_student_info->course = g_student_info->course->next;
            free(temp1);
        }
        student_t *temp2 = g_student_info;
        g_student_info = g_student_info->next;
        free(temp2);
    }
    return is_success;
}

int fruit_info_handler(const char *input_file, const char *output_file)
{
    if (NULL == input_file || NULL == output_file) {
        LOG_DEBUG("invalid parameter!");
        return -1;
    }
    LOG_DEBUG("Enter!");

    //handle input file


    return 0;
}

/**
 * usage: ./process_table <raw csv file> -[operate type] <output csv file>
 * 
 * operate type:                                                                                                                                                                                                                    
 *      -cal: Calculate total score ranking
 *      -card: Generate transcripts for each student 
*/
int main(int argc, char const *argv[])
{

    if (argc < 4) {
        LOG_DEBUG("Usage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }

    char raw_file[128] = {0};
    strncpy(raw_file, argv[1], sizeof(raw_file));
    //snprintf(raw_file, sizeof(raw_file), "%s", argv[1]);

    char operate_type[16] = {0};
    char output_file[128] = {0};
    if (argc == 4) {
        strncpy(operate_type, argv[2], sizeof(operate_type));
        strncpy(output_file, argv[3], sizeof(output_file));
        //snprintf(operate_type, sizeof(operate_type), "%s", argv[2]);
        //snprintf(output_file, sizeof(output_file), "%s", argv[3]);
    } else {
        LOG_DEBUG("Too many paramter!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
    }

    //check input parameter
    if (strcmp(operate_type, "-cal") != 0 && strcmp(operate_type, "-card") != 0) {
        LOG_DEBUG("Invalid operate type!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }
    if (strstr(raw_file, ".csv") == NULL || strstr(output_file, ".csv") == NULL) {
        LOG_DEBUG("Invalid file format!\r\nUsage: ./process_table <raw csv file> -[operate type] <output csv file>");
        return EXIT_FAILURE;
    }

    LOG_DEBUG("raw_file = %s, operate type = %s, output file = %s", raw_file, operate_type, output_file);

    // parse the table type
    int ret = parse_table_type(raw_file);
    if (0 != ret || 0 == strcmp(g_table_type, "")) {
        LOG_DEBUG("parse the type of table failed!");
        return EXIT_FAILURE;
    }

    int i = 0;
    for (i = 0; i < LIST_LENGTH(g_info_handle_list); i++) {
        if (0 == strcmp(g_table_type, g_info_handle_list[i].type_name)) {
            ret = g_info_handle_list[i].info_handle_func(raw_file, output_file);
            if (0 != ret) {
                LOG_DEBUG("handle table failed!");
                return EXIT_FAILURE;
            }
        }
    }

    LOG_DEBUG("exit normal!");
    return 0;
}
