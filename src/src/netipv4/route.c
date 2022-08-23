/*
 * route.c - XeOS IPv4 Routing Stuff
 *
 * (c) 2003 Rink Springer, BSD
 *
 * This will handle routing.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/network.h>
#include <sys/kmalloc.h>
#include <lib/lib.h>
#include <netipv4/ipv4.h>
#include <netipv4/route.h>

struct ROUTE_ENTRY* routes;

/*
 * This will return the device which to use for sending packets to [dest]. It
 * will return NULL if no devices could be found.
 *
 */
struct DEVICE*
route_find_device (uint32_t dest) {
	int i;

	/* scan all routes */
	for (i = 0; i < ROUTE_MAX_ENTRIES; i++)
		/* got a route? */
		if (routes[i].network == (dest & routes[i].mask))
			/* yes. return the device */
			return routes[i].device;

	/* not found. too bad */
	return NULL;
}

/*
 * route_find_ip (struct DEVICE* dev, uint32_t dest)
 *
 * This will find the IP record that is needed to send packets to [dest].
 * It return NULL on failure or the record on success.
 *
 */
struct IPV4_ADDR*
route_find_ip (struct DEVICE* dev, uint32_t dest) {
	int i;

	/* scan all routes */
	for (i = 0; i < IPV4_MAX_ADDR; i++)
		if ((dev->ipv4conf.address[i].addr & dev->ipv4conf.address[i].netmask) ==
				(dest & dev->ipv4conf.address[i].netmask))
			/* got it! */
			return &dev->ipv4conf.address[i];

	/* too bad */
	return NULL;
}

/*
 * route_add (struct DEVICE* dev, uint32_t dest, uint32_t mask,
 *            uint32_t gateway, uint32_t flags)
 *
 * This will add a route on device [dev] to [dest] with mask [mask]. It will
 * return zero on failure or non-zero on success.
 *
 */
int
route_add (struct DEVICE* dev, uint32_t dest, uint32_t mask, uint32_t gateway, uint32_t flags) {
	int i;

	/* scan all routes */
	for (i = 0; i < ROUTE_MAX_ENTRIES; i++)
		/* got an unused route? */
		if (!routes[i].flags) {
			/* yes. set the route up */
			routes[i].device = dev;
			routes[i].network = (dest & mask);
			routes[i].mask = mask;
			routes[i].gateway = gateway;
			routes[i].flags = flags | ROUTE_FLAG_INUSE;

			/* all done */
			return 1;
		}

	/* too bad */
	return 0;
}

/*
 * This will remove the route to [dest] with mask [mask]. It will return zero on
 * failure or non-zero on success.
 *
 */
int
route_remove (uint32_t dest, uint32_t mask) {
	int i;

	/* scan all routes */
	for (i = 0; i < ROUTE_MAX_ENTRIES; i++)
		/* got the route? */
		if (routes[i].network == (dest & routes[i].mask)) {
			/* yes. zap it */
			kmemset (&routes[i], 0, sizeof (struct ROUTE_ENTRY));
			return 1;
		}

	/* not found. too bad */
	return 0;
}

/*
 * route_flush()
 *
 * This will flush the routing table.
 *
 */
void
route_flush() {
	int i;

	/* scan all routes */
	for (i = 0; i < ROUTE_MAX_ENTRIES; i++)
		/* used but not permanent? */
		if ((routes[i].flags & ROUTE_FLAG_INUSE) &&
				!(routes[i].flags & ROUTE_FLAG_PERM))
			/* yes. zap it */
			kmemset (&routes[i], 0, sizeof (struct ROUTE_ENTRY));
}

/*
 * route_init()
 *
 * This will initialize the routing table.
 *
 */
void
route_init() {
	/* allocate memory for the routing table */
	routes = (struct ROUTE_ENTRY*)kmalloc (NULL, sizeof (struct ROUTE_ENTRY) * ROUTE_MAX_ENTRIES, 0);
	kmemset (routes, 0, sizeof (struct ROUTE_ENTRY) * ROUTE_MAX_ENTRIES);
}

/* vim:set ts=2 sw=2 tw=78: */
