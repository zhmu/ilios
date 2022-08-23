/*
 * input.c - XeOS input calls.
 * (c) 2003 Rink Springer, BSD licensed
 *
 * This code will handle input.
 *
 */
#include <sys/types.h>
#include <md/console.h>
#include <md/reboot.h>

#define INPUT_LEN	256
char input_tmp[INPUT_LEN];

const char*
gets() {
	int i = 0;
	uint8_t ch = 0;

	/* keep going */
	do {
		/* fetch a key */
		ch = 0;
		while (!ch) {
			/* save the processor, we only need to act on interrupts anyway */
			arch_relax();

			/* fetch the key */
			ch = arch_console_readch();
		}
		
		switch (ch) {
			case 8: /* backspace. got chars to burn? */
							if (i) {
			        	/* yes. zap the last char */
			        	input_tmp[--i] = 0;
			        	arch_console_putchar (8);
								arch_console_putchar (32);
								arch_console_putchar (8);
							}
							break;
		 case 13: /* return. leave */
		          arch_console_putchar ('\n');
							break;
		 case 21: /* ctrl-u. clear the entire line */
							for (; i > 0; i--) {
		          	arch_console_putchar (8);
		          	arch_console_putchar (32);
		          	arch_console_putchar (8);
							}
							break;
		 case 23: /* ctrl-w. clear the last word */
							while (i > 0) {
								i--;
		          	arch_console_putchar (8);
		          	arch_console_putchar (32);
		          	arch_console_putchar (8);
								if (input_tmp[i] == ' ')
									break;
							}
							break;
		 default: /* dunno. just add it, if we have space */
							if (i < (INPUT_LEN - 1)) {
								input_tmp[i++] = ch;
								arch_console_putchar (ch);
							}
		}
	} while (ch != 13);

	/* null-terminate it and leave */
	input_tmp[i] = 0;
	return input_tmp;
}

/*
 * get_digit (char ch, int base)
 *
 * This will retrieve the value of [ch] in [base], or -1 on failure.a
 *
 */
int
get_digit (char ch, int base) {
	if ((ch >= '0') && (ch <= '9')) return (ch - '0');

	if (base == 16) {
		if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
		if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
  }

  return -1;
}

/*
 * strtol(const char *nptr, char **endptr, int base)
 * 
 * (man strtol)
 *
 */
long int
strtol (const char *nptr, char **endptr, int base) {
	long int li = 0;
	long int lj;
	int count = 0;
	char* ptr;
	int i, j;

	/* first, we must see how much charachters we have */
	ptr = (char*)nptr;
	while (get_digit (*ptr, base) != -1) {
		ptr++; count++;
	}
	
	/* store the final position */
	if (endptr)
		*endptr = ptr;

	/* handle all digits */
	ptr = (char*)nptr;
	for (i = 0; i < count; i++) {
		/* calculate the power */
		lj = 1;
		for (j = 0; j < (count - i - 1); j++)
			lj *= (long int)base;
		li += ((long int)get_digit (*ptr, base) * (long int)lj);

		/* next */
		ptr++;
	}

	return li;
}

/* vim:set ts=2 sw=2: */
