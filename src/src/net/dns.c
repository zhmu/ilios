/*
 * ILIOS DNS IPv4 stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles DNS queries, according to
 * RFC1035.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <net/dns.h>
#include <lib/lib.h>
#include <net/socket.h>
#include <md/console.h>
#include <netipv4/arp.h>
#include <netipv4/ipv4.h>
#include <netipv4/udp.h>

struct SOCKET* dns_socket;
int dns_query_pending = 0;

#define DNS_SERVER_ADDR 0xc0a80001

uint8_t dns_tempacket[2048];
uint16_t dns_tempkt_len;

/*
 * This will be called on incoming DNS answers.
 */
void
dns_callback (struct SOCKET* s, struct DEVICE* dev, uint32_t addr, void* data, uint32_t len) {
	struct DNS_HEADER* dhdr = (struct DNS_HEADER*)data;

	/* is the reply long enough? */
	if (len < sizeof (struct DNS_HEADER))
		/* no. bail out */
		return;

	/* is this a reply? */
	if (!(dhdr->opcode_flags & DNS_FLAG_RESPONSE))
		/* no. bail out */
		return;

	/* no more queries pending */
	dns_query_pending = 0;

	/* copy the packet over */
	kmemcpy (dns_tempacket, data, len);
	dns_tempkt_len = len;
}

/*
 * dns_send_query (uint32_t addr, uint8_t type, char* query)
 *
 * This will send query [query] of type [type] to [addr]. It will return
 * zero on failure or non-zero on success.
 *
 */
int
dns_send_query (uint32_t addr, uint16_t type, char* query) {
	uint8_t pkt[512], i;
	struct DNS_HEADER* dhdr = (struct DNS_HEADER*)pkt;
	uint8_t* ptr;
	char* ptr2;
	uint8_t len = kstrlen (query);

	/* build the DNS packet */
	dhdr->id = 0;
	dhdr->opcode_flags = DNS_FLAG_RD;
	dhdr->ra_z_rcode = 0;
	dhdr->qdcount = 0x0100;
	dhdr->ancount = 0;
	dhdr->nscount = 0;
	dhdr->arcount = 0;

	/* build the question */
	ptr = (uint8_t*)(pkt + sizeof (struct DNS_HEADER));
	/* qname */
	len = 0;
	while (*query) {
		ptr2 = kstrchr (query, '.');
		if (ptr2 == NULL)
			ptr2 = kstrchr (query, 0);
		i = ptr2 - query;
		*ptr++ = i; len++;
		if (!i) break;
		while (i--) {
			*ptr++ = *query++;
			len++;
		}
		if (*query)
			query++;
	}
	*ptr++ = 0; len++;
	/* qtype */
	*ptr++ = (type >> 8);
	*ptr++ = (type & 0xff);
	/* qclass */
	*ptr++ = (DNS_CLASS_IN >> 8);
	*ptr++ = (DNS_CLASS_IN & 0xff);

	/* query is probably pending */
	dns_query_pending = 1;

	/* transmit this */
	return udp_xmit_packet (dns_socket, addr, DNS_PORT, pkt, len + sizeof (struct DNS_HEADER) + 10);
}

/*
 * This will send a request for the address of [query]. It will return
 * zero on failure or the address on success.
 */
uint32_t
dns_query (char* query) {
	uint8_t* pkt;
	struct DNS_HEADER* dhdr = (struct DNS_HEADER*)dns_tempacket;
	uint8_t i, j, done;
	uint16_t count, type, rlen, class, w;
	uint8_t tmp_query[512];
	uint32_t addr;
	uint8_t* ptr;
	uint16_t qtype = DNS_QTYPE_HOST;

	/* copy the query */
	kstrcpy (tmp_query, query);

	/* keep querying, who knows what we'll find */
	while (1) {
		/* query for a direct host */
		kprintf ("dns: doing query for [%s]\n", tmp_query);
		if (!dns_send_query (DNS_SERVER_ADDR, qtype, tmp_query)) {
			/* this failed. bail out */
			dns_query_pending = 0;
			return 0;
		}

		/* we've done a query. wait until it is answered */
		while (dns_query_pending) {
			/* key pressed? */
			if (arch_console_peekch()) {
				/* yes. bail out */
				dns_query_pending = 0;
				return 0;
			}
		}

		/* build the new pointer */
		pkt = (uint8_t*)(dns_tempacket + sizeof (struct DNS_HEADER));

		/* got a failure? */
		if (dhdr->ra_z_rcode & 0xf)
			/* yes. bail out */
			return 0;

		/* got at least one response? */
		if (!dhdr->ancount)
			/* no. bail out */
			return 0;

		/* got a query? */
		if (dhdr->qdcount) {
			/* yes. walk past them */
			count = ((dhdr->qdcount >> 8) | ((dhdr->qdcount & 0xff) << 8));
			kprintf ("count %x -> ptr %x (%x)\n", count, pkt - dns_tempacket, *pkt);
			while (count--) {
				while (1) {
					i = *pkt++;
					if (!i) break;
					if (i & 0xc0) {
						pkt++;
						break;
					}
					pkt += i;
				}
				/* skip the rest, too */
				pkt += 6;
			}
		}

		count = ((dhdr->ancount >> 8) | ((dhdr->ancount & 0xff) << 8));
		kprintf ("count %x -> ptr %x\n", count, pkt - dns_tempacket);
		done = 0;
		while (count--) {
			/* we have the type now */
			type  = (*pkt++ << 8);
			type |=  *pkt++;
			/* we have the class now */
			class  = (*pkt++ << 8);
			class |=  *pkt++;
			/* skip the TTL (XXX) */
			pkt += 4;
			/* fetch the len */
			rlen  = (*pkt++ << 8);
			rlen |=  *pkt++;

			kprintf ("type %x\n", type);

			/* host answer? */
			if (type == DNS_QTYPE_HOST) {
				kprintf ("a\n");
				/* yes. fetch the IP */
				addr  = (*pkt++ << 24);
				addr |= (*pkt++ << 16);
				addr |= (*pkt++ <<  8);
				addr |= (*pkt++      );
				
				/* all done */
				return addr;
			} else if (type == DNS_QTYPE_CNAME) {
				/* cname. build the next query */
				j = 0; kprintf ("cname\n");
				ptr = pkt;
				while (1) {
					i = *ptr++;
					if (!i) break;
					if (j)
						tmp_query[j++] = '.';
					/* high bytes set? */
					if (i & 0xc0) {
						/* yes. copy previous data */
						w  = (i << 8);
						w |=  *ptr++;
						w &= 0x3fff;
						kprintf ("dns ptr %x\n", w);
						ptr = (uint8_t*)(dns_tempacket + w);
						i = *ptr++;
						while (i--)
							tmp_query[j++] = *ptr++;
					} else {
						while (i--)
							tmp_query[j++] = *ptr++;
					}
				}
				tmp_query[j++] = 0;

				done = 1; qtype = DNS_QTYPE_HOST;
			} else {
				/* ??? skip this */
				pkt += rlen;
			}

			/* done something? */
			if (!done)
				/* no. did a cname request? */
				if (qtype == DNS_QTYPE_CNAME)
					/* yes. bail out */
					return 0;

			/* were we doing a A type query? */
			if ((!done) && (qtype == DNS_QTYPE_HOST))
				/* yes. do a CNAME query now */
				qtype = DNS_QTYPE_CNAME;
		}
	}

	kprintf ("dns_callback(): unknown msg\n");
	return 0;
}

/*
 * dns_init()
 *
 * This will initialize the DNS client.
 *
 */
void
dns_init() {
	/* allocate a socket for DNS replies */
	dns_socket = socket_alloc (SOCKET_TYPE_UDP4);

	/* bind the socket to the DNS port */
	socket_bind (dns_socket, DNS_PORT);

	/* specify the correct callback function */
	socket_set_callback (dns_socket, dns_callback);
}

/* vim:set ts=2 sw=2: */
