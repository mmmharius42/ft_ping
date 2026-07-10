# ft_ping

Reimplementation of the `ping` command in C, using raw ICMP sockets.

## Goal

Recode a `ping` capable of sending ICMP Echo Request packets and displaying
the Echo Replies received, with output formatted similarly to the
`inetutils-2.0` implementation.

## Current status

- [x] Repo + Makefile setup
- [x] Handling the "no argument" case
- [x] Opening a `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`
- [ ] Building a `struct sockaddr_in` with the target IP
- [ ] Building a `struct icmphdr` (Echo Request)
- [ ] Computing the ICMP checksum
- [ ] `sendto()` / `recvfrom()`
- [ ] Parsing the response (IP header + ICMP header)
- [ ] `ping`-style output (RTT, seq, ttl...)
- [ ] `-v` and `-?` options
- [ ] Hostname (FQDN) resolution without DNS resolution on packet return

## Build

```bash
make
sudo ./ft_ping <ip>
```

!!! Requires root privileges (or the `CAP_NET_RAW` capability), since
`SOCK_RAW` is a privileged socket type.

## Technical notes

- Raw socket: `socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`
- Structures used: `struct icmphdr` (`<netinet/ip_icmp.h>`),
  `struct sockaddr_in` (`<netinet/in.h>`).

## Sources

- RFC 792 (ICMP)
- `man 7 icmp`
- `man 7 raw`
- https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html