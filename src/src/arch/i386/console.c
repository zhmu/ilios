/*
 * console.c - ILIOS i386 Console Driver
 * (c) 2002, 2003 Rink Springer, BSD licensed
 *
 * This will handle the internal i386-only console.
 *
 * The cursor movement code was inspired by
 * http://osdev.berlios.de/console_io.html, (c) Jens Olsson
 *
 */
#include <sys/tty.h>
#include <sys/types.h>
#include <sys/irq.h>
#include <sys/device.h>
#include <lib/lib.h>
#include <md/memory.h>
#include <md/pio.h>

addr_t tty_videobase = 0xb8000;
uint32_t offs = 0;
uint32_t tty_crtc_base;

void i386_status(); /* XXX */

#define CONSOLE_ATTR 0x07

#define KEYBUF_SIZE 16
#define KEYFLAG_SHIFT 1
#define KEYFLAG_CTRL 2

char keybuf[KEYBUF_SIZE];
size_t	keybuf_curpos = 0;
uint8_t	key_flags = 0;

uint8_t console_keymap_lcase[128] = {
	/* 00-07 */    0, 0x1b,  '1',  '2',  '3',  '4',  '5',  '6',
  /* 08-0f */  '7',  '8',  '9',  '0',  '-',  '=',    8,    9,
  /* 10-17 */  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
	/* 18-1f */  'o',  'p',  '[',  ']', 0x0d,    0,  'a',  's',
	/* 20-27 */  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
	/* 28-2f */ '\'',  '`',   0,  '\\',  'z',  'x',  'c',  'v',
	/* 30-37 */  'b',  'n',  'm',  ',',  '.',  '/',  '?',  '*',
	/* 38-3f */    0,  ' ',    0,    0,    0,    0,    0,    0,
	/* 40-47 */    0,    0,    0,    0,    0,    0,    0,  '7',
	/* 48-4f */  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	/* 50-57 */  '2',  '3',  '0',  '.',    0,    0,  'Z',    0,
	/* 57-5f */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 60-67 */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 68-6f */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 70-76 */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 77-7f */    0,    0,   0,     0,    0,    0,    0,    0
};

uint8_t console_keymap_ucase[128] = {
	/* 00-07 */    0, 0x1b,  '!',  '@',  '#',  '$',  '%',  '^',
  /* 08-0f */  '&',  '*',  '(',  ')',  '_',  '+',    8,    9,
  /* 10-17 */  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
	/* 18-1f */  'O',  'P',  '{',  '}', 0x0d,    0,  'A',  'S',
	/* 20-27 */  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ';',
	/* 28-2f */  '"',  '~',   0,   '|',  'Z',  'X',  'C',  'V',
	/* 30-37 */  'B',  'N',  'M',  '<',  '>',  '?',  '?',  '*',
	/* 38-3f */    0,  ' ',    0,    0,    0,    0,    0,    0,
	/* 40-47 */    0,    0,    0,    0,    0,    0,    0,  '7',
	/* 48-4f */  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
	/* 50-57 */  '2',  '3',  '0',  '.',    0,    0,  'Z',    0,
	/* 57-5f */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 60-67 */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 68-6f */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 70-76 */    0,    0,   0,     0,    0,    0,    0,    0,
	/* 77-7f */    0,    0,   0,     0,    0,    0,    0,    0
};

uint8_t console_keymap_ctrl[128] = {
	/* 00-07 */    0,    0,    0,    0,    0,    0,    0,    0,
  /* 08-0f */    0,    0,    0,    0,    0,    0,    0,    0,
  /* 10-17 */   17,   23,    5,   18,   20,   25,   21,    9,
	/* 18-1f */   15,   16,    0,    0,    0,    0,    1,   19,
	/* 20-27 */    4,    6,    7,    8,   10,   11,   12,    0,
	/* 28-2f */    0,    0,    0,    0,   26,   24,    3,   22,
	/* 30-37 */    2,   14,   13,    0,    0,    0,    0,    0,
	/* 38-3f */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 40-47 */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 48-4f */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 50-57 */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 57-5f */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 60-67 */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 68-6f */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 70-76 */    0,    0,    0,    0,    0,    0,    0,    0,
	/* 77-7f */    0,    0,    0,    0,    0,    0,    0,    0
};

void arch_console_setpage (uint8_t pageno);

/*
 * This will read a single char from the keyboard buffer. It will return the
 * char on success or 0 on failure.
 */
uint8_t
arch_console_readch() {
	uint8_t ch;

	/* got any chars in the buffer? */
	if (keybuf_curpos == 0)
		/* no. return zero */
		return 0;

	/* fetch the first char */
	ch = keybuf[0];

	/* shift everything */
	kmemcpy (keybuf, keybuf + 1, KEYBUF_SIZE);
	keybuf_curpos--;

	/* return the char */
	return ch;
}

/*
 * This will check whether a key is available. It will return
 * zero if not, or the ASCII scan code if there is. The key
 * will *NOT* be removed from the buffer.
 */
uint8_t
arch_console_peekch() {
	/* got any chars in the buffer? */
	if (keybuf_curpos == 0)
		/* no. return zero */
		return 0;

	/* return the char */
	return keybuf[0];
}

/*
 * This will handle 'special' key [sc].
 */
void
kbd_do_specialkey (uint8_t sc) {
	uint8_t _sc = (sc & 0x7f);
	uint8_t bit = 0;

	switch (_sc) {
		case 0x1D: /* ctrl */
		           bit = KEYFLAG_CTRL;
							 break;
		case 0x2A: /* left shift */
		case 0x36: /* right shift */
		           bit = KEYFLAG_SHIFT;
							 break;
	}

	/* have a bit to touch? */
	if (bit) {
		/* yes. make? */
		if (sc == _sc)
			/* yes. set the bit */
			key_flags |= bit;
		else
			/* no. remove the bit */
			key_flags &= ~bit;
	}

	/* got F1-F8 ? */
	if ((_sc >= 0x3B) && (_sc <= 0x42)) {
		/* yes. activate that video page */
	  if (_sc == 0x3C) /* XXX: F2 displays the status page */
			i386_status();
		arch_console_setpage (_sc - 0x3B);
	}
}

/*
 * This will handle scancode [sc].
 */
void
kbd_do_scancode (uint8_t sc) {
	uint8_t ch;

	/* break? */
	if (sc & 0x80) {
		/* yes. handle potention special keys */
		kbd_do_specialkey (sc);
		return;
	}

	/* fetch the char */
	ch = console_keymap_lcase[sc];

	/* did we have something? */
	if (ch) {
		/* yes. is shift touched? */
		if (key_flags & KEYFLAG_SHIFT) {
			/* yes. uppercase the char */
			ch = console_keymap_ucase[sc];
		}

		/* is ctrl touched? */
		if (key_flags & KEYFLAG_CTRL) {
			/* yes. remap the char */
			ch = console_keymap_ctrl[sc];
		}

		/* still have something? */
		if (ch) {
			/* yes. put in into the keyboard buffer */
			keybuf[keybuf_curpos] = ch;
			keybuf_curpos++;
		
			/* about to overrun? */
			if (keybuf_curpos == KEYBUF_SIZE)
				/* yes. discard a char */
				(void)arch_console_readch();
		}
	} else {
		/* handle potential special keys */
		kbd_do_specialkey (sc);
	}
}

/*
 * This will handle keyboard IRQ's.
 */
void
arch_console_irq (struct DEVICE* dev) {
	/* fetch the input */
	while (inb (0x64) & 1) {
		/* handle scancode */
		kbd_do_scancode (inb (0x60));
	}
}

/*
 * This will initialize the i386 console.
 */
void
arch_console_init() {
	uint8_t data;
	size_t i;

	/* figure out the video type */
	asm ("inb %w1,%0" : "=a" (data) : "d" (0x3cc)); /* data = inb (0x3cc) */
	if ((data & 1)) {
		tty_videobase = 0xb8000; tty_crtc_base = 0x3d0;
	} else {
		tty_videobase = 0xb0000; tty_crtc_base = 0x3c0;
	}
	tty_videobase = MAP_MEMORY (tty_videobase);

	/* register our irq */
	if (!irq_register (1, (void*)&arch_console_irq, NULL)) {
		kprintf ("console: cannot register IRQ 1\n");
	}
	
	/* blank the screen */
	for (i = 0; i < 8000; i += 2)
		*(uint16_t*)(tty_videobase + i) = (CONSOLE_ATTR << 8) | 0x20;

	/* read and discard all current chars in the keyboard buffer */
	while (inb (0x64) & 1)
		(void)inb (0x60);

}

/*
 * This will put char [ch] raw and plain on the screen.
 */
void
arch_console_putch (uint8_t ch) {
#if 1
	uint16_t* w = (uint16_t*)(tty_videobase + offs);

	/* just write it in the videomemory */
	*w = (CONSOLE_ATTR << 8) | ch;
	offs += 2;
#else
	uint8_t* b = (uint8_t*)(tty_videobase + offs);
	*b = ch;
	offs += 2;
#endif
}

/*
 * This will put [ch] on the console. Special chars and scrolling are supported.
 */
void
arch_console_putchar (uint8_t ch)  {
	switch (ch) {
		case '\n': /* newline */
		           offs = offs - (offs % 160);
		           offs += 160;
							 break;
		   case 8: /* backspace */
							 offs -= 2;
							 break;
			default: /* just print it */
		           arch_console_putch (ch);
							 break;
	}

	/* need to scroll? */
	if (offs >= 4000) {
		/* handle the scrolling */
		kmemcpy ((void*)(tty_videobase),
		         (void*)(tty_videobase + 160),
		 				 (24 * 160));

		/* blank out the bottom line */
		kmemset ((void*)(tty_videobase + (24 * 160)), 0x0, 160);

		/* one line less */
		offs -= 160;
	}

	/* move the cursor */
	outb (tty_crtc_base + 4, 0xf);
	outb (tty_crtc_base + 5, ((offs >> 1) & 0xff));
	outb (tty_crtc_base + 4, 0xe);
	outb (tty_crtc_base + 5, (offs >> 1) >> 8);
}

/*
 * This will print [s] on the console.
 */
void
arch_console_puts (const char* s) {
	while (*s)
		arch_console_putchar (*s++);
}

/*
 * This will clear the console and set the cursor to the top-left position.
 */
void
arch_console_clear() {
	/* start at the top left */
	offs = 0;

	/* clear the screen */
	kmemset ((void*)tty_videobase, 0, 4000);

	/* move the cursor */
	outb (tty_crtc_base + 4, 0xf);
	outb (tty_crtc_base + 5, ((offs >> 1) & 0xff));
	outb (tty_crtc_base + 4, 0xe);
	outb (tty_crtc_base + 5, (offs >> 1) >> 8);
}

/*
 * This will activate page [pageno].
 */
void
arch_console_setpage (uint8_t pageno) {
	uint16_t offset = (pageno * 2000);

	outb (tty_crtc_base + 4, 0xd);
	outb (tty_crtc_base + 5, (offset & 0xff));
	outb (tty_crtc_base + 4, 0xc);
	outb (tty_crtc_base + 5, (offset >> 8));
}

/* vim:set ts=2 sw=2: */
