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
struct sockaddr_in g_target_svr_addr;
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
        // send or sendto
        /*
        if (send(g_cln_sock, msg_list[i], strlen(msg_list[i]), 0) < strlen(msg_list[i])) {
            LOG_INFO("send  msg to server socket failed: %s", strerror(errno));
            continue;
        }
        */

        // recv or recvfrom
        /*
        if (recv(g_cln_sock, recv_buf, MAX_SOCKET_MSG_LEN, 0) <= 0) {
            LOG_INFO("receive  msg from server socket failed: %s", strerror(errno));
            continue;
        }
        */

        // sendto
        socklen_t len = sizeof(struct sockaddr_in);
        int send_cnt = sendto(g_cln_sock, msg_list[i], strlen(msg_list[i]), 0, (const struct sockaddr*)&g_target_svr_addr, len);
        if (send_cnt <= 0) {
            LOG_INFO("send msg to client socket failed: %s", strerror(errno));
            continue;
        }

        // recvfrom
        int recv_cnt = recvfrom(g_cln_sock, recv_buf, MAX_SOCKET_MSG_LEN, 0, (const struct sockaddr*)&g_target_svr_addr, &len);
        if (recv_cnt <= 0) {
            LOG_INFO("recv msg from client socket failed: %s", strerror(errno));
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
    // tcp -> SOCK_STREAM
    // udp -> SOCK_DGRAM
    g_cln_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_cln_sock < 0) {
        LOG_INFO("open socket failed: %s", strerror(errno));
        return -1;
    }

    // connect
    // set client's attributes
    memset(&g_target_svr_addr, 0, sizeof(struct sockaddr_in));
    g_target_svr_addr.sin_family = AF_INET;
    g_target_svr_addr.sin_port   = htons(SOCKET_SERVER_PORT);
    if (inet_pton(AF_INET, SOCKET_SERVER_IP, &g_target_svr_addr.sin_addr) <= 0) {
        LOG_INFO("inet_pton IP address failed!");
        return -1;
    }
    if (connect(g_cln_sock, (struct sockaddr*)&g_target_svr_addr, sizeof(struct sockaddr_in)) != 0) {
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
