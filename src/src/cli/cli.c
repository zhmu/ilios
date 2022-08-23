/*
 * cli.c - ILIOS Command Line Interface
 *
 * This will handle commands typed at the console.
 *
 */
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmalloc.h>
#include <lib/lib.h>
#include <md/console.h>
#include <md/reboot.h>
#include <cli/cli.h>
#include <cli/cmd.h>
#include <assert.h>
#include <config.h>

#define MIN(x,y) ((x)<(y)?(x):(y))

char hostname[HOSTNAME_LEN];

/* command declarations */
int cmd_int_create   (struct CLI_ARGS* args);
int cmd_int_destroy  (struct CLI_ARGS* args);
int cmd_int_bind     (struct CLI_ARGS* args);
int cmd_int_unbind   (struct CLI_ARGS* args);
int cmd_int_status   (struct CLI_ARGS* args);
int cmd_reboot       (struct CLI_ARGS* args);
int cmd_set_hostname (struct CLI_ARGS* args);
int cmd_show_memory  (struct CLI_ARGS* args);
int cmd_show_version (struct CLI_ARGS* args);
int cmd_help         (struct CLI_ARGS* args);
int cmd_arp_list     (struct CLI_ARGS* args);
int cmd_arp_flush    (struct CLI_ARGS* args);
int cmd_arp_query    (struct CLI_ARGS* args);
int cmd_route_list   (struct CLI_ARGS* args);
int cmd_route_flush  (struct CLI_ARGS* args);
int cmd_route_add    (struct CLI_ARGS* args);
int cmd_route_delete (struct CLI_ARGS* args);
int cmd_set_routing  (struct CLI_ARGS* args);

/*
 * syntax:
 *
 * command %(type){(descr) %(type){desc}
 *
 * if = interface
 * dv = driver
 * di = decimal integer
 * hi = hexadecimal integer
 * ip = IP address
 * st = String
 * om = Operating Mode
 * bl = Boolean
 */
struct COMMAND commands[] = {
	{
		"interface create",
		"Creates an interface",
		"%st{interface name} %dv{driver} @hi{io port} @hi{irq}",
		&cmd_int_create
	},
	{
		"interface destroy",
		"Deletes an interface",
		"%if{interface name}",
		&cmd_int_destroy
	},
	{
		"interface bind",
		"Binds an interface",
		"%if{interface name} %ip{ip address} @ip{netmask}",
		&cmd_int_bind
	},
	{
		"interface unbind",
		"Unbinds an interface",
		"%if{interface name} %ip{ip address}",
		&cmd_int_unbind
	},
	{
		"reboot",
		"Reboots the machine",
		"",
		&cmd_reboot
	},
	{
		"set hostname",
		"Set hostname",
		"%st{host name}",
		&cmd_set_hostname
	},
	{
		"interface status",
		"Interface status",
		"",
		&cmd_int_status
	},
	{
		"show memory",
		"Memory information",
		"",
		&cmd_show_memory
	},
	{
		"show version",
		"Version information",
		"",
		&cmd_show_version
	},
	{
		"help",
		"Display available commands",
		"",
		&cmd_help
	},
	{
		"arp list",
		"Display the ARP table",
		"",
		&cmd_arp_list
	},
	{
		"arp flush",
		"Flush the ARP table",
		"",
		&cmd_arp_flush
	},
	{
		"arp query",
		"Submit an ARP query",
		"%ip{IP}",
		&cmd_arp_query
	},
	{
		"route list",
		"Displays the routing table",
		"",
		&cmd_route_list
	},
	{
		"route flush",
		"Flushes the routing table",
		"",
		&cmd_route_flush
	},
	{
		"route add",
		"Add a static route",
		"%ip{network} %ip{netmask} %ip{gateway}",
		&cmd_route_add
	},
	{
		"route delete",
		"Removes a static route",
		"%ip{network} %ip{netmask}",
		&cmd_route_delete
	},
	{
		"set routing",
		"Enable or disable router operation",
		"%bl{yes/no]",
		&cmd_set_routing
	},
	{ NULL, NULL, NULL, NULL } 
};

struct BOOL_VALUE bool_values[] = {
	{ "yes",     1 },
	{ "on",      1 },
	{ "enable",  1 },
	{ "no",      0 },
	{ "off",     0 },
	{ "disable", 0 },
 	{ NULL,      0 }
};

int ne_init (char* name, struct DEVICE_RESOURCES* res);
int lo_init (char* name, struct DEVICE_RESOURCES* res);

struct NIC_DRIVER nic_drivers[] = {
	{
		"ne",
		"NE2000 compitable driver",
		&ne_init
	},
	{
		"lo",
		"Loopback driver",
		&lo_init
	},
	{ NULL, NULL }
};

/* list_xxx_cmd are the list functions for a command chain */
void list_init_cmd (addr_t* arg) { *arg = (addr_t)commands; }
char* list_fetch_cmd (addr_t* arg) { struct COMMAND* cmd = (struct COMMAND*)*arg; return cmd->cmd; }
char* list_fetchdesc_cmd (addr_t* arg) { struct COMMAND* cmd = (struct COMMAND*)*arg; return cmd->desc; }
void list_next_cmd (addr_t* arg) { *arg = *arg + sizeof (struct COMMAND); }
int list_done_cmd (addr_t* arg) { struct COMMAND* cmd = (struct COMMAND*)*arg; return cmd->cmd == NULL ? 1 : 0; }

/* list_xxx_drv are the list functions for a driver chain */
void list_init_drv (addr_t* arg) { *arg = (addr_t)nic_drivers; }
char* list_fetch_drv (addr_t* arg) { struct NIC_DRIVER* drv = (struct NIC_DRIVER*)*arg; return drv->name; }
char* list_fetchdesc_drv (addr_t* arg) { struct NIC_DRIVER* drv = (struct NIC_DRIVER*)*arg; return drv->desc; }
void list_next_drv (addr_t* arg) { *arg = *arg + sizeof (struct NIC_DRIVER); }
int list_done_drv (addr_t* arg) { struct NIC_DRIVER* cmd = (struct NIC_DRIVER*)*arg; return cmd->name == NULL ? 1 : 0; }

/* list_xxx_dev are the list functions for an interface chain */
void list_init_dev (addr_t* arg) { *arg = (addr_t)coredevice; }
char* list_fetch_dev (addr_t* arg) { struct DEVICE* dev = (struct DEVICE*)*arg; return dev->name; }
void list_next_dev (addr_t* arg) { struct DEVICE* dev = (struct DEVICE*)*arg; *arg = (addr_t)dev->next; }
int list_done_dev (addr_t* arg) { struct DEVICE* dev = (struct DEVICE*)*arg; return dev->next == NULL ? 1 : 0; }

/* list_xxx_bool are the list functions for a boolean chain */
void list_init_bool (addr_t* arg) { *arg = (addr_t)bool_values; }
char* list_fetch_bool (addr_t* arg) { struct BOOL_VALUE* bv = (struct BOOL_VALUE*)*arg; return bv->name; };
void list_next_bool (addr_t* arg) { *arg = *arg + sizeof (struct BOOL_VALUE); };
int list_done_bool (addr_t* arg) { struct BOOL_VALUE* bv = (struct BOOL_VALUE*)*arg; return (bv->name == NULL) ? 1 : 0; }

/*
 * This will scan the driver table for the driver. It will return a pointer to
 * the entry on success or NULL on failure.
 *
 */ 
struct NIC_DRIVER*
driver_find (char* name) {
	struct NIC_DRIVER* drv = nic_drivers;

	/* wade though 'em all */
	while (drv->name) {
		/* match? */
		if (!kstrcmp (drv->name, name))
			/* yes. victory! */
			return drv;

		/* next */
		drv++;
	}

	/* no such driver, sorry */
	return NULL;
}

/*
 * This will scan the boolean table for the value. It will return -1 on failure
 * or the value on success.
 */ 
int
bool_find (char* name) {
	struct BOOL_VALUE* bv = bool_values;

	/* wade though 'em all */
	while (bv->name) {
		/* match? */
		if (!kstrcmp (bv->name, name))
			/* yes. victory! */
			return bv->value;

		/* next */
		bv++;
	}

	/* no such value, sorry */
	return -1;
}

/*
 * This will fetch an IP address from [buf] and put it in [addr]. It will
 * return 1 and update [buf] on success or return 0 on failure.
 *
 */
int
ip_fetch_addr (char** buf, uint32_t* addr) {
	uint32_t a, b, c, d;
	char* ptr = *buf;

	/* isolate the IP addresses */
	a = strtol (ptr, &ptr, 10);
	if (*ptr == '.') {
		ptr++;
		b = strtol (ptr, &ptr, 10);
		if (*ptr == '.') {
			ptr++;
			c = strtol (ptr, &ptr, 10);
			if (*ptr == '.') {
				ptr++;
				d = strtol (ptr, &ptr, 10);
			} else
				return 0;
		} else
			return 0;
	} else
		return 0;

	/* is the range correct? */
	if ((a > 255) || (b > 255) || (c > 255) || (d > 255))
		/* no. too bad */
		return 0;

	/* yay, got it! copy the address and update the pointer */
	*buf = ptr;
	*addr = (a << 24) | (b << 16) | (c << 8) | (d & 0xff);
	return 1;
}

/* This will try to expand stuff in a given list.
 */
void
cli_expand_list (char* buf, void (*init)(addr_t* arg), char* (*fetch)(addr_t* arg), char* (*fetch_desc)(addr_t* arg), void (*next)(addr_t* arg), int (*done)(addr_t* arg)) {
	int i, match, maxmatch = -1;
	char* val;	
	char* matchval = NULL;
	int pos = kstrlen (buf);
	addr_t arg = 0;

	/* safety first */
	ASSERT (fetch != NULL);
	ASSERT (next != NULL);
	ASSERT (done != NULL);

	/* initialize the list */
	if (init != NULL)
		init (&arg);

	/* keep going while we have arguments */
	while (!done (&arg)) {
		/* calculate match */
		match = 0; val = fetch (&arg);
		for (i = 0; i <= MIN (kstrlen (buf), kstrlen (val)); i++)
			if (buf[i] == val[i])
				match++;
			else
				break;
		if (match < kstrlen (buf))
			match = 0;

		/* better match than before? */
		if ((match > maxmatch) && (match != 0)) {
			/* yes. use this match */
			maxmatch = match;
			matchval = val;
		} else if (match == maxmatch) {
			/* equal match! upcoming space in whatever we're trying to match? */
			if (kstrchr (val + match, ' ') == NULL) {
				/* no. show all possibilities */
				kprintf ("\n");

				/* display all which match */
				init (&arg);
				while (!done (&arg)) {
					/* fetch the value */
					val = fetch (&arg);

					/* match? */
					if (!kstrncmp (val, buf, kstrlen (buf))) {
						/* yes. show the value */
						kprintf ("  %s", val);

						/* got a description? */
						if (fetch_desc != NULL) {
							/* yes. display it */
							i = kstrlen (val);
							while (i++ < CLI_MAX_CMD_LEN) kprintf (" ");
							kprintf ("%s\n", fetch_desc (&arg));
						} else {
							/* no. display the newline */
							kprintf ("\n");
						}
					}

					/* next */
					next (&arg);
				}

				/* re-write the prompt */
				kprintf ("%s> %s", hostname, buf);
				return;
			}
		}

		/* next */
		next (&arg);
	}

	/* got a good match? */
	if (matchval) {
		/* yes. expand this command */
		for (i = maxmatch; i < kstrlen (matchval); i++) {
			kprintf ("%c", matchval[i]);
			buf[pos++] = matchval[i];

			if (matchval[i] == ' ')
				break;
		}
		buf[pos] = 0;
	}
}

/* This will try to expand argument [lastarg] of [buf] with type [type]. It
 * will return zero on failure or non-zero on success.
 */
int
cli_expand_type (char* buf, char* type, char* lastarg) {
	/* driver? */
	if (!kstrcmp (type, "dv")) {
		/* yes. handle it */
		cli_expand_list (lastarg, list_init_drv, list_fetch_drv, list_fetchdesc_drv, list_next_drv, list_done_drv);
		return 1;
	}

	/* interface? */
	if (!kstrcmp (type, "if")) {
		/* yes. handle it */
		cli_expand_list (lastarg, list_init_dev, list_fetch_dev, NULL, list_next_dev, list_done_dev);
		return 1;
	}

	/* boolean? */
	if (!kstrcmp (type, "bl")) {
		/* yes. handle it */
		cli_expand_list (lastarg, list_init_bool, list_fetch_bool, NULL, list_next_bool, list_done_bool);
		return 1;
	}

	/* we can't expand this */
	return 0;
}

/* This will try to expand the arguments of command [cmd] in [buf].
 */
void
cli_expand_args (struct COMMAND* cmd, char* buf) {
	int argno = 0, i;
	int pos = kstrlen (buf);
	char* ptr;
	char* lastarg = "";
	char type[3];
	char* workbuf;
	char desc[CLI_MAX_DESC_LEN];

	/* do we even have any arguments for this command? */
	if (!*cmd->arg)
		/* no. just leave */
		return;

	/* skip past the command */
	workbuf = buf + kstrlen (cmd->cmd);

	/* end of the line here? */
	if (!*workbuf) {
		/* yes. append a space */
		buf[pos++] = ' '; buf[pos] = 0;
		kprintf (" ");
	}

	/* first of all, check at which argument we are */
	argno = 0; ptr = (workbuf + 1);
	while (1) {
		/* skip spaces */
		while (*ptr == ' ') ptr++;
		lastarg = ptr;
		if (!*ptr) break;

		/* skip the argument itself */
		while ((*ptr != ' ') && (*ptr)) ptr++;
		if (!*ptr) break;

		/* next argument reached */
		argno++;
	}

	/* look this argument up */
	ptr = cmd->arg;
	while (argno) {
		/* ensure everything is cozy */
		ASSERT ((*ptr == '%') || (*ptr == '@'));

		/* walk past this argument */
		while (*ptr != '}') ptr++;

		/* skip the space */
		ptr++;
		while (*ptr == ' ') ptr++;

		/* end of the line? */
		if (!*ptr)
			/* yes. no arguments here, bail out */
			return;

		/* next */
		argno--;
	}

	/* now, we got something in the shape %xx{text} in ptr. parse it */
	ASSERT ((*ptr == '%') || (*ptr == '@')); ptr++;
	type[0] = *ptr++; type[1] = *ptr++; type[2] = 0;
	ASSERT (*ptr == '{'); ptr++;
	i = 0; while (*ptr != '}') desc[i++] = *ptr++;
	desc[i] = 0;

	/* do we have an argument here? */
	if (*lastarg) {
		/* yes. try to expand this type */
		if (cli_expand_type (buf, type, lastarg))
			/* this worked. bail out */
			return;
	}

	/* display an overview of this option */
	kprintf ("\n    %s\n", desc);
	kprintf ("%s> %s", hostname, buf);
}

/* This will try to match a command to [buf]. It will return the command on
 * success or NULL on failure.
 */
struct COMMAND*
cli_match_command (char* buf) {
	struct COMMAND* cmd = commands;

	/* try to match up a complete command */
	while (cmd->cmd) {
		/* if the command is too short, skip it */
		if (kstrlen (cmd->cmd) > kstrlen (buf)) {
			cmd++;
			continue;
		}

		/* match? */
		if (!kstrncmp (cmd->cmd, buf, kstrlen (cmd->cmd)))
			/* yes. return it */
			return cmd;

		/* next */
		cmd++;
	}

	/* no matches */
	return NULL;
}

/* This will try to expand the command or parameters in [buf].
 */
void
cli_expand (char* buf) {
	struct COMMAND* cmd;

	/* try to match up a complete command */
	cmd = cli_match_command (buf);
	if (cmd != NULL) {
		/* this worked. expand the arguments */
		cli_expand_args (cmd, buf);
		return;
	}

	/* handle the list of commands */
	cli_expand_list (buf, list_init_cmd, list_fetch_cmd, list_fetchdesc_cmd, list_next_cmd, list_done_cmd);
}

/*
 * This will handle a console command.
 */
int
cli_handle_cmd (char* buf) {
	struct COMMAND* cmd;
	char* ptr;
	char* lastarg;
	char* arg;
	char* tmp;
	int done = 0;
	uint16_t type;
	struct CLI_ARGS args;

	/* first of all, isolate the command itself */
	cmd = cli_match_command (buf);
	if (cmd == NULL) {
		/* this failed. complain */
		kprintf ("unknown command\n");	
		return 0;
	}

	/* zap the argument buffer */
	kmemset (&args, 0, sizeof (struct CLI_ARGS));

	/* handle the arguments */
	buf += kstrlen (cmd->cmd);

	/* do we even have arguments? */
	if (*cmd->arg) {
		/* yes. handle all arguments required */
		ptr = buf; arg = cmd->arg;
		while (!done) {
			/* skip spaces */
			while (*ptr == ' ') ptr++;
			lastarg = ptr;
			if (!*ptr) break;

			/* skip the argument itself */
			while ((*ptr != ' ') && (*ptr)) ptr++;
			done = !*ptr; *ptr = 0; ptr++;

			/* handle the argument with this pack */
			if ((*arg != '%') && (*arg != '@')) {
				/* too much arguments! */
				kprintf ("too much arguments\n");
				return 0;
			}
			arg++;
			type  = (*arg++ << 8);
			type |= (*arg++); /* must be seperate, gcc messes up otherwise! */
			ASSERT (*arg == '{'); arg++;
			while (*arg != '}') arg++;
			arg++;
			while (*arg == ' ') arg++;

			/* wade through the arguments */
			switch (type) {
				case ('i' << 8) | 'f': /* interface */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_INTERFACE;
				                       args.arg[args.num_args].value.dev = device_find (lastarg);
				                       if (args.arg[args.num_args].value.dev == NULL) {
				                         /* no such interface */
				                         kprintf ("no such interface (%s)\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				case ('d' << 8) | 'v': /* driver */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_DRIVER;
				                       args.arg[args.num_args].value.driver = driver_find (lastarg);
				                       if (args.arg[args.num_args].value.driver == NULL) {
				                         /* no such interface */
				                         kprintf ("no such driver (%s)\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				case ('d' << 8) | 'i': /* decimal int */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_INTEGER;
				                       args.arg[args.num_args].value.integer = strtol (lastarg, &tmp, 10);
				                       if (*tmp) {
				                         /* not an integer */
				                         kprintf ("not an integer (%s)\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				case ('h' << 8) | 'i': /* hex int */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_INTEGER;
				                       args.arg[args.num_args].value.integer = strtol (lastarg, &tmp, 16);
				                       if (*tmp) {
				                         /* not an integer */
				                         kprintf ("not an integer (%s)\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				case ('i' << 8) | 'p': /* ipv4 address */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_IPV4ADDRESS;
				                       if (!ip_fetch_addr (&lastarg, &args.arg[args.num_args].value.ipv4addr)) {
				                         /* not an integer */
				                         kprintf ("not an IPv4 address (%s)\n", lastarg);
				                         return 0;
				                       }
				                       if (*lastarg) {
				                         /* not an integer */
				                         kprintf ("not an IPv4 address (%s)\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				case ('s' << 8) | 't': /* string */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_STRING;
				                       args.arg[args.num_args].value.string = lastarg;
				                       args.num_args++;
				                       break;
				case ('b' << 8) | 'l': /* driver */
				                       args.arg[args.num_args].type = CLI_ARGTYPE_BOOLEAN;
				                       args.arg[args.num_args].value.boolean = bool_find (lastarg);
				                       if (args.arg[args.num_args].value.boolean == -1) {
				                         /* no such interface */
				                         kprintf ("%s is not a boolean\n", lastarg);
				                         return 0;
				                       }
				                       args.num_args++;
				                       break;
				              default: /* ??? */
				                       panic ("cli_handle_cmd(): unsupported argument type [%c%c] for string [%s]", (type >> 8), (type & 0xff), lastarg);
			}
		}

		/* still got arguments on the list? */
		if (*arg == '%') {
			/* yes. complain */
			kprintf ("insufficient arguments\n");
			return 0;
		}
	} else {
		/* no arguments required. are some supplied? */
		if (*buf) {
			/* yes. complain */
			kprintf ("too much arguments\n");
			return 0;
		}
	}

	/* looks well! call the function! */
	return cmd->func (&args);
}

/*
 * This will handle the ILIOS Command Line Interface.
 */
void
cli_go() {
	uint8_t ch, pos;
	char console_buf[CLI_MAX_LINE_LEN];

	/* set the default hostname */
	kstrcpy (hostname, DEFAULT_HOSTNAME);

	/* handle it forever */
	for (;;) {
		/* show the prompt and wait for a command */
		kprintf ("%s> ", hostname);

		/* fetch input */
		pos = 0;
		do {
			/* fetch a key */
			ch = 0;
			while (!ch) {
				/* save the processor, we only need to act on interrupts anyway */
				arch_relax();

				/* handle network packets */
				network_handle_queue();

				/* fetch the key */
				ch = arch_console_readch();
			}

			switch (ch) {
				case 8: /* backspace. got chars to burn? */
								if (pos) {
									/* yes. zap the last char */
									console_buf[--pos] = 0;
									arch_console_putchar (8);
									arch_console_putchar (32);
									arch_console_putchar (8);
								}
								break;
				case 9: /* tab. try to complete command / arguments */
								console_buf[pos] = 0;
								cli_expand (console_buf);
								pos = kstrlen (console_buf);
								break;
			 case 13: /* return. leave */
								arch_console_putchar ('\n');
								console_buf[pos] = 0;
								break;
			 case 21: /* ctrl-u. clear the entire line */
								for (; pos > 0; pos--) {
									arch_console_putchar (8);
									arch_console_putchar (32);
									arch_console_putchar (8);
								}
								break;
			 case 23: /* ctrl-w. clear the last word */
								while (pos > 0) {
									pos--;
									arch_console_putchar (8);
									arch_console_putchar (32);
									arch_console_putchar (8);
									if (console_buf[pos] == ' ')
										break;
								}
								break;
			 default: /* dunno. just add it, if we have space */
								if (pos < (CLI_MAX_LINE_LEN - 1)) {
									console_buf[pos++] = ch;
									arch_console_putchar (ch);
								}
			}
		} while (ch != 13);

		/* handle the command */
		cli_handle_cmd (console_buf);
	}
}

/*
 * This will launch script [script]. It will stop whenever the script is
 * finished or an error occours.
 */
void
cli_launch_script (char** script) {
	while (*script) {
		if (!cli_handle_cmd (*script))
			return;
		script++;
	}
}

/* vim:set ts=2 sw=2: */
