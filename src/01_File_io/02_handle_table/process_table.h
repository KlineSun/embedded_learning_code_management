#include <stdio.h>

#define MAX_BUF_SIZE 2048
#define LIST_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

char *get_time_str();
#define LOG_DEBUG(format, ...) printf("%s %s() %d: "format"\n", get_time_str(), __func__, __LINE__, ##__VA_ARGS__)

typedef enum {
    STUDENT = 0,
    FRUIT
} table_info_type;

typedef struct
{
    table_info_type type;
    int (*info_handle_func)(const char *info, char *result, int len);
} info_handle_t;

/*typedef struct {
    // table info type
    table_info_type type;

    // parse type of info
    int (*parse_type)(char *info);

    // handle input information
    info_handle_t *info_handle;
} table_parser_t;*/


// student type struct begin
typedef enum {
    CHINESE = 1,
    MATH,
    ENGILISH,
    PHYSICS,
    CHEMISTRY,
    BIOLOGY,
    POLITICS,
    HISTORY,
    GEOGRAPHY
} course_type;

typedef struct {
    course_type type;
    char name[32];
    int index;
    float score;
} course_t;

typedef struct {
    int rank;
    int index;
    const char *comment;
} mark_t;

typedef struct student_t 
{
    char name[32];
    course_t course;
    int course_count;
    float total_score;
    int rank;
    mark_t mark;
    student_t *next;
} student_t;
// student type struct end

// fruit type struct begin
typedef enum {
    APPLE = 1,
    BANANAS,
    PEARS,
    ORANGES,
    TANGERINES,
    WATERMELON,
    STRAWBERRIES
} fruit_type;

typedef struct fruit_t
{
    fruit_type tpye;
    char * name
    float costs;
    float price;
    float sales_volume;
    float total;
    float margins;
    fruit_t *next;
} fruit_t;
// fruit type struct end


static info_handle_t g_info_handle_list[] = {
    {STUDENT, student_info_handler},
    {FRUIT,   fruit_info_handler}
};

static mark_t mark_list[] = {
    {1, "Perfect!"},
    {2, "Wonderful!"},
    {3, "Pretty good!"},
    {4, "Keep it up!"},
};

