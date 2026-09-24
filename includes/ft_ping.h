#include <stdio.h>
#include <unistd.h> //getpid
#include <sys/socket.h> //socket / sendto
#include <netinet/ip_icmp.h> //icmphdr strcut
#include <netinet/in.h> //sockaddr_in / in_addr
#include <string.h> //memset
#include <arpa/inet.h> //inet_pton / inet_ntop
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h> //sqrt
#include <netdb.h> //getaddrinfo / freeaddrinfo / gai_strerror 

#define PACKET_SIZE 64
#define DATA_LEN    56

extern volatile sig_atomic_t    g_running;
extern char                     *av;
extern size_t                   g_sent, g_received;
extern double                   g_rtt_min, g_rtt_max, g_rtt_sum, g_rtt_sum2;
extern unsigned short           g_ident;
extern char                     g_ip_str[INET_ADDRSTRLEN];

unsigned short  checksum(void *b, int len);
int             init_sockin(struct sockaddr_in *sock);
void            init_icmphdr(struct icmphdr *packet, int seq);
void            handle_sigint(int sig);
void            usage(void);
const char      *icmp_code_str(int type, int code);
void            print_stats(void);
double          elapsed_ms(struct timespec *a, struct timespec *b);
// struct sockaddr_in {
//     short            sin_family;   // e.g. AF_INET
//     unsigned short   sin_port;     // e.g. htons(3490)
//     struct in_addr   sin_addr;     // see struct in_addr, below
//     char             sin_zero[8];  // zero this if you want to
// };

// struct in_addr {
//     unsigned long s_addr;  // load with inet_aton()
// };

// struct icmphdr {
//   u_int8_t type;
//   u_int8_t code;
//   u_int16_t checksum;
//   union {
//     struct {
//       u_int16_t id;
//       u_int16_t sequence;
//     } echo;
//     u_int32_t gateway;
//     struct {
//       u_int16_t __unused;
//       u_int16_t mtu;
//     } frag;
//   } un;
// };