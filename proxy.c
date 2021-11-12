#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    int * buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

void doit(int fd);
void sbuf_init(sbuf_t * sp, int n);
void sbuf_deinit(sbuf_t * sp);
void sbuf_insert(sbuf_t * sp, int item);
int sbuf_remove(sbuf_t * sp);
void * thread(void * vargp);
void handler(int sig);

sbuf_t sbuf;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    Signal(SIGPIPE, handler);
    listenfd = Open_listenfd(argv[1]);
    pthread_t tid;
    sbuf_init(&sbuf, 32);
    for (int i = 0; i < 32; ++i)
        Pthread_create(&tid, NULL, thread, NULL);
    while (1) {
        clientlen = sizeof(clientaddr);
        fprintf(stderr, "Blocking in Accept..\n");
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        fprintf(stderr, "Request from %s\n", inet_ntoa(((struct sockaddr_in *)&clientaddr)->sin_addr));
        sbuf_insert(&sbuf, connfd);
    }
    return 0;
}


void doit(int client_proxy_fd) {
    char host[128], port[8];
    rio_t rio;
    Rio_readinitb(&rio, client_proxy_fd);
    printf("Enter thread\n");

    int proxy_server_fd = socks_authenticate(client_proxy_fd, host, port);
    if (proxy_server_fd < 0) {
        printf("Invalid DNAME\n");
        return;
    }
   // int proxy_server_fd = Open_clientfd(host, port);
    printf("Built up connection with %s\n", host);

    socks_serve(client_proxy_fd, proxy_server_fd);
    Close(proxy_server_fd);
}

void sbuf_init(sbuf_t * sp, int n) {
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->slots, 0, n);
    Sem_init(&sp->items, 0, 0);
}

void sbuf_deinit(sbuf_t * sp) {
    Free(sp->buf);
}

void sbuf_insert(sbuf_t * sp, int item) {
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp) {
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}

void * thread(void * vargp) {
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

void handler(int sig) {
    return;
}
