%{
/*
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"
#include <glib.h>
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
			printf("\n# %s:%s:%s\n", p, table, $2);
			/* If the string doesn't contain a newline, just print
			 * it on a single line. */
			if (strchr($5, '\n') == NULL) {
				printf("msgid \"%s\"\n", $5);
			} else {
				/* Split the string up along newline
				 * boundaries, for readability. */
				lines = g_strsplit($5, "\n", -1);
				if (lines != NULL) {
					printf("msgid \n");
					for (i = 0; lines[i] != NULL; i++) {
						printf("\"%s\"\n", lines[i]);
					}
					g_strfreev(lines);
				}
			}
			/* Output the template translation. */
			printf("msgstr \"\"\n");
		}
	} |
	ERROR_CODE_START TOKEN COMMA QUOTE QUOTE {
	};
table_end: ERROR_TABLE_END { table = "unknown"; };
