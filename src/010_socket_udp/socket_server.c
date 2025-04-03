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
#include <arpa/inet.h>
#include <unistd.h>

int g_srv_sock = -1;

static void *socket_server_monitor(void *priv)
{
    if (g_srv_sock < 0) {
        LOG_DEBUG("Socekt invalid!");
        return NULL;
    }

    // accpet
    // only for tcp, no need in udp.
    /*
    int cln_sock = -1;
    int accept_retry = 5;
re_accpet:
    accept_retry--;
    if (cln_sock >= 0) {
        close(cln_sock);
        cln_sock = -1;
    }
    struct sockaddr_in cln_addr;
    socklen_t len = sizeof(cln_addr);
    cln_sock = accept(g_srv_sock, (struct sockaddr*)&cln_addr, &len);
    if (cln_sock < 0 && accept_retry > 0) {
        LOG_DEBUG("accept client socket failed: %s", strerror(errno));
        goto re_accpet;
    }
    
    // get  client info: ip、port
    char cln_ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &cln_addr.sin_addr, cln_ip, INET_ADDRSTRLEN);
    unsigned short port = ntohs(cln_addr.sin_port);
    LOG_DEBUG("accept a connect from IP[%s] port[%d]", cln_ip, port);

    char recv_buf[MAX_SOCKET_MSG_LEN] = {0};
    while (true) {
        // recv msg
        int recv_cnt = recv(cln_sock, recv_buf, MAX_SOCKET_MSG_LEN, 0);
        if (recv_cnt <= 0 && accept_retry > 0) {
            LOG_DEBUG("recv  msg from client socket failed: %s", strerror(errno));
            goto re_accpet;
        }

        // handle msg
        LOG_DEBUG("server get client msg: %s", recv_buf);

        // response to client
        int send_cnt = send(cln_sock, SOCKET_SERVER_RESPONSE_KEY, strlen(SOCKET_SERVER_RESPONSE_KEY), 0);
        if (send_cnt < strlen(SOCKET_SERVER_RESPONSE_KEY) && accept_retry > 0) {
            LOG_DEBUG("send  msg from client socket failed: %s", strerror(errno));
            goto re_accpet;
        }
    }*/


    while (true) {
        char recv_buf[MAX_SOCKET_MSG_LEN] = {0};
        struct sockaddr_in cln_addr;
        socklen_t len = (socklen_t)sizeof(struct sockaddr_in);
        // recvfrom
        int recv_cnt = recvfrom(g_srv_sock, recv_buf, MAX_SOCKET_MSG_LEN, 0, (const struct sockaddr*)&cln_addr, &len);
        if (recv_cnt <= 0) {
            LOG_DEBUG("recv msg from client socket failed: %s", strerror(errno));
            continue;
        }

        // get client info: ip, port
        LOG_DEBUG("accept a connect from IP[%s] port[%d]", inet_ntoa(cln_addr.sin_addr), ntohs(cln_addr.sin_port));
        LOG_DEBUG("server get client msg: %s", recv_buf);

        // sendto
        int send_cnt = sendto(g_srv_sock, SOCKET_SERVER_RESPONSE_KEY, strlen(SOCKET_SERVER_RESPONSE_KEY), 0,
                        (const struct sockaddr*)&cln_addr, len);
        if (send_cnt <= 0) {
            LOG_DEBUG("send msg to client socket failed: %s", strerror(errno));
            continue;
        }

    }
    return NULL;
}

int socket_server_init()
{
    // open socket
    // tcp -> SOCK_STREAM
    // udp -> SOCK_DGRAM
    g_srv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_srv_sock < 0) {
        LOG_DEBUG("open socket failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in svr_addr;
    memset(&svr_addr, 0, sizeof(struct sockaddr_in));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port   = htons(SOCKET_SERVER_PORT);
    svr_addr.sin_addr.s_addr    = INADDR_ANY;

    // bind
    if (bind(g_srv_sock, (const struct sockaddr*)&svr_addr, sizeof(struct sockaddr_in)) != 0) {
        LOG_DEBUG("Bind socket failed: %s", strerror(errno));
        return -1;
    }

    // listen
    // only for tcp, no need in udp.
    /*
    if (listen(g_srv_sock, MAX_LESTEN_PORT) != 0) {
        LOG_DEBUG("Listen socket failed: %s", strerror(errno));
        return -1;
    }*/

    // create monitor thread
    pthread_t tid = -1;
    int ret = pthread_create(&tid, NULL, socket_server_monitor, NULL);
    if (ret != 0 || tid <= 0) {
        LOG_DEBUG("Create socket monitor thread failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

void socket_server_deinit()
{
    if (g_srv_sock >= 0)
        close(g_srv_sock);
}