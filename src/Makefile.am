NULL =

bin_PROGRAMS = krb5-auth-dialog
man_MANS = krb5-auth-dialog.1

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = krb5-auth-dialog.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = krb5-auth-dialog.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.gnome.KrbAuthDialog.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

krb5_auth_dialog_SOURCES =	\
	krb5-auth-dialog.c      \
	krb5-auth-dialog.h      \
	krb5-auth-applet.c	\
	krb5-auth-applet.h	\
	krb5-auth-pwdialog.c    \
	krb5-auth-pwdialog.h    \
	krb5-auth-gconf.c	\
	krb5-auth-gconf.h	\
	krb5-auth-gconf-tools.c	\
	krb5-auth-gconf-tools.h	\
	krb5-auth-dbus.c	\
	krb5-auth-dbus.h	\
	krb5-auth-tools.c	\
	krb5-auth-tools.h	\
	dummy-strings.c		\
	$(NULL)

krb5_auth_dialog_CPPFLAGS = \
	-I$(top_srcdir)/gtksecentry \
	-I$(top_srcdir)/secmem \
	-DKA_DATA_DIR=\""$(pkgdatadir)"\" \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALE_DIR=\""$(localedir)/"\" \
	-DBIN_DIR=\""$(bindir)/"\" \
	$(AM_CPPFLAGS) \
	$(NULL)

krb5_auth_dialog_CFLAGS = \
	$(GTK_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(KRB5_CFLAGS) \
	$(NETWORK_MANAGER_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

krb5_auth_dialog_LDADD = \
	$(top_builddir)/gtksecentry/libgtksecentry.a \
	$(top_builddir)/secmem/libsecmem.a \
	$(NETWORK_MANAGER_LIBS) \
	$(LIBCAP) \
	$(KRB5_LIBS) \
	$(LIBNOTIFY_LIBS) \
	$(DBUS_LIBS) \
	$(GCONF_LIBS) \
	$(GIO_LIBS) \
	$(GTK_LIBS) \
	$(NULL)

BUILT_SOURCES = \
	krb5-auth-applet-dbus-glue.h \
	$(NULL)

krb5-auth-applet-dbus-glue.h: $(srcdir)/krb5-auth-applet-dbus.xml
	dbus-binding-tool                 \
		--mode=glib-server        \
		--prefix=krb5_auth_dialog \
		$< > $@

AM_CPPFLAGS = \
	-I $(top_srcdir)/gtksecentry/ \
	-I $(top_srcdir)/secmem/ \
	$(DISABLE_DEPRECATED) \
	$(NULL)

pkgdatadir = $(datadir)/krb5-auth-dialog
pkgdata_DATA = \
	krb5-auth-dialog.xml \
	$(NULL)

EXTRA_DIST = \
	$(pkgdata_DATA) \
	$(schema_in_files) \
	$(autostart_in_files) \
	$(service_in_files) \
	$(srcdir)/krb5-auth-applet-dbus.xml \
	krb5-auth-dialog.1.in \
	$(NULL)

CLEANFILES = $(schema_DATA) $(service_DATA)
DISTCLEANFILES = \
	krb5-auth-dialog.desktop \
	krb5-auth-applet-dbus-glue.h \
	$(NULL)

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
	done \
	fi
endif

