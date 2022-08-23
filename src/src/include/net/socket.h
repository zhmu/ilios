/*
 *
 * ILIOS TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This will handle socket administration.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <netipv4/ip.h>

#ifndef __SOCKET_H__
#define __SOCKET_H__

/* SOCKET_NUM_SOCKS is the number of sockets supported */
#define SOCKET_NUM_SOCKS 1024

/* SOCKET_TYPE_xxx are the supported socket types */
#define SOCKET_TYPE_UNUSED 0
#define SOCKET_TYPE_UDP4 1
#define SOCKET_TYPE_TCP4 2

/* SOCKET_STATE_xxx are the socket states */
#define SOCKET_STATE_UNUSED 0
#define SOCKET_STATE_LISTEN 1

struct SOCKET {
	uint8_t	state;
	uint8_t type;
	uint16_t port;

	void (*callback)(struct SOCKET* s, struct DEVICE* dev, uint32_t addr, void* data, uint32_t len);

	uint8_t	sockdata[64];
};

extern struct SOCKET* socket;

void socket_init();
struct SOCKET* socket_alloc (int type);
struct SOCKET* socket_find (int type, int port);
void socket_close (struct SOCKET* s);
int socket_bind (struct SOCKET* s, int port);
void socket_set_callback (struct SOCKET* s, void (*callback)(struct SOCKET* s, struct DEVICE* dev, uint32_t addr, void* data, uint32_t len));

#endif /* __SOCKET_H__ */
