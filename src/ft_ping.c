#include <ft_ping.h>

#define PACKET_SIZE 64
#define DATA_LEN    56

volatile sig_atomic_t g_running = 1;

int                 _v = 0;
char                *av = NULL;
int                 g_sockfd = -1;
unsigned short      g_ident = 0;
size_t              g_sent = 0, g_received = 0;
double              g_rtt_min = 0, g_rtt_max = 0, g_rtt_sum = 0, g_rtt_sum2 = 0;
char                packet_buf[PACKET_SIZE];
char                g_ip_str[INET_ADDRSTRLEN];

void    handle_sigint(int sig) {
    (void)sig;
    g_running = 0;
}

void    usage(void) {
    printf("Usage: ft_ping [OPTION...] HOST ...\n");
    printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
    printf("\n");
    printf("  -v                         verbose output\n");
    printf("  -?, --help                 give this help list\n");
}
double  elapsed_ms(struct timespec *a, struct timespec *b) {
    return ((b->tv_sec - a->tv_sec) * 1000.0 + (b->tv_nsec - a->tv_nsec) / 1000000.0);
}

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

const char  *icmp_code_str(int type, int code) {
    if (type == ICMP_DEST_UNREACH) {
        if (code == ICMP_NET_UNREACH)
            return ("Destination Net Unreachable");
        if (code == ICMP_HOST_UNREACH)
            return ("Destination Host Unreachable");
        if (code == ICMP_PROT_UNREACH)
            return ("Destination Protocol Unreachable");
        if (code == ICMP_PORT_UNREACH)
            return ("Destination Port Unreachable");
        return ("Destination Unreachable");
    }
    if (type == ICMP_TIME_EXCEEDED) {
        if (code == ICMP_EXC_TTL)
            return ("Time to live exceeded");
        return ("Time exceeded");
    }
    if (type == ICMP_SOURCE_QUENCH)
        return ("Source Quench");
    if (type == ICMP_REDIRECT)
        return ("Redirect");
    if (type == ICMP_PARAMETERPROB)
        return ("Parameter problem");
    return ("Unknown ICMP type");
}

int     recv_reply(void) {
    char                buf[1024];
    char                from_str[INET_ADDRSTRLEN];
    struct sockaddr_in  from;
    socklen_t           fromlen = sizeof(from);
    struct timespec     now, sent;
    struct iphdr        *iph;
    struct icmphdr      *reply;
    ssize_t             n;
    int                 hlen;
    double              rtt;

    n = recvfrom(g_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return (0);
        return (perror("ft_ping: recvfrom"), 0);
    }

    iph = (struct iphdr *)buf;
    hlen = iph->ihl * 4;
    if (n < hlen + (ssize_t)sizeof(struct icmphdr))
        return (1);
    reply = (struct icmphdr *)(buf + hlen);
    inet_ntop(AF_INET, &iph->saddr, from_str, sizeof(from_str));

    if (reply->type != ICMP_ECHOREPLY) {
        if (_v)
            printf("%zd bytes from %s: %s\n", n - hlen, from_str,
                icmp_code_str(reply->type, reply->code));
        return (1);
    }
    if (ntohs(reply->un.echo.id) != g_ident)
        return (1);
    if (n < hlen + (ssize_t)sizeof(struct icmphdr) + (ssize_t)sizeof(sent))
        return (1);

    memcpy(&sent, buf + hlen + sizeof(struct icmphdr), sizeof(sent));
    rtt = elapsed_ms(&sent, &now);

    g_received++;
    if (g_received == 1 || rtt < g_rtt_min)
        g_rtt_min = rtt;
    if (rtt > g_rtt_max)
        g_rtt_max = rtt;
    g_rtt_sum += rtt;
    g_rtt_sum2 += rtt * rtt;

    printf("%zd bytes from %s: icmp_seq=%u ttl=%d time=%.3f ms\n",
        n - hlen, from_str, ntohs(reply->un.echo.sequence), iph->ttl, rtt);
    return (1);
}

void    print_stats(void) {
    double  avg, vari;

    printf("--- %s ping statistics ---\n", av);
    printf("%zu packets transmitted, %zu packets received", g_sent, g_received);
    if (g_sent)
        printf(", %d%% packet loss",
            (int)(((g_sent - g_received) * 100) / g_sent));
    printf("\n");
    if (g_received) {
        avg = g_rtt_sum / g_received;
        vari = (g_rtt_sum2 / g_received) - (avg * avg);
        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
            g_rtt_min, avg, g_rtt_max, vari > 0 ? sqrt(vari) : 0.0);
    }
}

int     main(int ac, char **arv) {
    struct sockaddr_in  dest;
    struct icmphdr      *packet = (struct icmphdr *)packet_buf;
    struct sigaction    sa;
    struct timespec     send_time, now;
    struct timeval      tv;
    double              left;
    int                 seq;
    int                 i;

    for (i = 1; i < ac; i++) {
        if (strcmp(arv[i], "-v") == 0)
            _v = 1;
        else if (strcmp(arv[i], "-?") == 0 || strcmp(arv[i], "--help") == 0)
            return (usage(), 0);
        else if (arv[i][0] == '-' && arv[i][1])
            return (fprintf(stderr, "ft_ping: unrecognized option '%s'\n"
                "Try 'ft_ping --help' for more information.\n", arv[i]), 1);
        else
            av = arv[i];
    }
    if (av == NULL)
        return (fprintf(stderr, "ft_ping: missing host operand\n"
            "Try 'ft_ping --help' for more information.\n"), 1);

    g_ident = getpid() & 0xffff;

    if ((g_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        if (errno == EPERM || errno == EACCES)
            fprintf(stderr, "ft_ping: Lacking privilege for icmp socket.\n");
        else
            perror("ft_ping: socket");
        return (1);
    }

    if (!init_sockin(&dest))
        return (close(g_sockfd), 1);

    inet_ntop(AF_INET, &dest.sin_addr, g_ip_str, sizeof(g_ip_str));

    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    printf("PING %s (%s): %d data bytes", av, g_ip_str, DATA_LEN);
    if (_v)
        printf(", id 0x%04x = %u", g_ident, g_ident);
    printf("\n");

    seq = 0;
    while (g_running) {
        init_icmphdr(packet, seq);
        clock_gettime(CLOCK_MONOTONIC, &send_time);
        if (sendto(g_sockfd, packet_buf, PACKET_SIZE, 0,
                (struct sockaddr *)&dest, sizeof(dest)) < 0)
            perror("ft_ping: sendto");
        else
            g_sent++;

        while (g_running) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            left = 1000.0 - elapsed_ms(&send_time, &now);
            if (left <= 0.001)
                break;
            tv.tv_sec = (time_t)(left / 1000.0);
            tv.tv_usec = (long)((left - tv.tv_sec * 1000.0) * 1000.0);
            if (tv.tv_sec == 0 && tv.tv_usec == 0)
                tv.tv_usec = 1;
            setsockopt(g_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (!recv_reply())
                break;
        }
        seq++;
    }
    print_stats();
    close(g_sockfd);
    return (g_received == 0);
}