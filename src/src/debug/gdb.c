/*
 * gdb.c - ILIOS GNU Debugger Suport Code
 *
 * This provides support for the GNU Debugger!
 *
 */
#include <sys/types.h>
#include <md/sio.h>
#include <lib/lib.h>
#include <debug/gdb.h>

#ifdef SUPPORT_GDB

unsigned char gdb_debugch[64];
int gdb_debugptr = 0;
int gdb_curptr = 0;

extern void set_debug_traps();

/*
 * gdb_handle_input (char ch)
 *
 * This will handle GDB input char [ch].
 *
 */
void
gdb_handle_input (char ch) {
	gdb_debugch[gdb_debugptr++] = ch;
	if (gdb_debugptr >= 64)
		gdb_debugptr = 0;
}

/*
 * getDebugChar()
 *
 * Called by the GDB core when it wants a char.
 *
 */
char
getDebugChar() {
	unsigned char ch;

	asm ("pushf\nsti");
	while (gdb_debugptr == gdb_curptr);
	asm ("popf");

	ch = gdb_debugch[gdb_curptr++];
	if (gdb_curptr >= 64)
		gdb_curptr = 0;
	return ch;
}

/*
 * putDebugChar(unsigned char ch)
 *
 * Called by the GDB core when it wants a char.
 *
 */
void
putDebugChar(unsigned char ch) {
	arch_sio_send (ch);
}

/*
 * gdb_init()
 *
 * THis will initialize the GDB kernel debugger.
 *
 */
void
gdb_init() {
	/* initialize debugger hooks */
	set_debug_traps();
}

/*
 * gdb_launch()
 *
 * This will launch the GNU debugger! Oh yes!
 *
 */
void
gdb_launch() {
	breakpoint();
}
#endif /* SUPPORT_GDB */

/* vim:set ts=2 sw=2: */
