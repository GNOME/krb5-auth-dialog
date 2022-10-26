%{
/*
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ftw.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>
#include "grammar.h"
%}
%x QUOTED
%%
error_table|et		{ yylval.sval = g_strdup(yytext);
			  return ERROR_TABLE_START; };
end			{ return ERROR_TABLE_END; };
error_code|ec		{ yylval.sval = g_strdup(yytext);
			  return ERROR_CODE_START; };
,			{ return COMMA; };
\\\n			{ };
<INITIAL>[\r\n]		{ };
<INITIAL>[A-Za-z0-9_-]+	{ yylval.sval = g_strdup(yytext);
			  return TOKEN; };
<INITIAL>[ \t]		{ };
<INITIAL>\"		{ BEGIN(QUOTED);
			  yylval.sval = g_strdup("");
			  return QUOTE; };
<QUOTED>\"		{ BEGIN(INITIAL);
			  return QUOTE; };
<QUOTED>[^\"]+		{ yylval.sval = g_strdup(yytext);
			  return LITERAL; };
<QUOTED>\n		{ yylval.sval = g_strdup(yytext);
			  return LITERAL; };
<*>^#.*$		{ };
<*>^(error_table_base|index)[ \t]+[0-9]+$ { };
<*>^(prefix|id|error_table_manager)[ \t]+.*$ { };
%%

/* Complete list of filenames, an iterator for that list, and the contents of
 * the current item. */
static GList *filenames = NULL, *filename = NULL;
const char *currentfile = NULL;

int yyerror(void);

int
yyerror(void)
{
	g_print("Syntax error (%s).\n", currentfile);
	exit(1);
}

/* Callback for ftw().  Adds the filename being examined to the global list of
 * filenames. */
static int
fn(const char *file, const struct stat *st, int flag)
{
	int i;
	if (flag == FTW_F) {
		i = strlen(file);
		if ((i > 3) &&
		    (strncmp(file + strlen(file) - 3, ".et", 3) == 0)) {
			filenames = g_list_append(filenames, g_strdup(file));
		}
	}
	return 0;
}

/* Open the next filename in the list of files, if we have a list and we
 * haven't reached its end. */
int
yywrap(void)
{
	if ((filename != NULL) && (g_list_next(filename) != NULL)) {
		fclose(yyin);
		filename = g_list_next(filename);
		currentfile = filename->data;
		yyin = fopen(currentfile, "r");
		return 0;
	}
	return 1;
}

/* Spew forth a gettext .pot header. */
static void
header(void)
{
	const char *boilerplate = "#include <glib/gi18n.h>\n\nconst char *dummy[] = {\n"
		"\t/* Translators: files from dummy-strings.c are *all* possible errors\n"
		"\t   returned from Kerberos (since Kerberos itself doesn't handle i18n). If in\n"
		"\t   doubt skip src/dummy-strings.h as the strings from other files\n"
		"\t   are the ones the user will see in any case. */\n";
	printf("%s", boilerplate);
}

static void
tail(void)
{
	const char *boilerplate = "\tNULL\n};\n";
	printf("%s", boilerplate);
}

int
main(int argc, char **argv)
{
	int i;
	/* Call getopt.  We don't provide any options just now, but this lets
	 * us handle "--help" and "-h" queries simply. */
	while ((i = getopt(argc, argv, "")) != -1) {
		switch (i) {
		default:
			printf("Usage: etpo [directory ...]\n");
			return 2;
			break;
		}
	}
	/* Assume that each non-option argument is a directory. */
	for (i = optind; i < argc; i++) {
		if (ftw(argv[i], fn, 10) != 0) {
			perror("ftw");
			return 1;
		}
	}
	/* Spew out a header. */
	header();
	if (g_list_length(filenames) > 0) {
		/* Open the first file and start parsing it. */
		filename = filenames;
		currentfile = filename->data;
		yyin = fopen(currentfile, "r");
		yyparse();
		fclose(yyin);
	} else {
		/* Start parsing stdin. */
		currentfile = "<stdin>";
		yyin = stdin;
	}
	tail();
	return 0;
}
