#include <ft_ping.h>

unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0; 
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) // read buffer 16 bits at a time
        sum += *buf++;

    if (len == 1) //if impair add oct at the end 
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF); // fold the carry: add whatever overflowed past 16bits back into the lower 16bits
    sum += (sum >> 16); // do again -> can re overflow in specific case..

    result = ~sum; //protocol need to inverting the result bit
    return (result);
}