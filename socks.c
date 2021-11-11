#include "csapp.h"

void socks_authenticate(int fd) {
    char buf[MAXLINE];
    printf("Ready to receive socks5 handshake\n");
    //int cnt = rio_readn(fd, buf, 256);
    int cnt = rio_readn(fd, buf, 3);
    //int cnt = read(fd, buf, 20);
    // sscanf(buf, "%s %s %s", method, urll, version);
    printf("Handshake received, Ver:%x NM:%x M:%x\n", buf[0], buf[1], buf[2]);
    short NO_AUTHEN = 0x0005;
    char* handshake_reply = &NO_AUTHEN;
    rio_writen(fd, (char*)handshake_reply, 2);
    printf("Handshake replied, ready to process requests\n");
    cnt = read(fd, buf, 256);
    printf("Request received %d bytes\n",cnt);
    int dst_port = ((unsigned char)buf[cnt - 2]) * 256 + (unsigned char)buf[cnt - 1];
    if (buf[3] == 0x01) {
        printf("IPv4:%d.%d.%d.%d, Port:%d\n", buf[4],buf[5],buf[6],buf[7],dst_port);
    } else if (buf[3] == 0x03) {
        buf[cnt - 2] = '\0';
        printf("DNS: %s, Port:%d\n", buf + 4, dst_port);
    }
    
}
