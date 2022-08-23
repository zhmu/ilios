/*
 * irq.c - XeOS Interrupt Manager
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This code is inspired by Yoctix (src/sys/reg/interrupts.c), (c) 1999
 * Anders Gavare.
 *
 * This code will handle the allocation of interrupts.
 *
 */
#include <sys/types.h>
#include <sys/irq.h>
#include <lib/lib.h>
#include <md/config.h>
#include <md/interrupts.h>

IRQ_HANDLER irq_handlers[MAX_IRQS + 1];

/*
 * This will initialize the IRQ manager.
 */
void
irq_init() {
	/* reset all handlers */
	kmemset (&irq_handlers, 0, sizeof (IRQ_HANDLER) * (MAX_IRQS + 1));
}

/*
 * This will assign handler [handler], thread [t] or variable [var] to IRQ
 * [num]. [name] is used as a description. This will return zero on failure on
 * non-zero on success. If [var] is not NULL, it will be used as the refence
 * value. If [p] is not NULL, [handler] is ignored and [p] will be resumed if
 * the interrupt occours.
 */
int
irq_register (uint8_t num, void* handler, struct DEVICE* dev) {
	uint32_t oldints, i;

	/* number within range? */
	if (num > MAX_IRQS)
		/* no. complain */
		return 0;

	/* scan the record for an available entry */
	for (i = 0; i < IRQ_MAX_HANDLERS; i++)
		/* available? */
		if ((irq_handlers[num].device[i] == NULL) &&
			  (irq_handlers[num].handler[i] == NULL))
			/* yes. bail out */
			break;

	/* found a handler? */
	if (i == IRQ_MAX_HANDLERS)
		/* no. too bad, so sad */
		return 0;

	/* update the entry */
	oldints = arch_interrupts (DISABLE);
	irq_handlers[num].device[i] = dev;
	irq_handlers[num].handler[i] = handler;
	irq_handlers[num].stray_count = 0;
	arch_interrupts (oldints);

	/* all went ok */
	return 1;
}

/*
 * irq_unregister (struct DEVICE* dev)
 *
 * This will unregister IRQ's of device [dev].
 *
 */
void
irq_unregister (struct DEVICE* dev) {
	int oldints;
	int i, j;

	/* disable interrupts and zap the handler */
	oldints = arch_interrupts (DISABLE);

	/* scan the record for entries */
	for (i = 0; i < MAX_IRQS; i++)
		for (j = 0; j < IRQ_MAX_HANDLERS; j++)
			/* used? */
			if (irq_handlers[i].device[j] == dev) {
				/* yes. free it */
				irq_handlers[i].device[j] = NULL;
				irq_handlers[i].handler[j] = NULL;
			}

	/* all done, restore interrupts */
	arch_interrupts (oldints);
}

/*
 * irq_handler (unsigned int num)
 *
 * This will run the IRQ handler of IRQ [num].
 *
 */
void
irq_handler (unsigned int num) {
	void (*handler)(struct DEVICE*);
	struct DEVICE* dev;
	uint32_t i, ok = 0;

	/* call all handlers */
	for (i = 0; i < IRQ_MAX_HANDLERS; i++) {
		handler = irq_handlers[num].handler[i];
		dev = irq_handlers[num].device[i];
		if (handler != NULL) {
			handler (dev); ok++;
		}
	}

	/* is this IRQ properly handled? */
	if (ok)
		/* yes. bail out */
		return;

		/* got enough? */
	if (irq_handlers[num].stray_count == -1)
		/* yes. enough is enough, bye */
		return;

	/* increment warn, and get out of here */
	irq_handlers[num].stray_count++;
	
	/* got a high enough stray count? */
	if (irq_handlers[num].stray_count > IRQ_ENOUGH_STRAY) {
		/* yes. log that and stop logging */
		kprintf ("warning: too many stray IRQ 0x%x, not logging anymore\n", num);
		irq_handlers[num].stray_count = -1;
		return;
	}

	/* logging */
	kprintf ("warning: stray IRQ 0x%x\n", num);
}

/* vim:set ts=2 sw=2: */
