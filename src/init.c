#include <ft_ping.h>

int     init_sockin(struct sockaddr_in *sock) {
    struct addrinfo hints, *res;
    int             ret;

    memset(sock, 0, sizeof(*sock));
    sock->sin_family = AF_INET;
    if (inet_pton(AF_INET, av, &sock->sin_addr) == 1)
        return (1);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;
    ret = getaddrinfo(av, NULL, &hints, &res);
    if (ret != 0)
        return (fprintf(stderr, "ft_ping: unknown host\n"), 0);
    sock->sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return (1);
}

void    init_icmphdr(struct icmphdr *packet, int seq) {
    struct timespec now;
    unsigned char   *payload;
    size_t          i;

    memset(packet, 0, PACKET_SIZE);
    packet->type = ICMP_ECHO;
    packet->code = 0;
    packet->un.echo.id = htons(g_ident);
    packet->un.echo.sequence = htons(seq);

    payload = (unsigned char *)packet + sizeof(struct icmphdr);
    clock_gettime(CLOCK_MONOTONIC, &now);
    memcpy(payload, &now, sizeof(now));
    for (i = sizeof(now); i < DATA_LEN; i++)
        payload[i] = (unsigned char)i;

    packet->checksum = 0;
    packet->checksum = checksum(packet, PACKET_SIZE);
}
