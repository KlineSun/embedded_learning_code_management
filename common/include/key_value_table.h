#ifndef KEY_VALUE_TABLE_H
#define KEY_VALUE_TABLE_H

#define KEY_VALUE_TABLE_PATH "/home/book/etc/user_kv_table"
#define MAX_TABLE_KEY_LENTH   128
#define MAX_TABLE_VALUE_LENTH 256
#define KEY_VALUE_SEPARATOR   ":"

typedef struct key_value_node {
    char key[MAX_TABLE_KEY_LENTH];
    char value[MAX_TABLE_VALUE_LENTH];
    struct key_value_node *next;
} key_value_node_t;


int kv_table_init();
int get_kv(const char *key, char *value);
int add_kv(const char *key, const char *value);
int delete_kv(const char *key);
int destroy_kv_table();

#endif // KEY_VALUE_TABLE_H