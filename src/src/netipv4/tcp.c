/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles TCP.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/network.h>
#include <lib/lib.h>
#include <netipv4/arp.h>
#include <netipv4/cksum.h>
#include <netipv4/icmp.h>
#include <netipv4/ip.h>
#include <netipv4/ipv4.h>
#include <netipv4/icmp.h>
#include <netipv4/route.h>
#include <netipv4/tcp.h>

/*
 * This will calculate the TCP checksum of TCP packet with header [tcphdr]
 * and IP header [iphdr] spanning a total of [len] bytes.
 *
 * This is taken from tcpdump.
 */
uint32_t
tcp_cksum (uint8_t* tcphdr, uint32_t source, uint32_t dest, uint32_t len) {
	union phu {
		struct phdr {
			uint32_t src;
			uint32_t dst;
			uint8_t  mbz;
			uint8_t  proto;
			uint16_t len;
		} ph;
		uint16_t pa[6];
	} phu;
	uint16_t* sp;

	/* build the pseudo-header */
	phu.ph.len = htons (len);
	phu.ph.mbz = 0;
	phu.ph.proto = IP_PROTO_TCP;
	phu.ph.src = htonl (source);
	phu.ph.dst = htonl (dest);

	sp = &phu.pa[0];
	return ipv4_cksum ((void*)tcphdr, len, sp[0] + sp[1] + sp[2] + sp[3] + sp[4] + sp[5]);
}

/*
 * This will handle TCP SYN packet [np]. It will return
 * zero on failure or non-zero on success.
 */
int
tcp_handle_syn (struct SOCKET* s, struct NETPACKET* np) {
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);
	struct TCP_HEADER* tcphdr = (struct TCP_HEADER*)(np->data + sizeof (struct IP_HEADER));
	struct TCP_SOCKINFO* si = (struct TCP_SOCKINFO*)&s->sockdata;
	uint32_t dest = ipv4_conv_addr (iphdr->source);
	char buf[512];
	struct TCP_HEADER* dtcphdr = (struct TCP_HEADER*)buf;
	char* ptr = (buf + sizeof (struct TCP_HEADER));

	/* are we in LISTEN-ing mode or SYN received mode? */
	if ((si->state == TCP_STATE_LISTEN) || (si->state == TCP_STATE_SYN_RECV)) {
		/* yes. we've gotten the initial sequence of the host now */
		si->his_seqno = tcphdr->seqno;
		si->my_seqno = 100;

		/* build the response */
		dtcphdr->source = htons (tcphdr->dest);
		dtcphdr->dest   = htons (tcphdr->source);
		dtcphdr->seqno  = htonl (si->my_seqno);
		dtcphdr->ackno  = htonl (tcphdr->seqno + 1);
		dtcphdr->offs 	= ((sizeof (struct TCP_HEADER) + 8) / 4) << 4;
		dtcphdr->flags	= (TCP_BIT_SYN | TCP_BIT_ACK);
		dtcphdr->window = htons (ntohs (tcphdr->window));
		dtcphdr->urg    = 0;

		/* add an MSS option */
		*ptr++ = 0x02;
		*ptr++ = 0x04;
		*ptr++ = (1460 >>   8);
		*ptr++ = (1460 & 0xff);
		*ptr++ = 0x00; *ptr++ = 0x00;
		*ptr++ = 0x00; *ptr++ = 0x00;
		*ptr++ = 0x00; *ptr++ = 0x00;

		/* calculate the checksum */
		dtcphdr->checksum = 0;
		dtcphdr->checksum = tcp_cksum (buf,
				ipv4_conv_addr (iphdr->source),
				ipv4_conv_addr (iphdr->dest),
				sizeof (struct TCP_HEADER) + 8);

		/* off you go */
		kprintf ("tcp_handle_syn(): got SYN of %I:%u\n", ipv4_conv_addr (iphdr->source), tcphdr->source);
		kprintf ("seq = %u, ack = %u\n", si->his_seqno, tcphdr->ackno);

		/* build the IP packet */
		if (!ip_transmit_packet (IP_PROTO_TCP, dest, sizeof (struct TCP_HEADER) + 8, buf))
			/* this failed. bail out */
			return 0;

		/* next state */
		si->state = TCP_STATE_SYN_RECV;
		return 0;
	}

	return 0;
}

/*
 * This will handle incoming TCP packet [np]. It will return
 * zero on failure or non-zero on success.
 */
int
tcp_handle_packet (struct NETPACKET* np) {
	struct IP_HEADER* iphdr = (struct IP_HEADER*)(np->data);
	struct TCP_HEADER* tcphdr = (struct TCP_HEADER*)(np->data + sizeof (struct IP_HEADER));
	uint16_t dstport = (tcphdr->dest >> 8) | ((tcphdr->dest & 0xff) << 8);
	struct SOCKET* s = socket_find (SOCKET_TYPE_TCP4, dstport);
	uint32_t addr = ipv4_conv_addr (iphdr->source);

	/* do we have a socket bound to this? */
	if (s == NULL) {
		/* no. send ICMP unreachable port message */
		icmp_send_unreachable (addr, ICMP_CODE_PORTUNREACHABLE, (uint8_t*)iphdr, sizeof (struct IP_HEADER), (uint8_t*)tcphdr, 8);
		return 0;
	}

	/* convert sequence numbers */
	tcphdr->seqno = ntohl (tcphdr->seqno);
	tcphdr->ackno = ntohl (tcphdr->ackno);

	/* convert port numbers */
	tcphdr->source = ntohs (tcphdr->source);
	tcphdr->dest   = ntohs (tcphdr->dest);

	/* got a SYN packet? */
	if (tcphdr->flags & TCP_BIT_SYN)
		/* yes. deal with that */
		return tcp_handle_syn (s, np);

	/* got an ACK? */
	if (tcphdr->flags & TCP_BIT_ACK) {
		/* yep. */
	}

	return 0;
}

/*
 * This will initialize socket [s] for TCP use.
 */
void
tcp_init_socket (struct SOCKET* s) {
	struct TCP_SOCKINFO* si = (struct TCP_SOCKINFO*)&s->sockdata;

	/* clear the sockinfo out */
	kmemset (si, 0, sizeof (struct TCP_SOCKINFO));
}

/* vim:set ts=2 sw=2 tw=78: */
