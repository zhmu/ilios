/*
 * cli.h - ILIOS Command Line Interface
 * (c) 2002-2004 Rink Springer, BSD
 *
 * This file contains helper stuff for CLI-using functions.
 *
 */
#ifndef __CLI_H__
#define __CLI_H__

#define CLI_MAX_LINE_LEN	128
#define CLI_MAX_ARGS 8
#define CLI_MAX_CMD_LEN  40
#define CLI_MAX_DESC_LEN 40
#define CLI_MAX_ARGNAME_LEN 40

struct NIC_DRIVER {
	char*	name;
	char*	desc;
	int   (*init)(char* name, struct DEVICE_RESOURCES* res);
};

struct BOOL_VALUE {
	char* name;
	int	  value;
};

#define CLI_ARGTYPE_UNUSED       0
#define CLI_ARGTYPE_INTERFACE    1
#define CLI_ARGTYPE_DRIVER       2
#define CLI_ARGTYPE_INTEGER      3
#define CLI_ARGTYPE_IPV4ADDRESS  4
#define CLI_ARGTYPE_STRING       5
#define CLI_ARGTYPE_BOOLEAN      6

struct CLI_ARG {
	uint8_t  type;

	union {
		struct DEVICE* dev;
		struct NIC_DRIVER* driver;
		uint32_t integer;
		uint32_t ipv4addr;
    char*    string;
		int      boolean;
	} value;
};

struct CLI_ARGS {
	int num_args;
	struct CLI_ARG arg[CLI_MAX_ARGS];
};

struct COMMAND {
	char* cmd;
	char* desc;
	char* arg;
	int  (*func)(struct CLI_ARGS* args);
};

extern char hostname[HOSTNAME_LEN];
extern struct COMMAND commands[];

void cli_go();
void cli_launch_script (char** script);

#endif /* __CLI_H__ */

/* vim:set ts=2 sw=2: */
