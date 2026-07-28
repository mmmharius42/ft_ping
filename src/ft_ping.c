#include <ft_ping.h>
#define PACKET_SIZE 64
volatile sig_atomic_t g_running = 1;

int g_sent = 0, g_received = 0, _v = 0;
double g_rtt_min = -1, g_rtt_max = 0, g_rtt_sum = 0, g_rtt_sum2 = 0;
char *av = NULL;
char packet_buf[PACKET_SIZE];
char g_canonname[256] = {0};
int g_sockfd = -1;

void    handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

void    usage(void) {
    printf("Usage: ft_ping [-v] [-?] destination\n");
    printf("  -v  verbose output\n");
    printf("  -?  print help and exit\n");
}

int     init_sockin(struct sockaddr_in *sock) {
    struct addrinfo hints, *res;
    memset(sock, 0, sizeof(*sock));
    sock->sin_family = AF_INET;
    if (inet_pton(sock->sin_family, av, &sock->sin_addr) == 1)
        return (1);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_flags = AI_CANONNAME;
    int ret = getaddrinfo(av, NULL, &hints, &res);
    if (ret != 0)
        return (fprintf(stderr, "ft_ping: %s: %s\n", av, gai_strerror(ret)), 0);
    sock->sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    if (res->ai_canonname)
        strncpy(g_canonname, res->ai_canonname, sizeof(g_canonname) - 1);
    freeaddrinfo(res);
    return (1);
}

void     init_icmphdr(struct icmphdr *packet, int seq) {
    memset(packet, 0, PACKET_SIZE);
    packet->type = ICMP_ECHO; // 8 = echo request
    packet->un.echo.id = getpid();
    packet->un.echo.sequence = seq;
    packet->checksum = 0;
    packet->checksum = checksum(packet, PACKET_SIZE);
}

void    print_stats(char *dest, struct timespec *total_ms) {
    if (_v) {

    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double t_ms = (now.tv_sec - total_ms->tv_sec) * 1000 + (now.tv_nsec - total_ms->tv_nsec) / 1000000;
    printf("\n--- %s ping statistics ---\n", dest);
    double loss = g_sent ? (100.0 * (g_sent - g_received) / g_sent) : 0;
    printf("%d packets transmitted, %d received, %.0f%% packet loss, time %.0fms\n",
        g_sent, g_received, loss, t_ms);
    if (g_received > 0) {
        double avg = g_rtt_sum / g_received;
        double variance = (g_rtt_sum2 / g_received) - (avg * avg);
        double mdev = variance > 0 ? sqrt(variance) : 0;
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", g_rtt_min, avg, g_rtt_max, mdev);
    }
}

int     main(int ac, char **arv) {
    if (ac == 1)
        return(fprintf(stderr, "ping: usage error: Destination address required\n"), -1);
    for (int i = 1; i < ac; i++) {
        if (strcmp(arv[i], "-v") == 0)
            _v = 1;
        else if (strcmp(arv[i], "-?") == 0)
            return (usage(), 0);
        else
            av = arv[i];
    }
    if (av == NULL) {
        fprintf(stderr, "ping: usage error: Destination address required\n");
        return (1);
    }
    struct sockaddr_in sock;
    struct icmphdr *packet = (struct icmphdr *)packet_buf;
    struct timespec start, end, total_ms;
    int ret = init_sockin(&sock);
    if (ret <= 0) {
        if (ret == 0)
            return(fprintf(stderr, "ping: invalid address: '%s'\n", av), 1);
        else
            return(1);
    }
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
        return(perror("socket"), 1);
    g_sockfd = sockfd;

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sock.sin_addr, ip_str, sizeof(ip_str));

    if (_v) {
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), sock6.fd: -1 (socktype: 0), "
            "hints.ai_family: AF_INET\n", g_sockfd);
        if (g_canonname[0])
            printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n\n", g_canonname);
    }

    /* Reverse DNS resolution of the target, done once (matches inetutils'
       default behaviour outside of -n). Falls back silently to the IP. */
    char host_str[NI_MAXHOST];
    struct sockaddr_in rev = sock;
    if (getnameinfo((struct sockaddr *)&rev, sizeof(rev), host_str, sizeof(host_str),
            NULL, 0, NI_NAMEREQD) != 0)
        strncpy(host_str, ip_str, sizeof(host_str) - 1);

    printf("PING %s (%s) 56(84) bytes of data.\n", av, ip_str);

    int seq = 0;
    clock_gettime(CLOCK_MONOTONIC, &total_ms);
    while (g_running) {
        seq++;
        init_icmphdr(packet, seq);
        clock_gettime(CLOCK_MONOTONIC, &start);
        if (sendto(sockfd, packet_buf, PACKET_SIZE, 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
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

            if (reply->type != ICMP_ECHOREPLY) {
                printf("received unexpected ICMP type %d from %s\n", reply->type, av);
            }
            else {
                g_received++;
                if (g_rtt_min < 0 || rtt_ms < g_rtt_min)
                    g_rtt_min = rtt_ms;
                if (rtt_ms > g_rtt_max)
                    g_rtt_max = rtt_ms;
                g_rtt_sum += rtt_ms;
                g_rtt_sum2 += rtt_ms * rtt_ms;
                if (strcmp(host_str, ip_str) != 0)
                    printf("%zd bytes from %s (%s): icmp_seq=%d ident=%d ttl=%d time=%.3f ms\n",
                        n - (iph->ihl * 4), host_str, ip_str, reply->un.echo.sequence,
                        reply->un.echo.id, iph->ttl, rtt_ms);
                else
                    printf("%zd bytes from %s: icmp_seq=%d ident=%d ttl=%d time=%.3f ms\n",
                        n - (iph->ihl * 4), ip_str, reply->un.echo.sequence,
                        reply->un.echo.id, iph->ttl, rtt_ms);
            }
        }
        sleep(1);
    }
    print_stats(av, &total_ms);
    close(sockfd);
    return (0);
}