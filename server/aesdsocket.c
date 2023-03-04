#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define FILE_NAME "/var/tmp/aesdsocketdata"

static char *const PORT = "9000";
static bool sysKill = false;
static bool daemonMode = false;

void addSigActions();

void signalHandler(int signal);

int bindSocket(int socket_fd);

int initSocket();

void handleSocket(int socket_fd);

char *buffer2File(char *buff, int buffLen);

void file2socket(int current_socket);

int main(int argc, char *argv[]) {
    // check if it wants to run in daemon mode
    if (argc >= 2 && !strcmp(argv[1], "-d")) {
        daemonMode = true;
    }

    // open system logger
    openlog(NULL, 0, LOG_USER);
    // add signal actions
    addSigActions();

    // get socket file descriptor
    int socket_fd = initSocket();
    // bind socket to port
    if (bindSocket(socket_fd)){
        exit(-1);
    }

    if (fork() != 0) {
        exit(0);
    }

    // listen to socket
    listen(socket_fd, 5);

    while (!sysKill) {
        handleSocket(socket_fd);
    }

    close(socket_fd);
    remove(FILE_NAME);
}

void handleSocket(int socket_fd) {
    socklen_t addrClientLen = sizeof(struct sockaddr_in);
    struct sockaddr_in addrClient;
    int current_socket = accept(socket_fd, (void *) &addrClient, &addrClientLen);

    if (current_socket < 0) {
        if (sysKill)
            return;
        exit(1);
    } else {
        char clientAddr[20];
        char *buff = NULL;
        int buffSz, buffLen;
        ssize_t rcvCnt;

        syslog(LOG_DEBUG, "Accepted connection from %s", clientAddr);

        while (!sysKill) {
            if (buff == NULL) {
                buffLen = 0;
                buffSz = 0;
            }
            if (buffLen == buffSz) {
                if (buffSz == 0) {
                    buffSz = 100;
                } else {
                    buffSz *= 2;
                }
                buff = realloc(buff, buffSz);
            }

            rcvCnt = recv(current_socket, buff + buffLen, 1, 0);
            if (rcvCnt == -1 || rcvCnt == 0) {
                break;
            } else if (rcvCnt == 1) {
                buffLen++;
                if (buff[buffLen - 1] == '\n') {
                    write(1, buff, buffLen);

                    buff = buffer2File(buff, buffLen);
                    file2socket(current_socket);
                }
            }
        }
        if (buff) {
            free(buff);
            buff = NULL;
        }
        close(current_socket);
    }
}

void file2socket(int current_socket) {
    FILE *file;
    file = fopen(FILE_NAME, "rb");
    if (file == NULL) {
        exit(1);
    }
    while (1) {
        char cc;
        int c = fgetc(file);
        if (c == EOF)
            break;
        cc = (char)c;
        send(current_socket, &cc, 1, 0);
    }
    fclose(file);
}

char *buffer2File(char *buff, int buffLen) {
    FILE *file;
    file = fopen(FILE_NAME, "ab");
    if (file == NULL) {
        exit(1);
    }
    fwrite(buff, 1, buffLen, file);
    fclose(file);

    free(buff);
    return NULL;
}

int initSocket() {
    syslog(LOG_DEBUG, "initSocket()");
    int socket_fd;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "Error allocating socket file descriptor\n");
        exit(1);
    }
    int so_reuseaddr = 1;
    setsockopt(socket_fd, IPPROTO_TCP, SO_REUSEADDR, &so_reuseaddr, sizeof(int));
    syslog(LOG_DEBUG, "initSocket() completed");
    return socket_fd;
}

int bindSocket(int socket_fd) {
    syslog(LOG_DEBUG, "bindSocket()");
    struct addrinfo *paiAddrinfo;
    struct addrinfo reqAddrinfo = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
            .ai_protocol = 0,
            .ai_flags = AI_PASSIVE,
    };
    int ret;
    if (getaddrinfo(NULL, PORT, &reqAddrinfo, &paiAddrinfo) != 0) {
        syslog(LOG_ERR,"Error in getaddrinfo\n");
        ret = 1;
    }
    if (bind(socket_fd, paiAddrinfo->ai_addr, paiAddrinfo->ai_addrlen) != 0) {
        syslog(LOG_ERR,"Error in bind\n");
        ret = 1;
    }
    freeaddrinfo(paiAddrinfo);

    syslog(LOG_DEBUG, "bindSocket() completed with return code %d", ret);
    return ret;
}

void addSigActions() {
    syslog(LOG_DEBUG, "addSigActions()");

    struct sigaction act = {
            .sa_handler = signalHandler,
    };
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    syslog(LOG_DEBUG, "addSigActions() Successfully");
}

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        syslog(LOG_DEBUG, "Caught signal, exiting");
        sysKill = true;
    }
}