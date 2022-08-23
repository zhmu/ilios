/*
 * cmd.h - ILIOS Command Stuff
 * (c) 2002, 2003 Rink Springer, BSD
 *
 * This file contains helper stuff for main/cmd.c
 *
 */
#ifndef __CMD_H__
#define __CMD_H__

struct COMMAND {
	char* cmd;
	char* desc;
	void (*func)(char* arg);
};

void cmd_handle (char* cmd);

#endif /* __CMD_H__ */
