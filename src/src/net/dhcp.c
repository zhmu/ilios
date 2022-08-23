/*
 * ILIOS DHCP IPv4 stack
 * (c) 2003 Rink Springer
 *
 * This is the file which handles DHCP queries, according to
 * RFC2131. The options are interprinted as of RFC2132.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <net/dhcp.h>
#include <lib/lib.h>
#include <net/socket.h>
#include <netipv4/arp.h>
#include <netipv4/ipv4.h>
#include <netipv4/udp.h>

struct SOCKET* dhcp_socket;

/*
 * This will handle up to [len] bytes of [dh].
 */
int
dhcp_handle_options (struct DEVICE* dev, struct DHCP_PACKET* dh, uint32_t len) {
	uint8_t* opt = (uint8_t*)(dh + sizeof (struct DHCP_PACKET));
	uint8_t* orgopt = opt;
	uint32_t subnet = 0xFFFFFF00; /* ??? */
	uint32_t routers[DHCP_MAX_ROUTERS];
	uint32_t dnsservers[DHCP_MAX_DNSSERVERS];
	uint8_t  hostname[DHCP_MAX_HOSTNAMELEN];
	uint8_t  domainname[DHCP_MAX_HOSTNAMELEN];
	uint8_t num_routers = 0;
	uint8_t num_dnsservers = 0;
	uint8_t i, done = 0;
	uint32_t my_addr = ipv4_conv_addr (dh->yiaddr);

	/* reset all options */
	kstrcpy (hostname, "");
	kstrcpy (domainname, "");

	/* wade through all options */
	while (!done) {
		/* about to exceed the buffer? */
		if ((orgopt + len) > opt)
			/* yes. bail out */
			break;

		/* fetch the DHCP option */
		switch (*opt++) {
			  case DHCP_OP_PAD: /* pad */
			  	                break;
			  case DHCP_OP_END: /* all done */
			  	                done = 1;
													break;
		 case DHCP_OP_SUBNET: /* subnet */
													if (*opt++ != 4)
														/* invalid length. discard the packet */
														return 0;
													subnet = ipv4_conv_addr (opt);
													opt += 4;
													break;
     case DHCP_OP_ROUTER: /* routers */
                          i = *opt++;
													if (i % 4)
														/* this violates the RFC, bail out */
														return 0;

													/* copy the routers */
													for (; i != 0; i -= 4) {
                          	routers[num_routers++] = ipv4_conv_addr (opt);
														opt += 4;
													}
													break;
  case DHCP_OP_DNSSERVER: /* dns servers */
                          i = *opt++;
													if (i % 4)
														/* this violates the RFC, bail out */
														return 0;

													/* copy the dnsservers */
													for (; i != 0; i -= 4) {
                          	dnsservers[num_dnsservers++] = ipv4_conv_addr (opt);
														opt += 4;
													}
													break;
   case DHCP_OP_HOSTNAME: /* hostname */
													i = *opt++;
													kmemcpy (hostname, opt, i);
													hostname[i] = 0;
													opt += i;
													break;
 case DHCP_OP_DOMAINNAME: /* domain name */
													i = *opt++;
													kmemcpy (domainname, opt, i);
													domainname[i] = 0;
													opt += i;
													break;
                 default: /* what's this? skip it */
													i = *opt++;
													opt += i;
													break;

		}
	}

	/* got an address? */
	if (!my_addr)
		/* no. discard the reply */
		return 0;

	/* be verbose */
	kprintf ("%s: DHCPACK from %I: bound to %I netmask %I\n", dev->name, ipv4_conv_addr (dh->siaddr), my_addr, subnet);
	if (hostname[0])
		kprintf ("%s: I am '%s'\n", dev->name, hostname);
	if (domainname[0])
		kprintf ("%s: My domainname is '%s'\n", dev->name, domainname);
	if (num_routers) {
		kprintf ("%s: got routers: ", dev->name);
		for (i = 0; i < num_routers; i++) {
			if (!i) { kprintf (", "); }
			kprintf ("%I", routers[i]);
		}
		kprintf ("\n");
	}
	if (num_dnsservers) {
		kprintf ("%s: got DNS servers: ", dev->name);
		for (i = 0; i < num_dnsservers; i++) {
			if (!i) { kprintf (", "); }
			kprintf ("%I", dnsservers[i]);
		}
		kprintf ("\n");
	}

	/* purge all interface settings */
	ipv4_purge_device (dev);

	/* allocate the new address */
	ipv4_add_address (dev, my_addr, subnet);

	/* victory */
	return 1;
}

/*
 * This will send a DHCP request packet for server [dh] over [dev].
 */
int
dhcp_request (struct DEVICE* dev, struct DHCP_PACKET* dh) {
	uint8_t pkt[1024];
	struct DHCP_PACKET* dpkt = (struct DHCP_PACKET*)pkt;
	uint8_t* opt = (pkt + sizeof (struct DHCP_PACKET));
	uint8_t hw_bcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	/* fill the structure out */
	kmemset (pkt, 0, sizeof (pkt));
	dpkt->op = DHCP_OP_BOOTREQUEST;
	dpkt->htype = ARP_HWTYPE_ETH;
	dpkt->hlen = ETHER_ADDR_LEN;
	dpkt->xid = 2;
	kmemcpy (dpkt->chaddr, dev->ether.hw_addr, ETHER_ADDR_LEN);

	/* append the requested IP address option */
	*opt++ = DHCP_OP_REQUESTEDADDR;
	*opt++ = 4;
	kmemcpy (opt, dh->yiaddr, 4);
	*opt = DHCP_OP_END;

	/* send it */
	return udp_xmit_packet_ex (dev, hw_bcast, 0, 0xFFFFFFFF, DHCP_PORT_CLIENT, DHCP_PORT_SERVER, pkt, sizeof (struct DHCP_PACKET) + 8);
}

/*
 * This will be called on incoming DHCP answers.
 */
void
dhcp_callback (struct SOCKET* s, struct DEVICE* dev, uint32_t addr, void* data, uint32_t len) {
	struct DHCP_PACKET* dh = (struct DHCP_PACKET*)data;

	/* is the packet is too short, silently discard it */
	if (len < sizeof (struct DHCP_PACKET))
		return;

	/* is this a reply? */
	if (dh->op != DHCP_OP_BOOTREPLY)
		/* no. discard it */
		return;

	/* sanity checks to ensure we're talking the same hardware */
	if ((dh->htype != ARP_HWTYPE_ETH) ||
		  (dh->hlen  != ETHER_ADDR_LEN))
			/* no. discard the packet */
			return;

	/*
	 * possible scenarios:
	 *
	 * 1) we just sent an DHCPDISCOVER and we receive a DHCPOFFER
	 *    in this case, dh->xid = 1
	 * 2) we replied a DHCPREQUEST to a DHCPOFFER and we get a DHCPACK
	 *    in this case, dh->xid != 1
	 * 3) we replied a DHCPREQUEST to a DHCPOFFER and we get a DHCPNAK
	 *    in this case, dh->xid != 1
	 *
	 */

	/* got a DHCPOFFER? */
	if (dh->xid == 1) {
		/* yes. seems fine, take it */
		dhcp_request (dev, dh);
		return;
	} else {
		/* no. handle the options */
		if (!dhcp_handle_options (dev, (struct DHCP_PACKET*)data, len))
			/* this failed. discard the packet */
			return;
	}
}

/*
 * This will initialize the DHCP client.
 */
void
dhcp_init() {
	/* allocate a socket for DHCP replies */
	dhcp_socket = socket_alloc (SOCKET_TYPE_UDP4);

	/* bind the socket to the DHCP reply port */
	socket_bind (dhcp_socket, DHCP_PORT_CLIENT);

	/* specify the correct callback function */
	socket_set_callback (dhcp_socket, dhcp_callback);
}

/*
 * This will send a DHCP discover packet over [dev].
 */
int
dhcp_discover (struct DEVICE* dev) {
	struct DHCP_PACKET dpkt;
	uint8_t hw_bcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	/* fill the structure out */
	kmemset (&dpkt, 0, sizeof (struct DHCP_PACKET));
	dpkt.op = DHCP_OP_BOOTREQUEST;
	dpkt.htype = ARP_HWTYPE_ETH;
	dpkt.hlen = ETHER_ADDR_LEN;
	dpkt.xid = 1;
	kmemcpy (&dpkt.chaddr, dev->ether.hw_addr, ETHER_ADDR_LEN);

	/* send it */
	return udp_xmit_packet_ex (dev, hw_bcast, 0, 0xFFFFFFFF, DHCP_PORT_CLIENT, DHCP_PORT_SERVER, (void*)&dpkt, sizeof (struct DHCP_PACKET));
}

/* vim:set ts=2 sw=2: */
