/*
 * device.h - XeOS Device Manager
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This is a generic include file which describes the devices.
 *
 */
#include <sys/types.h>
#include <sys/network.h>
#include <netipv4/ipv4.h>
#include <md/config.h>

#ifndef __DEVICE_H__
#define __DEVICE_H__

struct DEVICE;

/* DEVICE_BUFSIZE is the size we use to store internals transfer buffers */
#define DEVICE_BUF_SIZE		(PAGESIZE)

/* DEVICE_NUM_TXBUFS is the number of buffers a device has */
#define DEVICE_NUM_TXBUFS	32

/*
 * DEVICE_RESOURCES is the resources of the device.
 *
 */
struct DEVICE_RESOURCES {
	uint16_t port;
	uint8_t  irq;
	uint8_t  drq;
  uint16_t flags;
};

/*
 * DEVICE_NETDATA is data belonging to a network device
 *
 */
struct DEVICE_NETDATA {
	void*					 data;				/* more data */
};

struct NETPACKET;

/*
 * DEVICE is a structure which covers about any device in the system.
 *
 */
struct DEVICE {
	char*                   name;      /* device name */
	struct DEVICE*          next;      /* next device */
	struct DEVICE_RESOURCES	resources; /* resources */
	void*                   data;      /* device specific data */

	ETHERNET_OPTIONS ether;

	uint8_t					       addr_len;
	struct IPV4_CONFIG     ipv4conf;

	struct NETPACKET*      xmit_packet_first;
	struct NETPACKET*      xmit_packet_last;

	uint64_t               rx_bytes, rx_frames;
	uint64_t               tx_bytes, tx_frames;

  void (*xmit)(struct DEVICE* dev);
};

#ifdef __KERNEL
extern struct DEVICE* coredevice;

void           device_init();
struct DEVICE* device_register(struct DEVICE*);
void           device_unregister (struct DEVICE* dev);

void           device_dump();
struct         DEVICE* device_find (char*);
size_t         device_xmit (struct DEVICE* dev, void* buffer, size_t count);
#endif /* __KERNEL */
#endif

/* vim:set ts=2 sw=2: */
