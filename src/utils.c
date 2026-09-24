#include <ft_ping.h>

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