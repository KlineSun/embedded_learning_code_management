#ifndef SOCKET_TST_H
#define SOCKET_TST_H

#define SOCKET_SERVER_PORT  (8888)
#define SOCKET_SERVER_IP    "127.0.0.1"
#define MAX_LESTEN_PORT     (5)
#define MAX_SOCKET_MSG_LEN  (2048)
#define SOCKET_SERVER_RESPONSE_KEY  "MSG OK"

int socket_server_init();
int socket_client_init();
void socket_server_deinit();
void socket_client_deinit();

#endif // SOCKET_TST_H