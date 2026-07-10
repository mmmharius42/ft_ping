#include <ft_ping.h>

int main(int ac, char **av) {
    (void)av;
    if (ac == 1)
        return(printf("ping: usage error: Destination address required\n"), -1);
    int sockfd; 
    if ((sockfd = socket(AF_INET, SOCK_RAW, 1)) < 0)
        perror("socket");
}