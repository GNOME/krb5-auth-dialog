NULL =

pkglibdir = $(KA_PLUGINS_DIR)

pkglib_LTLIBRARIES = \
	libka-plugin-dummy.la \
	$(NULL)

# Dummy example plugin
libka_plugin_dummy_la_SOURCES = \
	ka-plugin-dummy.c \
	ka-plugin-dummy.h \
	$(NULL)

libka_plugin_dummy_la_CPPFLAGS = \
	$(GTK_CFLAGS)       \
	-I$(top_srcdir)/src \
	$(NULL)

libka_plugin_dummy_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(NULL)
