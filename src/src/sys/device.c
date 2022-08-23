/*
 * device.c - ILIOS Device Manager
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code will handle the allocation of devices.
 *
 */
#include <sys/types.h>
#include <sys/kmalloc.h>
#include <sys/device.h>
#include <sys/network.h>
#include <sys/tty.h>
#include <lib/lib.h>
#include <md/interrupts.h>

struct DEVICE* coredevice = NULL;

/*
 * This will register a new device for thread [t], based on [dev]. It will
 * return a pointer to the new device, or NULL on failure.
 */
struct DEVICE*
device_register (struct DEVICE* dev) {
	struct DEVICE* device = coredevice;
	struct DEVICE* newdevice;

	/* initialize the new device */
	newdevice = (struct DEVICE*)kmalloc (NULL, sizeof (struct DEVICE), 0);
	kmemcpy (newdevice, dev, sizeof (struct DEVICE));

	/* duplicate the name */
	newdevice->name = kstrdup (dev->name);

	/* find the end of the chain */
	if (device)
		while (device->next)
			device = device->next;

	/* got a core device? */
	if (coredevice == NULL)
		/* no. replace it */
		coredevice = newdevice;
	else
		device->next = newdevice;
	
	/* neither rx or tx data */
	newdevice->rx_frames = 0; newdevice->rx_bytes = 0;
	newdevice->tx_frames = 0; newdevice->tx_bytes = 0;

	/* display the address */
	kprintf ("%s at", newdevice->name);
	if (newdevice->resources.port)
		kprintf (" io 0x%x", newdevice->resources.port);
	if (newdevice->resources.irq)
		kprintf (" irq 0x%x", newdevice->resources.irq);
	if (newdevice->resources.drq)
		kprintf (" drq 0x%x", newdevice->resources.drq);
	kprintf ("\n");

	/* all set */
	return newdevice;
}

/*
 * This will unregister device [dev].
 */
void
device_unregister (struct DEVICE* dev) {
	struct DEVICE* device = coredevice;
	struct DEVICE* prevdev = NULL;

	/* wade through the tree */
	while (device) {
		/* match? */
		if (device == dev) {
			/* yes. core device? */
			if (dev == coredevice) {
				/* yes. we have a new core device! */
				coredevice = dev->next;
				break;
			} else {
				/* no. update the previous device's next pointer */
				prevdev->next = dev->next;
				break;
			}
		}

		/* next */
		prevdev = device; device = device->next;
	}

	/* free the device structures */
	kfree (dev->name);

	/* free the device itself */
	kfree (dev);
}

/*
 * This will initialize the device manager.
 */
void
device_init() {
	/* nothing to do! */
}

/*
 * This will show a listing of all available devices.
 */
void
device_dump() {
	struct DEVICE* dev = coredevice;

	kprintf ("Device dump\n");

	/* list them all */
	while (dev) {
		kprintf ("  %x: device '%s'\n", dev, dev->name);
		dev = dev->next;
	}
}

/*
 * This will scan the device chain for device [devname]. It will return a
 * pointer to the device if found or NULL on failure.
 */
struct DEVICE*
device_find (char* devname) {
	struct DEVICE* dev = coredevice;

	/* scan them all */
	while (dev) {
		/* match? */
		if (!kstrcmp (dev->name, devname))
			/* yes. got it */
			return dev;

		/* next */
		dev = dev->next;
	}
	return NULL;
}

/* vim:set ts=2 sw=2: */
