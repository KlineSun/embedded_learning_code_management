#include "socket_tst.h"
#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


int g_cln_sock = -1;
const char *msg_list[] = {
    "Hello server",
    "Request connect",
    "Sending message",
    "Waiting response",
    "Receive response message"
};

static void *socket_client_pthread(void *priv)
{
    if (g_cln_sock < 0) {
        LOG_INFO("Socket invalid!");
        return NULL;
    }

    char recv_buf[MAX_SOCKET_MSG_LEN] = {0};
    int list_len = sizeof(msg_list) / sizeof(msg_list[0]);
    for (int i = 0; i < list_len; i++) {
        if (send(g_cln_sock, msg_list[i], strlen(msg_list[i]), 0) < strlen(msg_list[i])) {
            LOG_INFO("send  msg to server socket failed: %s", strerror(errno));
            continue;
        }

        if (recv(g_cln_sock, recv_buf, MAX_SOCKET_MSG_LEN, 0) <= 0) {
            LOG_INFO("receive  msg from server socket failed: %s", strerror(errno));
            continue;
        }
        LOG_INFO("Receive response message from server: %s", recv_buf);
        sleep(2);
    }
    return NULL;
}

int socket_client_init()
{
    // open socket
    g_cln_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_cln_sock < 0) {
        LOG_INFO("open socket failed: %s", strerror(errno));
        return -1;
    }

    // set client's attributes
    struct sockaddr_in svr_addr;
    memset(&svr_addr, 0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port   = htons(SOCKET_SERVER_PORT);
    if (inet_pton(AF_INET, SOCKET_SERVER_IP, &svr_addr.sin_addr) <= 0) {
        LOG_INFO("inet_pton IP address failed!");
        return -1;
    }

    // connect
    if (connect(g_cln_sock, (struct sockaddr*)&svr_addr, sizeof(struct sockaddr_in)) != 0) {
        LOG_INFO("Bind socket failed: %s", strerror(errno));
        return -1;
    }

    // create monitor thread
    pthread_t tid = -1;
    int ret = pthread_create(&tid, NULL, socket_client_pthread, NULL);
    if (ret != 0 || tid <= 0) {
        LOG_INFO("Create socket monitor thread failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}


void socket_client_deinit()
{
    if (g_cln_sock >= 0)
        close(g_cln_sock);
}
