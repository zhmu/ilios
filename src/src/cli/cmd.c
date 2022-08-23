/*
 * cmd.c - ILIOS Command Parser
 *
 * This will handle commands typed at the console.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/irq.h>
#include <sys/kmalloc.h>
#include <sys/network.h>
#include <lib/lib.h>
#include <net/dns.h>
#include <net/socket.h>
#include <netipv4/ipv4.h>
#include <netipv4/arp.h>
#include <netipv4/route.h>
#include <netipv4/udp.h>
#include <md/console.h>
#include <md/reboot.h>
#include <net/dhcp.h>
#include <assert.h>
#include <config.h>
#include <cli/cli.h>
#include "../version.h"

/* make the parameter handling readable */
#define ARG_INTERFACE(x) args->arg[(x)].value.dev
#define ARG_DRIVER(x)    args->arg[(x)].value.driver
#define ARG_INTEGER(x)   args->arg[(x)].value.integer
#define ARG_IPV4ADDR(x)  args->arg[(x)].value.ipv4addr
#define ARG_STRING(x)    args->arg[(x)].value.string
#define ARG_BOOLEAN(x)   args->arg[(x)].value.boolean

/* Create a new interface */
int
cmd_int_create (struct CLI_ARGS* args) {
	struct DEVICE_RESOURCES res;
	struct NIC_DRIVER* drv;

	/* safety first */
	ASSERT (args->num_args >= 2);

	/* ensure the interface name is unique */
	if (device_find (ARG_STRING(0)) != NULL) {
		/* it is not. complain */
		kprintf ("duplicate interface [%s]\n", ARG_STRING(0));
		return 0;
	}

	/* set them up */
	kmemset (&res, 0, sizeof (struct DEVICE_RESOURCES));
	if (args->num_args >= 3) res.port = ARG_INTEGER(2);
	if (args->num_args >= 4) res.irq  = ARG_INTEGER(3);

	/* initialize the device */
	drv = ARG_DRIVER(1);
	if (!drv->init (ARG_STRING(0), &res)) {
		/* this failed. complain */
		kprintf ("device failed to initialize\n");
		return 0;
	}

	/* all done */
	return 1;
}

/* Destroy an interface */
int
cmd_int_destroy (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 1);

	/* free the associated IRQ */
	irq_unregister (ARG_INTERFACE(0));

	/* zap the device */
	device_unregister (ARG_INTERFACE(0));

	/* all done */
	return 1;
}

/* Bind an interface */
int
cmd_int_bind (struct CLI_ARGS* args) {
	uint32_t netmask;
	uint32_t addr;

	/* safety first */
	ASSERT (args->num_args >= 2);
	addr = ARG_IPV4ADDR(1);

	/* is a netmask given? */
	if (args->num_args == 3)
		/* yes. extract it */
		netmask = ARG_INTEGER(2);
	else
		/* no. guess it */
		netmask = ipv4_guess_netmask (addr);

	/* add the address */
	if (!ipv4_add_address (ARG_INTERFACE(0), addr, netmask)) {
		kprintf ("cannot add address\n");
		return 0;
	}

	/* all done */
	return 1;
}

/* Unbind an interface */
int
cmd_int_unbind (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 2);

	/* remove the address */
	if (!ipv4_remove_address (ARG_INTERFACE(0), ARG_INTEGER(1))) {
		kprintf ("interface not bound to this address\n");
		return 0;
	}

	/* all done */
	return 1;
}

/* Reboot */
int
cmd_reboot (struct CLI_ARGS* args) {
	arch_reboot();

	/* NOTREACHED */
	return 0;
}

/* Set hostname */
int
cmd_set_hostname (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 1);

	/* hostname too long? */
	if (kstrlen (ARG_STRING(0)) > HOSTNAME_LEN) {
		/* yes. complain */
		kprintf ("supplied hostname too long\n");
		return 0;
	}

	/* update it */
	kstrcpy (hostname, ARG_STRING(0));
	return 1;
}

/* Interface status */
int
cmd_int_status (struct CLI_ARGS* args) {
	struct DEVICE* dev = coredevice;
	uint32_t i;

	/* walk through all devices */
	while (dev) {
		/* display the name */
		kprintf ("%s: port %x irq %x\n",
			dev->name,
			dev->resources.port,
			dev->resources.irq);

		/* do we have an interesting (eg. not 00:00:00:00:00:00) mac address? */
		if (!((dev->ether.hw_addr[0] == 0) && (dev->ether.hw_addr[1] == 0) &&
		    (dev->ether.hw_addr[2] == 0) && (dev->ether.hw_addr[3] == 0) &&
			  (dev->ether.hw_addr[4] == 0) && (dev->ether.hw_addr[5] == 0))) {
			/* yes. display it */
			kprintf ("    mac address %x:%x:%x:%x:%x:%x\n",
				dev->ether.hw_addr[0], dev->ether.hw_addr[1], dev->ether.hw_addr[2], 
				dev->ether.hw_addr[3], dev->ether.hw_addr[4], dev->ether.hw_addr[5]);
		}

		/* show the IP address */
		for (i = 0; i < IPV4_MAX_ADDR; i++)
			/* got a valid IP address here? */
			if ((dev->ipv4conf.address[i].addr)) 
				/* yes. display the address */
				kprintf ("    ipv4 address %I netmask %I\n",
					dev->ipv4conf.address[i].addr,
					dev->ipv4conf.address[i].netmask);

		/* show rx and tx */
		kprintf ("    received: %lu frames, %lu bytes\n", dev->rx_frames, dev->rx_bytes);
		kprintf ("    transmitted: %lu frames, %lu bytes\n", dev->tx_frames, dev->tx_bytes);

		/* next */
		dev = dev->next;
	}

	/* all done */
	return 1;
}

/* Displays memory information */
int
cmd_show_memory (struct CLI_ARGS* args) {
	size_t total, avail;
	uint32_t i, count = 0, free_count = 0, todo_count = 0;
	struct NETPACKET* pkt = network_netpacket;

	kmemstats (&total, &avail);
	kprintf ("memory: %u KB total, %u KB available\n", (total / 1024), (avail / 1024));

	/* count the number of free packets */
	for (i = 0; i < network_numbuffers; i++) {
		if (pkt->device != NULL) {
			kprintf ("{%u} ", i);
			count++;
		}

		/* next */
		pkt++;
	}

	/* count the number of free packets according to the chain */
	pkt = netpacket_first_avail;
	while (pkt) { free_count++; pkt = pkt->next; }

	/* count the number of packets to handle */
	pkt = netpacket_todo_first;
	while (pkt) { todo_count++; pkt = pkt->next; }

	kprintf ("netpackets:\n");
	kprintf ("%u total\n", network_numbuffers);
	kprintf ("%u in use\n", count);
	kprintf ("%u in queue to handle\n", todo_count);
	kprintf ("%u marked as available\n", free_count);

	/* NOTICE: the results given are never accurate; this is because the network
	 * system happily frees and allocates buffers while we're counting.
	 *
	 * We could disable interrupts during counting, but that might cause packet
	 * loss, which is something we want to avoid.
	 */
	return 1;
}

/* Displays version information */
int
cmd_show_version (struct CLI_ARGS* args) {
	kprintf ("%s\n", ILIOS_VERSION());
	return 1;
}

/* Displays version information */
int
cmd_help (struct CLI_ARGS* args) {
	struct COMMAND* cmd = commands;
	int i;

	/* wade through all commands */
	while (cmd->cmd) {
		/* display the command */
		kprintf ("%s", cmd->cmd);

		/* line it out */
		i = kstrlen (cmd->cmd);
		while (i++ < CLI_MAX_CMD_LEN) kprintf (" ");

		/* display the description */
		kprintf ("%s\n", cmd->desc);

		/* next */
		cmd++;
	}

	/* all done */
	return 1;
}

/* This will display the ARP table.
 */
int
cmd_arp_list (struct CLI_ARGS* args) {
	int i;

	/* just list the table */
	kprintf ("ARP table:\n");

	/* wade through the entire table */
	for (i = 0; i < ARP_CACHE_SIZE; i++)
		/* address here? */
		if (arp_cache[i].address)
			/* yes. display it */
			kprintf ("%I     %x:%x:%x:%x:%x:%x   %s\n",
					arp_cache[i].address,
					arp_cache[i].hw_addr[0], arp_cache[i].hw_addr[1],	
					arp_cache[i].hw_addr[2], arp_cache[i].hw_addr[3],	
					arp_cache[i].hw_addr[4], arp_cache[i].hw_addr[5],
					arp_cache[i].device->name);

	/* all done */
	return 1;
}

/* This will flush the ARP table.
 */
int
cmd_arp_flush (struct CLI_ARGS* args) {
	arp_flush();
	return 1;
}

/* This will submit an ARP query.
 */
int
cmd_arp_query (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 1);

	/* send the query and begone with it */
	if (arp_send_request (ARG_IPV4ADDR(0), NULL)) {
		/* this failed. complain */
		kprintf ("unable to query address\n");
		return 0;
	}

	/* yay */
	return 1;
}

/* Displays the routing table */
int
cmd_route_list (struct CLI_ARGS* args) {
	int i;

	/* wade through the entire table */
	for (i = 0; i < ROUTE_MAX_ENTRIES; i++)
		/* route here? */
		if (routes[i].flags)
			/* yes. display it */
			kprintf ("%I     %I     %I     %s\n",
					routes[i].network,
					routes[i].mask,
					routes[i].gateway,
					routes[i].device->name);

	/* all done */
	return 1;
}

/* Flushes the routing table */
int
cmd_route_flush (struct CLI_ARGS* args) {
	route_flush();
	return 1;
}

/* Add a static route */
int
cmd_route_add (struct CLI_ARGS* args) {
	struct DEVICE* dev;

	/* safety first */
	ASSERT (args->num_args == 3);

	/* is the gateway reachable? */
	dev = route_find_device (ARG_IPV4ADDR(2));
	if (dev == NULL) {
		/* no. complain */
		kprintf ("supplied gateway is unreachable\n");
		return 0;
	}

	/* does this conflict with anything? */
	if (route_find_ip (dev, ARG_IPV4ADDR(0)) != NULL) {
		/* yes. complain */
		kprintf ("route conflicts with existing route\n");
		return 0;
	}

	/* looks fine, add it */
	route_add (dev, ARG_IPV4ADDR(0), ARG_IPV4ADDR(1), ARG_IPV4ADDR(2), ROUTE_FLAG_GATEWAY);
	return 1;
}

int
cmd_route_delete (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 2);

	/* bye bye */
	if (!route_remove (ARG_IPV4ADDR (0), ARG_IPV4ADDR (1))) {
		/* this failed. complain */
		kprintf ("no such route\n");
		return 0;
	}

	/* victory */
	return 1;
}

int
cmd_set_routing  (struct CLI_ARGS* args) {
	/* safety first */
	ASSERT (args->num_args == 1);

	/* tell the IPv4 stack what to do */
	ipv4_set_routing (ARG_BOOLEAN (0));

	/* victory */
	return 1;
}

/* vim:set ts=2 sw=2: */
