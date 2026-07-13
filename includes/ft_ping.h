#include <stdio.h>
#include <unistd.h> //getpid
#include <sys/socket.h> //socket, sendto
#include <netinet/ip_icmp.h> //icmphdr strcut
#include <netinet/in.h> //sockaddr_in / in_addr
#include <string.h> //memset
#include <arpa/inet.h> //inet_pton
#include <time.h>

unsigned short checksum(void *b, int len);

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