/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the TCP include file.
 *
 */
#include <sys/types.h>
#include <sys/network.h>
#include <net/socket.h>

#ifndef __TCP_H__
#define __TCP_H__

/* TCP_BIT_xxx are the TCP control bits */
#define TCP_BIT_URG	0x20
#define TCP_BIT_ACK	0x10
#define TCP_BIT_PSH	0x08
#define TCP_BIT_RST	0x04
#define TCP_BIT_SYN	0x02
#define TCP_BIT_FIN	0x01

/* based on RFC 793 */
struct TCP_HEADER {
	uint16_t	source;
	uint16_t	dest;
	uint32_t	seqno;
	uint32_t	ackno;
	uint8_t		offs;
	uint8_t		flags;
	uint16_t	window;
	uint16_t	checksum;
	uint16_t	urg;
} __attribute__((packed));

#define TCP_STATE_LISTEN	0
#define TCP_STATE_SYN_SENT	1
#define TCP_STATE_SYN_RECV	2
#define TCP_STATE_ESTABLISHED	3
#define TCP_STATE_FINWAIT_1	4
#define TCP_STATE_FINWAIT_2	5
#define TCP_STATE_CLOSE_WAIT	6
#define TCP_STATE_CLOSING	7
#define TCP_STATE_LASTACK	8
#define TCP_STATE_TIMEWAIT	9
#define TCP_STATE_CLOSED	10

struct TCP_SOCKINFO {
	uint32_t	state;

	uint32_t	my_seqno;
	uint32_t	his_seqno;
};

int tcp_handle_packet (struct NETPACKET* np);
void tcp_init_socket (struct SOCKET* s);

#endif /* __TCP_H__ */
