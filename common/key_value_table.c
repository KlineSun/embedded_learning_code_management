#include "key_value_table.h"
#include "common_util.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>


volatile int g_kv_init_flag = 0;
volatile int g_kv_count = 0;
key_value_node_t *g_kv_table = NULL;
pthread_mutex_t g_kv_mutex;

int parse_kv(char *buf, key_value_node_t *node)
{
    if (NULL == buf || NULL == node) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (2 > strlen(buf)) {
        LOG_INFO("empty line!");
        return -1;
    }

    if (NULL == strstr(buf, KEY_VALUE_SEPARATOR)) {
        LOG_INFO("format error: %s", buf);
        return -1;
    }

    char *key = strtok(buf, KEY_VALUE_SEPARATOR);
    char *value = strtok(NULL, KEY_VALUE_SEPARATOR);
    if (NULL == key || NULL == value) {
        LOG_INFO("Invalid buf!");
        return -1;
    }

    if ( MAX_TABLE_KEY_LENTH <= strlen(key) || MAX_TABLE_VALUE_LENTH <= strlen(value)) {
        LOG_INFO("key value is too long!");
        return -1;
    }

    trim_string(key);
    trim_string(value);

    memset(node->key, 0, MAX_TABLE_KEY_LENTH);
    memset(node->value, 0, MAX_TABLE_VALUE_LENTH);
    snprintf(node->key, MAX_TABLE_KEY_LENTH, "%s", key);
    snprintf(node->value, MAX_TABLE_VALUE_LENTH, "%s", value);

    // LOG_INFO("Get key: %s, value: %s", node->key, node->value);
    return 0;
}

int kv_table_init()
{
    pthread_mutex_init(&g_kv_mutex, NULL);

    pthread_mutex_lock(&g_kv_mutex);
    if (1 == g_kv_init_flag) {
        LOG_INFO("Already initialized");
        return 0;
    }
    pthread_mutex_unlock(&g_kv_mutex);
    LOG_INFO("key-value table init");

    FILE *fp = NULL;
    int ret = 0;
    // 打开kv文件
    fp = fopen(KEY_VALUE_TABLE_PATH, "r");
    if (NULL == fp) {
        fp = fopen(KEY_VALUE_TABLE_PATH, "w+");
        if (NULL == fp) {
            LOG_INFO("create %s failed!", KEY_VALUE_TABLE_PATH);
            return -1;
        }
        LOG_INFO("create %s success!", KEY_VALUE_TABLE_PATH);
    }

    // 初始化哈希表
    char line_buf[MAX_FILE_LINE_LENTH] = {0};
    while (fgets(line_buf, MAX_FILE_LINE_LENTH, fp) != NULL) {
        if (2 > strlen(line_buf) || '#' == line_buf[0] || is_all_spaces(line_buf))
            continue;

        key_value_node_t *node = (key_value_node_t *)malloc(sizeof(key_value_node_t));
        if (NULL == node) {
            LOG_INFO("malloc failed!");
            fclose(fp);
            return -1;
        }

        ret = parse_kv(line_buf, node);
        if (0 != ret) {
            LOG_INFO("parse key value failed!");
            free(node);
            node = NULL;
            continue;
        }

        // 添加到公共列表中去
        pthread_mutex_lock(&g_kv_mutex);
        if (NULL == g_kv_table) {
            // 为列表尾部置为NULL
            node->next = NULL;
            g_kv_table = node;
            g_kv_count++;
            continue;
        } else {
            LIST_ADD_NODE(g_kv_table, node);
            g_kv_count++;
        }
        pthread_mutex_unlock(&g_kv_mutex);
        memset(line_buf, 0, MAX_FILE_LINE_LENTH);
    }

    //LOG_INFO("Parsing the table ends with a total of %d key-value pairs obtained", g_kv_count);
    g_kv_init_flag = 1;
    fclose(fp);
    return 0;
}

int get_kv(const char *key, char *value)
{
    if (NULL == key || NULL == value) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key) || MAX_TABLE_KEY_LENTH < strlen(key)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    key_value_node_t *node = g_kv_table;
    while (NULL != node) {
        if (0 == strcmp(node->key, key)) {
            snprintf(value, MAX_TABLE_VALUE_LENTH, "%s", node->value);
            return 0;
        }
        node = node->next;
    }

    LOG_INFO("Not find %s in table", key);
    return -1;
}

int flush_kv_file()
{
    FILE *fp = NULL;
    fp = fopen(KEY_VALUE_TABLE_PATH, "w+");
    if (NULL == fp) {
        LOG_INFO("open %s failed!", KEY_VALUE_TABLE_PATH);
        return -1;
    }

    if (NULL == g_kv_table || 0 == g_kv_count) {
        LOG_INFO("No key-value pairs in list");
        fclose(fp);
        fp = NULL;
        return 0;
    }

    pthread_mutex_lock(&g_kv_mutex);
    key_value_node_t *node = g_kv_table;
    while (NULL != node) {
        fprintf(fp, "%s%s%s\n", node->key, KEY_VALUE_SEPARATOR, node->value);
        node = node->next;
    }
    fflush(fp);
    fclose(fp);
    fp = NULL;
    pthread_mutex_unlock(&g_kv_mutex);
    return 0;
}

int append_kv(const char *key, const char *value)
{
    if (NULL == key || NULL == value) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key) || 1 > strlen(value)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    char line[MAX_FILE_LINE_LENTH] = {0};
    snprintf(line, MAX_FILE_LINE_LENTH, "%s%s%s\n", key, KEY_VALUE_SEPARATOR, value);

    FILE *fp = NULL;
    fp = fopen(KEY_VALUE_TABLE_PATH, "a+");
    if (NULL == fp) {
        LOG_INFO("open %s failed!", KEY_VALUE_TABLE_PATH);
        return -1;
    }

    pthread_mutex_lock(&g_kv_mutex);
    int ret = fprintf(fp, "%s", line);
    if (1 > ret) {
        LOG_INFO("write %s into %s failed!", line, KEY_VALUE_TABLE_PATH);
        return -1;
    }

    fflush(fp);
    fclose(fp);
    fp = NULL;
    pthread_mutex_unlock(&g_kv_mutex);
    return 0;
}

int is_exist_key(const char *key)
{
    if (NULL == key) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key) || MAX_TABLE_KEY_LENTH < strlen(key)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    if (NULL == g_kv_table || 0 == g_kv_count) {
        LOG_INFO("No key-value pairs in list");
        return -1;
    }

    key_value_node_t *node = g_kv_table;
    while (NULL != node) {
        if (0 == strcmp(node->key, key)) {
            return 1;
        }
        node = node->next;
    }
    return 0;
}

int modify_kv(const char *key, const char *value)
{
    if (NULL == key || NULL == value) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key) || 1 > strlen(value)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    if (NULL == g_kv_table || 0 == g_kv_count) {
        LOG_INFO("No key-value pairs in list");
        return -1;
    }

    if (1 != is_exist_key(key)) {
        LOG_INFO("Not find %s in table", key);
        return -1;
    }

    key_value_node_t *node = g_kv_table;
    while (NULL != node) {
        if (0 == strcmp(node->key, key))
        {
            snprintf(node->value, MAX_TABLE_VALUE_LENTH, "%s", value);
            break;
        }
        node = node->next;
    }

    flush_kv_file();
    return 0;
}

int add_kv(const char *key, const char *value)
{
    if (NULL == key || NULL == value) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key) || 1 > strlen(value)
        || MAX_TABLE_KEY_LENTH < strlen(key)
        ||  MAX_TABLE_VALUE_LENTH < strlen(value)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    if (1 == is_exist_key(key)) {
        // LOG_INFO("Already exist %s in table, modify it!", key);
        modify_kv(key, value);
        return 0;
    }

    int ret = 0;
    key_value_node_t *node = (key_value_node_t *)malloc(sizeof(key_value_node_t));
    if (NULL == node) {
        LOG_INFO("malloc failed!");
        return -1;
    }
    
    memset(node->key, 0, MAX_TABLE_KEY_LENTH);
    memset(node->value, 0, MAX_TABLE_VALUE_LENTH);
    snprintf(node->key, MAX_TABLE_KEY_LENTH, "%s", key);
    snprintf(node->value, MAX_TABLE_VALUE_LENTH, "%s", value);
    
    // 添加到公共列表中去
    pthread_mutex_lock(&g_kv_mutex);
    if (NULL == g_kv_table) {
        // 为列表尾部置为NULL
        node->next = NULL;
        g_kv_table = node;
        g_kv_count++;
    } else {
        LIST_ADD_NODE(g_kv_table, node);
        g_kv_count++;
    }
    pthread_mutex_unlock(&g_kv_mutex);

    // 更新文件
    if (0 != access(KEY_VALUE_TABLE_PATH, F_OK)) {
        ret = flush_kv_file();
    } else {
        ret = append_kv(node->key , node->value);
    }
    if (0 != ret) {
        LOG_INFO("save key value into %s failed!", KEY_VALUE_TABLE_PATH);
        return -1;
    }

    // LOG_INFO("Save \"%s%s%s\" into table", key, KEY_VALUE_SEPARATOR, value);
    return 0;
}

int delete_kv(const char *key)
{
    if (NULL == key) {
        LOG_INFO("Invalid paramter!");
        return -1;
    }

    if (1 > strlen(key)) {
        LOG_INFO("Invalid key!");
        return -1;
    }

    if (NULL == g_kv_table || 0 == g_kv_count) {
        LOG_INFO("No key-value pairs in list");
        return -1;
    }

    key_value_node_t *node = g_kv_table;
    key_value_node_t *prev = NULL;
    pthread_mutex_lock(&g_kv_mutex);
    while (NULL != node) {
        if (0 == strcmp(node->key, key)) {
            if (NULL == prev) {
                g_kv_table = node->next;
            } else {
                prev->next = node->next;
            }
            free(node);
            node = NULL;
            g_kv_count--;
            break;
        }
        prev = node;
        node = node->next;
    }
    pthread_mutex_unlock(&g_kv_mutex);

    int ret = flush_kv_file();
    if (0 != ret) {
        LOG_INFO("fluse key value list into %s failed!", KEY_VALUE_TABLE_PATH);
        return -1;
    }

    return 0;
}

int destroy_kv_table()
{
    if (NULL == g_kv_table || 0 == g_kv_count) {
        LOG_INFO("No key-value pairs in list");
        return -1;
    }

    pthread_mutex_lock(&g_kv_mutex);
    LIST_FREE(g_kv_table);
    g_kv_table = NULL;
    pthread_mutex_unlock(&g_kv_mutex);

    pthread_mutex_destroy(&g_kv_mutex);
    return 0;
}

