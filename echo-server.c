#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define MAX_CLIENT 100

struct Client {
    int sock;
    struct sockaddr_in addr;
};

struct Client clients[MAX_CLIENT];

int client_count = 0;

int option_echo = 0;
int option_broadcast = 0;

pthread_mutex_t mutex;

void usage(void) {
    printf("syntax : echo-server <port> [-e[-b]]\n");
    printf("sample : echo-server 1234 -e -b\n");
}

void add_client(int client_sock, struct sockaddr_in client_addr) {
    pthread_mutex_lock(&mutex);

    if (client_count < MAX_CLIENT) {
        clients[client_count].sock = client_sock;
        clients[client_count].addr = client_addr;
        client_count++;
    }

    pthread_mutex_unlock(&mutex);
}

void remove_client(int client_sock) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++) {
        if (clients[i].sock == client_sock) {

            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }

            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}

void broadcast_message(char* msg, int sender_sock) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++) {

        if (clients[i].sock == sender_sock)
            continue;

        send(clients[i].sock, msg, strlen(msg), 0);
    }

    pthread_mutex_unlock(&mutex);
}

void* client_thread(void* arg) {
    int client_sock = *(int*)arg;

    free(arg);

    char buf[BUF_SIZE];
    int len;

    while ((len = recv(client_sock, buf, BUF_SIZE - 1, 0)) > 0) {

        buf[len] = '\0';

        printf("client[%d] : %s", client_sock, buf);

        if (option_echo) {
            send(client_sock, buf, strlen(buf), 0);
        }

        if (option_broadcast) {
            broadcast_message(buf, client_sock);
        }
    }

    printf("client[%d] disconnected\n", client_sock);

    remove_client(client_sock);

    close(client_sock);

    return NULL;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        usage();
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);

    for (int i = 2; i < argc; i++) {

        if (strcmp(argv[i], "-e") == 0) {
            option_echo = 1;
        }
        else if (strcmp(argv[i], "-b") == 0) {
            option_broadcast = 1;
        }
    }

    int server_sock;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len = sizeof(client_addr);

    pthread_mutex_init(&mutex, NULL);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;

    setsockopt(server_sock,
               SOL_SOCKET,
               SO_REUSEADDR,
               &opt,
               sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock,
             (struct sockaddr*)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");

        close(server_sock);

        return EXIT_FAILURE;
    }

    if (listen(server_sock, 10) < 0) {

        perror("listen");

        close(server_sock);

        return EXIT_FAILURE;
    }

    printf("server listening on port %d\n", port);

    while (1) {

        int client_sock;

        client_sock = accept(server_sock,
                             (struct sockaddr*)&client_addr,
                             &client_len);

        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        printf("client connected : %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        add_client(client_sock, client_addr);

        pthread_t tid;

        int* pclient = (int*)malloc(sizeof(int));

        if (pclient == NULL) {
            perror("malloc");
            close(client_sock);
            continue;
        }

        *pclient = client_sock;

        pthread_create(&tid,
                       NULL,
                       client_thread,
                       pclient);

        pthread_detach(tid);
    }

    pthread_mutex_destroy(&mutex);

    close(server_sock);

    return 0;
}
