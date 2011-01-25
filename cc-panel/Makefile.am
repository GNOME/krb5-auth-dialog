NULL =

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libka.la

libka_la_SOURCES = \
	ka-module.c \
	cc-ka-panel.c \
	$(top_srcdir)/src/ka-gconf-tools.c \
	$(top_srcdir)/src/ka-gconf-tools.h \
	$(top_srcdir)/src/ka-tools.c \
	$(top_srcdir)/src/ka-tools.h \
	cc-ka-panel.h

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
        $(CONTROL_CENTER_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	-DKA_DATA_DIR=\""$(pkgdatadir)"\" \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALE_DIR=\""$(localedir)/"\" \
	$(NULL)

libka_la_LIBADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS) \
	$(CONTROL_CENTER_LIBS) \
	$(NULL)

AM_CPPFLAGS = \
	$(DISABLE_DEPRECATED) \
	$(NULL)

libka_la_LDFLAGS = -export_dynamic -avoid-version -module -no-undefined -export-symbols-regex '^g_io_module_(load|unload)'

desktopdir = $(datadir)/applications
desktop_in_files = gnome-ka-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

pkgdatadir = $(datadir)/krb5-auth-dialog
pkgdata_DATA =			\
	ka-panel.ui \
	$(NULL)

CLEANFILES = $(desktop_DATA)

EXTRA_DIST = \
	$(desktop_in_files) \
	$(pkgdata_DATA) \
	$(NULL)
