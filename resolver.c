#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>

/*
i dont know what the hell i did, i think put in the wrong code lmao
what i was doing was wrong, after months i came to read this crap and I got scared...
and thats why I made these adjustments
*/

#define MAX_IP_LEN INET6_ADDRSTRLEN

void get_ips(const char *host) {
    struct addrinfo hints = {0}, *res, *p;
    hints.ai_family = AF_UNSPEC; // ipv4 or ipv6, or use AF_INET for ipv4 or AF_INET6 to ipv6
    hints.ai_socktype = 0;   // any socket

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        perror("getaddrinfo");
        return;
    }

    printf("IPs for %s:\n", host);
    for (p = res; p != NULL; p = p->ai_next) { // goes through the shit that returned
        char ip[MAX_IP_LEN];
        void *addr;
        const char *ver;

        if (p->ai_family == AF_INET) {
            addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
            ver = "IPv4";
        } else if (p->ai_family == AF_INET6) {
            addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
            ver = "IPv6";
        } else {
            continue;
        }

        inet_ntop(p->ai_family, addr, ip, sizeof(ip)); //converts bin to string (4 bytes for ipv4 or 16 bytes to ipv6)
        printf("  %s: %s\n", ver, ip); // print this shit
    }

    freeaddrinfo(res); // no memory leak monkey
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    get_ips(argv[1]);
    return 0;
}
