/*
 * dhcp.h - ILIOS DHCP IPv4 stack
 * (c) 2003 Rink Springer, BSD licensed
 *
 */
#include <sys/types.h>

#ifndef __DHCP_H__
#define __DHCP_H__

#define DHCP_PORT_SERVER 67
#define DHCP_PORT_CLIENT 68

#define DHCP_MAX_ROUTERS     (256 / 4)
#define DHCP_MAX_DNSSERVERS  (256 / 4)
#define DHCP_MAX_HOSTNAMELEN 255

#define DHCP_OP_BOOTREQUEST	1
#define DHCP_OP_BOOTREPLY   2	

#define DHCP_OP_END          255
#define DHCP_OP_PAD          0
#define DHCP_OP_SUBNET       1
#define DHCP_OP_TIMEOFFSET	 2
#define DHCP_OP_ROUTER       3
#define DHCP_OP_TIMESERVER   4
#define DHCP_OP_NAMESERVER   5
#define DHCP_OP_DNSSERVER    6
#define DHCP_OP_LOGSERVER    7
#define DHCP_OP_COOKIESERVER 8
#define DHCP_OP_LPRSERVER    9
#define DHCP_OP_IMPRESSERVER 10
#define DHCP_OP_RESLOCSERVER 11
#define DHCP_OP_HOSTNAME     12
#define DHCP_OP_BOOTFILESIZE 13
#define DHCP_OP_MERITDUMPFILE 14
#define DHCP_OP_DOMAINNAME   15
#define DHCP_OP_SWAPSERVER   16
#define DHCP_OP_ROOTPATH     17
#define DHCP_OP_EXTPATH      18
#define DHCP_OP_IPFORWARDING 19
#define DHCP_OP_SOURCEROUTE	 20
#define DHCP_OP_POLICYFILTER 21
#define DHCP_OP_MAX_DGRAM    22
#define DHCP_OP_DEFAULT_TTL  23
#define DHCP_OP_MTU_TIMEOUT  24
#define DHCP_OP_MTU_TABLE    25
#define DHCP_OP_MTU          26
#define DHCP_OP_LOCALSUBNETS 27
#define DHCP_OP_BCAST_ADDR   28
#define DHCP_OP_MASKDISCOVER 29
#define DHCP_OP_MASKSUPPLIER 30
#define DHCP_OP_ROUTERDISCOVER 31
#define DHCP_OP_ROUTERSOL    32
#define DHCP_OP_STATICROUTE  33
#define DHCP_OP_TRAILENC     34
#define DHCP_OP_ARP_TIMEOUT  35
#define DHCP_OP_ETHER_ENC    36
#define DHCP_OP_TCPDEFAULTTTL 37
#define DHCP_OP_KEEPALIVEINT 38
#define DHCP_OP_KEEPALIVEGARBAGE 39
#define DHCP_OP_NISDOMAIN    40
#define DHCP_OP_NISSERVERS   41
#define DHCP_OP_NTPSERVERS   42
#define DHCP_OP_VENDORSPECIFIC 43
#define DHCP_OP_REQUESTEDADDR	50

struct DHCP_PACKET {
	uint8_t  op;
	uint8_t  htype;
	uint8_t  hlen;
	uint8_t  hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint8_t  ciaddr[4];
	uint8_t  yiaddr[4];
	uint8_t  siaddr[4];
	uint32_t giaddr;
	uint8_t  chaddr[16];
	uint8_t  sname[64];
	uint8_t	 file[128];
} __attribute__((packed));

void dhcp_init();
int dhcp_discover (struct DEVICE* dev);

#endif /* __DHCP_H__ */

/* vim:set ts=2 sw=2: */
