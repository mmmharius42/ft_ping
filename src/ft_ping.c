#include <ft_ping.h>

volatile sig_atomic_t g_running = 1;

int g_sent = 0;
int g_received = 0;
double g_rtt_min = -1;
double g_rtt_max = 0;
double g_rtt_sum = 0;

void    handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

int     init_sockin(char **av, struct sockaddr_in *sock) {
    memset(sock, 0, sizeof(*sock));
    sock->sin_family = AF_INET;
    return (inet_pton(sock->sin_family, av[1], &sock->sin_addr));
}

void     init_icmphdr(struct icmphdr *packet, int seq) {
    memset(packet, 0, sizeof(*packet));
    packet->type = ICMP_ECHO; // 8 = echo request
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = seq;
    packet->checksum = 0;
    packet->checksum = checksum(packet, sizeof(*packet));
}

void    print_stats(char *dest) {
    printf("\n--- %s ping statistics ---\n", dest);
    double loss = g_sent ? (100.0 * (g_sent - g_received) / g_sent) : 0;
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n",
        g_sent, g_received, loss);
    if (g_received > 0) {
        double avg = g_rtt_sum / g_received;
        printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", g_rtt_min, avg, g_rtt_max);
    }
}

int     main(int ac, char **av) {
    if (ac == 1)
        return(fprintf(stderr, "ping: usage error: Destination address required\n"), -1);
    struct sockaddr_in sock;
    struct icmphdr packet;
    struct timespec start, end;
    int ret = init_sockin(av, &sock);
    if (ret <= 0) {
        if (ret == 0)
            return(fprintf(stderr, "ping: invalid address: '%s'\n", av[1]), 1);
        else
            return(perror("inet_pton"), 1);
    }
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        return(perror("socket"), 1);

    signal(SIGINT, handle_sigint);

    printf("PING %s (%s) 56(84) bytes of data.\n", av[1], av[1]);

    int seq = 0;
    while (g_running) {
        seq++;
        init_icmphdr(&packet, seq);
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
            perror("sendto");
        else
            g_sent++;
        char buf[1024];
        socklen_t addrlen = sizeof(sock);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&sock, &addrlen);
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (n < 0) {
            if (errno != EINTR)
                perror("recvfrom");
        } else {
            double rtt_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
            struct iphdr *iph = (struct iphdr *)buf;
            struct icmphdr *reply = (struct icmphdr *)(buf + (iph->ihl * 4));

            g_received++;
            if (g_rtt_min < 0 || rtt_ms < g_rtt_min)
                g_rtt_min = rtt_ms;
            if (rtt_ms > g_rtt_max)
                g_rtt_max = rtt_ms;
            g_rtt_sum += rtt_ms;

            printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                n - (iph->ihl * 4), av[1], reply->un.echo.sequence, iph->ttl, rtt_ms);
        }
        sleep(1);
    }
    print_stats(av[1]);
    close(sockfd);
    return (0);
}