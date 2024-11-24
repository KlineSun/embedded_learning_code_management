#include <stdio.h>
#include "common_util.h"

#define MAX_BUF_SIZE 2048
#define LIST_LENGTH(x) (sizeof(x) / sizeof((x)[0]))
#define null (NULL)
#define true (TRUE)
#define false (FALSE)

int student_info_handler(const char *input_file);
int fruit_info_handler(const char *input_file);
int count_sub_str(const char* str, const char *sub_str);

typedef struct
{
    char *type_name;
    int (*info_handle_func)(const char *input_file);
} info_handle_t;

static info_handle_t g_info_handle_list[] = {
    {"course_table", student_info_handler},
    {"sale_table",   fruit_info_handler}
};

// student type struct begin
typedef enum {
    NAME = 0,
    CHINESE,
    MATH,
    ENGLISH,
    PHRSICS,
    CHEMMISTR,
    BIOLOGY,
    POLITICS,
    HISTORY,
    GEOGRAPHY,
    TOTAL,
    RANK,
    MARK,
    INVALID_TYPE = -1
} data_type_enum;

// typedef struct course_t course_t;
typedef struct course_t {
    char name[32];
    float score;
    struct course_t *next;
} course_t;

typedef struct {
    int rank;
    char *comment;
} mark_t;

static mark_t mark_list[] = {
    {1, "Perfect!"},
    {2, "Wonderful!"},
    {3, "Pretty good!"},
    {4, "Keep it up!"},
};

typedef struct student_t
{
    char name[32];
    course_t *course;
    int course_count;
    float total_score;
    int rank;
    mark_t mark;
    struct student_t *next;
} student_t;
// student type struct end

// fruit type struct begin
const char *fruit_type_list[] = {"Apple", "Bananas" , "Pears", "Oranges"
                                 "Tangerines", "Watermelon" , "Strawberries"};

typedef struct fruit_t
{
    char * name;
    float costs;
    float price;
    float sales_volume;
    float total;
    float margins;
    struct fruit_t *next;
} fruit_t;
// fruit type struct end

typedef struct {
    int item_count;
    char **items;
} character_array_2d_t;
