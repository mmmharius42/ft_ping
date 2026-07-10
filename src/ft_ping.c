#include <ft_ping.h>

int    init_sockin(char **av, struct sockaddr_in *sock) {
    memset(sock, 0, sizeof(*sock));
    sock->sin_family = AF_INET;
    return (inet_pton(sock->sin_family, av[1], &sock->sin_addr));
}

void     init_icmphdr(struct icmphdr *packet) {
    memset(packet, 0, sizeof(*packet));
    packet->type = ICMP_ECHO; // 8 = echo request
    packet->un.echo.id = getpid();
}

int main(int ac, char **av) {
    if (ac == 1)
        return(fprintf(stderr, "ping: usage error: Destination address required\n"), -1);
    struct sockaddr_in sock;
    struct icmphdr packet;
    int ret = init_sockin(av, &sock);
    if (ret <= 0) {
        if (ret == 0)
            return(fprintf(stderr, "ping: invalid address: '%s'\n", av[1]), 1);
        else
            return(perror("inet_pton"), 1);
    }
    int sockfd; 
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        perror("socket");
    init_icmphdr(&packet);
    if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
        perror("sendto");

}