/*
 *     NEARDAL Tester command line interpreter
 *
 *     Copyright 2012 Intel Corporation. All rights reserved.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License version 2
 *     as published by the Free Software Foundation.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software Foundation,
 *     Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __NCL_CMD_H__
#define __NCL_CMD_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "neardal.h"
#include "ncl.h"

/* Command Line Interpretor context... */
typedef struct {
	/* NEARDAL Callback already initialized? */
	gboolean	cb_initialized;

	/* command line interpretor context */
	GString		*clBuf;		/* Command line buffer */

} NCLCmdContext;

/* Array prototype of command line functions interpretor */
typedef	NCLError(*ncl_cmd_func)(int argc, char *argv[]);
typedef struct {
	char		*cmdName;	/* Command name */
	ncl_cmd_func	func;		/* Address of processing function */
	char		*helpStr;	/* Minimal help */
} NCLCmdInterpretor;

/* Initialize/Destroy command line interpretor context */
NCLError		ncl_cmd_init(char *execCmdLineStr);
void			ncl_cmd_finalize(void);

/* Return command line functions */
NCLCmdInterpretor	*ncl_cmd_get_list(int *nbCmd);
int			ncl_cmd_get_nbCmd(void);
NCLCmdContext		*ncl_cmd_get_ctx(void);

/* Print out used by command line functions (and prompt) */
void			ncl_cmd_print(FILE *fprintout, char *format, ...);

#define		NCL_CMD_PRINT(format, ...) \
			ncl_cmd_print(stdout, format, ## __VA_ARGS__)

#define		NCL_CMD_PRINTF(format, ...) \
			ncl_cmd_print(stdout, "%s(): " format, __func__, \
			## __VA_ARGS__)

#define		NCL_CMD_PRINTIN() \
			ncl_cmd_print(stdout, "%s(): Processing...\n", \
			__func__)

#define		NCL_CMD_PRINTERR(format, ...) \
			ncl_cmd_print(stderr, "ERR in %s(): " format, \
			__func__, ## __VA_ARGS__)

#endif /* __NCL_CMD_H__ */
