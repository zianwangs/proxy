#include "csapp.h"
#include "poll.h"
#define MAX_READ (1 << 17)
int socks_pwd_authen(int);
int socks_authenticate(int, char*, char*);
void socks_serve(int, int);

static void non_block_fd(int fd) {
    int flags = fcntl(fd, F_GETFL, 0); 
    flags |= flags|O_NONBLOCK; 
    fcntl(fd, F_SETFL, flags);
}

int socks_pwd_authen(int fd) {
    char buf[128];
    short pwd_authen = 0x0205;
    int wc = write(fd, (char*)&pwd_authen, 2);
    char username[256], pwd[256];
    read(fd, buf, 2);
    read(fd, username, buf[1]);
    username[buf[1]] == '\0';
    read(fd, buf, 1);
    read(fd, pwd, buf[0]);
    pwd[buf[0]] = '\0';
    if (strcmp(username, "abc") || strcmp(pwd, "123")) {
        printf("Authentication failed\n");
        pwd_authen = 0x0105;
        rio_writen(fd, (char*)pwd_authen, 2);
        return -1;
    }
    printf("Password Authentication succeeded\n");
    return 0;
}

int socks_authenticate(int fd, char* host, char* port) {
    unsigned char buf[128] = {0};
    printf("Ready to receive socks5 handshake\n");
    int cnt = read(fd, buf, 6);
    printf("Handshake received %d bytes, Ver:%x NM:%x M:%x %x %x\n",cnt, buf[0], buf[1], buf[2], buf[3], buf[4]);
    if (buf[2] == 0x02 || buf[3] == 0x02 || buf[4] == 0x02) 
        if (socks_pwd_authen(fd) == -1)  
            return -1; 
    short NO_AUTHEN = 0x0005;
    char* handshake_reply = &NO_AUTHEN;
    rio_writen(fd, (char*)handshake_reply, 2);
    printf("Handshake replied, ready to process requests\n");
    cnt = recv(fd, buf, 4, 0);
    cnt += recv(fd, buf + 4, 64, 0);
    int dst_port = buf[cnt - 2] * 256 + buf[cnt - 1];
    sprintf(port, "%d", dst_port);
    if (buf[3] == 0x01) {
        printf("IPv4:%d.%d.%d.%d, Port:%d\n", buf[4],buf[5],buf[6],buf[7],dst_port);
        memcpy(host, buf + 4, 4);
        host[4] = '\0';
    } else if (buf[3] == 0x03) {
        buf[cnt - 2] = '\0';
        memcpy(host, buf + 5, cnt - 6);
        printf("DNAME: %s, Port:%d\n", host, dst_port);
    }
    if (strcmp(host, "www.pornhub.com") == 0) return -1;
    int proxy_server_fd = open_clientfd(host, port);
    unsigned char SUCCESS_REPLY[10];
    SUCCESS_REPLY[0] = 5, SUCCESS_REPLY[1] = proxy_server_fd > 0 ? 0 : 4;
    SUCCESS_REPLY[2] = 0, SUCCESS_REPLY[3] = 1, SUCCESS_REPLY[4] = 3;
    SUCCESS_REPLY[5] = 16, SUCCESS_REPLY[6] = 159, SUCCESS_REPLY[7] = 128;
    SUCCESS_REPLY[8] = 10, SUCCESS_REPLY[9] = 80;
    rio_writen(fd, SUCCESS_REPLY, 10);
    printf("Reply sent, handshake done\n");
    return proxy_server_fd;
        
}

void socks_serve(int client_proxy_fd, int proxy_server_fd) {
    char buf[MAX_READ];
    int cnt1 = -1, cnt2 = -1;
    printf("Start passing data\n");
    // non_block_fd(client_proxy_fd);
    // non_block_fd(proxy_server_fd);
    struct pollfd pfds[2] = {0};
    pfds[0].fd = client_proxy_fd;
    pfds[1].fd = proxy_server_fd;
    pfds[0].events = pfds[1].events = POLLIN;
    while (poll(pfds, 2, -1) != -1) {
        cnt1 = cnt2 = -1;
        if (pfds[0].revents & POLLERR) break;
        if (pfds[1].revents & POLLERR) break;
        if (pfds[0].revents & POLLHUP) break;
        if (pfds[1].revents & POLLHUP) break;
        if (pfds[0].revents & POLLIN) {
            cnt1 = recv(client_proxy_fd, buf, MAX_READ, 0);
            if (cnt1 == 0) break;
            send(proxy_server_fd, buf, cnt1, 0);
        }
        if (pfds[1].revents & POLLIN) {
            cnt2 = recv(proxy_server_fd, buf, MAX_READ, 0);
            send(client_proxy_fd, buf, cnt2, 0);
        }
        if (errno == ECONNRESET) break;
    }
    // EINTR, EPIPE, EWOULDBLOCK, EAGAIN
}
