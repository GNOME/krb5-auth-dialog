noinst_PROGRAMS = etpo
etpo_SOURCES = lexer.l grammar.y
lexer.c: grammar.h
AM_YFLAGS=-d
AM_CFLAGS=@GNOME_CFLAGS@
AM_LDFLAGS=@GNOME_LIBS@
DISTCLEANFILES=lexer.c

