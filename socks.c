#include "csapp.h"
#include "poll.h"
#define MAX_READ (1 << 16)
int socks_authenticate(int, char*, char*);
void socks_serve(int, int);

static void non_block_fd(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); 
    flags |= flags|O_NONBLOCK; 
    fcntl(fd, F_SETFL, flags);
}

int socks_authenticate(int fd, char* host, char* port) {
    unsigned char buf[MAXLINE];
    printf("Ready to receive socks5 handshake\n");
    //int cnt = rio_readn(fd, buf, 256);
    int cnt = rio_readn(fd, buf, 3);
    //int cnt = read(fd, buf, 20);
    printf("Handshake received, Ver:%x NM:%x M:%x\n", buf[0], buf[1], buf[2]);
    short NO_AUTHEN = 0x0005;
    char* handshake_reply = &NO_AUTHEN;
    rio_writen(fd, (char*)handshake_reply, 2);
    printf("Handshake replied, ready to process requests\n");
    cnt = read(fd, buf, 64);
    printf("Request received %d bytes\n",cnt);
    int dst_port = buf[cnt - 2] * 256 + buf[cnt - 1];
    sprintf(port, "%d", dst_port);
    if (buf[3] == 0x01) {
        printf("IPv4:%d.%d.%d.%d, Port:%d\n", buf[4],buf[5],buf[6],buf[7],dst_port);
        for (int i = 0; i < 4; ++i)
            host[i] = buf[i + 4];
        host[4] = '\0';
    } else if (buf[3] == 0x03) {
        buf[cnt - 2] = '\0';
        memcpy(host, buf + 5, cnt - 6);
        printf("DNAME: %s, Port:%d\n", host, dst_port);
    }
    int proxy_server_fd = open_clientfd(host, port);
    unsigned char SUCCESS_REPLY[10];
    SUCCESS_REPLY[0] = 5, SUCCESS_REPLY[1] = proxy_server_fd > 0 ? 0 : 4;
    SUCCESS_REPLY[2] = 0, SUCCESS_REPLY[3] = 1, SUCCESS_REPLY[4] = 3;
    SUCCESS_REPLY[5] = 16, SUCCESS_REPLY[6] = 159, SUCCESS_REPLY[7] = 128;
    SUCCESS_REPLY[8] = 0, SUCCESS_REPLY[9] = 80;
    rio_writen(fd, SUCCESS_REPLY, 10);
    printf("Reply sent, handshake done\n");
    return proxy_server_fd;
        
}

void socks_serve(int client_proxy_fd, int proxy_server_fd) {
    char buf[MAX_READ];
    int cnt1 = -1, cnt2 = -1;
    printf("Start passing data\n");
    struct pollfd pfds[2];
    non_block_fd(client_proxy_fd);
    non_block_fd(proxy_server_fd);
    pfds[0].fd = client_proxy_fd;
    pfds[1].fd = proxy_server_fd;
    pfds[0].events = pfds[2].events = POLLIN;
    //while (poll(&pfds, 2, -1) != -1) {
        while (1) {
            cnt1 = cnt2 = -2;
            if ((cnt1 = recv(client_proxy_fd, buf, MAX_READ, MSG_WAITALL)) > 0) {
                printf("Read %d bytes from client->proxy\n", cnt1);
                if (rio_writen(proxy_server_fd, buf, cnt1) < 0) {
                    break;
                }
                printf("Write %d bytes from proxy->server\n", cnt1);
            }
            if ((cnt1 == -1 || cnt1 == 0) && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("%d\n", errno);
                break;
            }
            if ((cnt2 = recv(proxy_server_fd, buf, MAX_READ, MSG_WAITALL)) > 0) {
                printf("Read %d bytes from client->proxy\n", cnt2);
                if (rio_writen(client_proxy_fd, buf, cnt2) < 0) {
                    break;
                }
                printf("Write %d bytes from proxy->server\n", cnt2);
            }
            if ((cnt2 == -1 || cnt2 == 0) && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("%d\n", errno);
                break;
            }
            if (cnt1 == 0 && cnt2 == 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) break;
       // }
    }
    printf("Nothing to read from client, end session\n");

}
