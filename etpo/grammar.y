%{
/*
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "config.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
static const char *table = "unknown";
extern const char *currentfile;
%}
%union {
	char *sval;
	int ival;
}
%token ERROR_TABLE_START
%token ERROR_TABLE_END
%token ERROR_CODE_START
%token COMMA
%token QUOTE
%token <sval>EMPTY
%token <sval>TOKEN
%token <sval>LITERAL
%type  <sval>literal
%%
statements: statement | statements statement ;
statement: table_start | error_code | table_end ;
table_start: ERROR_TABLE_START TOKEN { table = $2; };
literal: literal LITERAL { $$ = g_strconcat($1, $2, NULL);} | LITERAL;
error_code: ERROR_CODE_START TOKEN COMMA QUOTE literal QUOTE {
		{
			gchar **lines;
			int i;
			const char *p;
			/* Find the basename of the current file. */
			if (strchr(currentfile, '/') != NULL) {
				p = strrchr(currentfile, '/') + 1;
			} else {
				p = currentfile;
			}
			printf("\tN_(\"%s\"),\t/* %s:%s:%s */\n", $5, p, table, $2);
		}
	} |
	ERROR_CODE_START TOKEN COMMA QUOTE QUOTE {
	};
table_end: ERROR_TABLE_END { table = "unknown"; };
