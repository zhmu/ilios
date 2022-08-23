/*
 * ILIOS IPv4 TCP/IP network stack
 * (c) 2003 Rink Springer
 *
 * This will handle socket administration.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmalloc.h>
#include <net/socket.h>
#include <lib/lib.h>
#include <netipv4/tcp.h>

struct SOCKET* socket;

/*
 * This will return an available socket structure, or NULL on failure.
 */
struct SOCKET*
socket_alloc (int type) {
	int i;

	/* wade through the sockets */
	for (i = 0; i < SOCKET_NUM_SOCKS; i++)
		/* available socket? */
		if (socket[i].type == SOCKET_TYPE_UNUSED) {
			/* yes. set the socket up */
			kmemset (&socket[i], 0, sizeof (struct SOCKET));
			socket[i].type = type;

			/* all done */
			return &socket[i];
		}

	/* no sockets available */
	return NULL;
}

/*
 * This will scan which socket is bound to port [port] of type [type]. It will
 * return zero on failure or non-zero on success.
 */
struct SOCKET*
socket_find (int type, int port) {
	int i;

	/* wade through the sockets */
	for (i = 0; i < SOCKET_NUM_SOCKS; i++)
		/* match? */
		if ((socket[i].type == type) && (socket[i].port == port))
			/* yes. return the socket */
			return &socket[i];

	/* not bound */
	return NULL;
}

/*
 * This will try to bind a socket [s] to port [port]. It will return zero on
 * failure or non-zero on success.
 */
int
socket_bind (struct SOCKET* s, int port) {
	/* is that port already bound? */
	if (socket_find (s->type, port) != NULL)
		/* this failed. bail out */
		return 0;

	/* set the port as bound */
	s->state = SOCKET_STATE_LISTEN;
	s->port = port;

	/* TCP? */
	if (s->type == SOCKET_TYPE_TCP4)
		/* yes. give it a chance to modify the socket */
		tcp_init_socket (s);

	/* all done */
	return 1;
}

/*
 * This will close socket [s].
 */
void
socket_close (struct SOCKET* s) {
	s->state = SOCKET_STATE_UNUSED;
	s->port = 0;
}

/*
 * This will set the callback routine of socket [s] to [callback].
 */ 
void
socket_set_callback (struct SOCKET* s, void (*callback)(struct SOCKET* s, struct DEVICE* dev, uint32_t addr, void* data, uint32_t len)) {
	s->callback = callback;
}

/*
 * This will initialize the socket administration.
 */
void
socket_init() {
	/* allocate memory for the socket administration */
	socket = (struct SOCKET*)kmalloc (NULL, sizeof (struct SOCKET) * SOCKET_NUM_SOCKS, 0);

	/* reset the sockets */
	kmemset (socket, 0, sizeof (struct SOCKET) * SOCKET_NUM_SOCKS);
}

/* vim:set ts=2 sw=2 tw=78: */
