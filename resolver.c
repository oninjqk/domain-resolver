#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netinet/in.h>

#define timeout 2

void resolve(const char *host) {
    struct addrinfo hints, *res, *cur;
    char ipstr[inet6_addrstrlen];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = af_unspec;
    hints.ai_socktype = sock_stream;

    if (getaddrinfo(host, null, &hints, &res) != 0) return;

    printf("ips for %s:\n", host);

    for (cur = res; cur != null; cur = cur->ai_next) {
        void *addr;
        char *ver;

        if (cur->ai_family == af_inet) {
            addr = &((struct sockaddr_in*)cur->ai_addr)->sin_addr;
            ver = "ipv4";
        } else {
            addr = &((struct sockaddr_in6*)cur->ai_addr)->sin6_addr;
            ver = "ipv6";
        }

        inet_ntop(cur->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ver, ipstr);
    }

    freeaddrinfo(res);
}

int port_check(const char *ip, int port) {
    int sock = socket(af_inet, sock_stream, 0);
    if (sock < 0) return 0;

    struct sockaddr_in addr = {0};
    addr.sin_family = af_inet;
    addr.sin_port = htons(port);
    if (inet_pton(af_inet, ip, &addr.sin_addr) <= 0) {
        close(sock);
        return 0;
    }

    int flags = fcntl(sock, f_getfl, 0);
    fcntl(sock, f_setfl, flags | o_nonblock);

    connect(sock, (struct sockaddr*)&addr, sizeof addr);

    fd_set fdset;
    fd_zero(&fdset);
    fd_set(sock, &fdset);
    struct timeval tv = {timeout, 0};

    if (select(sock + 1, null, &fdset, null, &tv) > 0) {
        int err = 0;
        socklen_t len = sizeof err;
        getsockopt(sock, sol_socket, so_error, &err, &len);
        close(sock);
        return err == 0;
    }

    close(sock);
    return 0;
}

int real_ip(const char *host, int port, char *out_ip, size_t out_len) {
    struct addrinfo hints = {0}, *res, *cur;
    hints.ai_family = af_inet;
    hints.ai_socktype = sock_stream;

    if (getaddrinfo(host, null, &hints, &res) != 0) return 0;

    int sock = -1;
    for (cur = res; cur != null; cur = cur->ai_next) {
        sock = socket(af_inet, sock_stream, 0);
        if (sock < 0) continue;

        struct sockaddr_in addr = *(struct sockaddr_in *)cur->ai_addr;
        addr.sin_port = htons(port);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            struct sockaddr_in real_addr;
            socklen_t len = sizeof(real_addr);
            if (getpeername(sock, (struct sockaddr *)&real_addr, &len) == 0) {
                inet_ntop(af_inet, &real_addr.sin_addr, out_ip, out_len);
                close(sock);
                freeaddrinfo(res);
                return 1;
            }
            close(sock);
        } else {
            close(sock);
        }
    }
    freeaddrinfo(res);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s <host>\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];

    resolve(host);

    char real_ip[inet_addrstrlen] = {0};
    if (!real_ip(host, 443, real_ip, sizeof(real_ip))) {
        printf("could not connect to %s on port 443\n", host);
        return 1;
    }

    printf("\nreal connected ip on port 443: %s\n", real_ip);

    int ports[] = {80, 443, 22, 21, 3306};
    printf("testing open ports on %s:\n", real_ip);
    for (int i = 0; i < sizeof(ports) / sizeof(ports[0]); i++) {
        if (port_check(real_ip, ports[i])) {
            printf(" port %d: open\n", ports[i]);
        }
    }

    return 0;
}