#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int sock;

void usage(void) {
    printf("syntax : echo-client <ip> <port>\n");
    printf("sample : echo-client 192.168.10.2 1234\n");
}

void* recv_thread(void* arg) {

    char buf[BUF_SIZE];

    int len;

    while ((len = recv(sock, buf, BUF_SIZE - 1, 0)) > 0) {

        buf[len] = '\0';

        printf("%s", buf);

        fflush(stdout);
    }

    printf("server disconnected\n");

    close(sock);

    exit(0);

    return NULL;
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        usage();
        return EXIT_FAILURE;
    }

    char* ip = argv[1];

    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET,
                  ip,
                  &server_addr.sin_addr) <= 0) {

        perror("inet_pton");

        close(sock);

        return EXIT_FAILURE;
    }

    if (connect(sock,
                (struct sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");

        close(sock);

        return EXIT_FAILURE;
    }

    printf("connected to server\n");

    pthread_t tid;

    pthread_create(&tid,
                   NULL,
                   recv_thread,
                   NULL);

    char buf[BUF_SIZE];

    while (fgets(buf, BUF_SIZE, stdin) != NULL) {

        send(sock, buf, strlen(buf), 0);
    }

    close(sock);

    return 0;
}
