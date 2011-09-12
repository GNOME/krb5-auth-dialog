NULL =

bin_PROGRAMS = krb5-auth-dialog
man_MANS = krb5-auth-dialog.1

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = krb5-auth-dialog.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = krb5-auth-dialog.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

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
	ka-kerberos.c      	\
	ka-kerberos.h      	\
	ka-applet.c		\
	ka-applet-priv.h	\
	ka-applet.h		\
	ka-pwdialog.c    	\
	ka-pwdialog.h    	\
	ka-gconf.c		\
	ka-gconf.h		\
	ka-gconf-tools.c	\
	ka-gconf-tools.h	\
	ka-dbus.c		\
	ka-dbus.h		\
	ka-tools.c		\
	ka-tools.h		\
	ka-main-window.c	\
	ka-main-window.h	\
	ka-plugin.c		\
	ka-plugin.h		\
	ka-plugin-loader.c	\
	ka-plugin-loader.h	\
	ka-preferences.c	\
	ka-preferences.h	\
	ka-closures.h		\
	ka-closures.c		\
	ka-entry-buffer.h	\
	ka-entry-buffer.c	\
	dummy-strings.c		\
	$(NULL)

krb5_auth_dialog_CPPFLAGS = \
	-I$(top_srcdir)/secmem \
	-DKA_DATA_DIR=\""$(pkgdatadir)"\" \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALE_DIR=\""$(localedir)/"\" \
	-DBIN_DIR=\""$(bindir)/"\" \
	-DKA_PLUGINS_DIR=\""$(KA_PLUGINS_DIR)"\" \
	$(AM_CPPFLAGS) \
	$(NULL)

krb5_auth_dialog_CFLAGS = \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(KRB5_CFLAGS) \
	$(NETWORK_MANAGER_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

krb5_auth_dialog_LDADD = \
	$(top_builddir)/secmem/libsecmem.a \
	$(NETWORK_MANAGER_LIBS) \
	$(LIBCAP) \
	$(KRB5_LIBS) \
	$(LIBNOTIFY_LIBS) \
	$(GCONF_LIBS) \
	$(GIO_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(NULL)

AM_CPPFLAGS = \
	-I $(top_srcdir)/secmem/ \
	$(DISABLE_DEPRECATED) \
	$(NULL)

pkgdatadir = $(datadir)/krb5-auth-dialog
pkgdata_DATA = \
	krb5-auth-dialog.ui \
	$(NULL)

EXTRA_DIST = \
	$(pkgdata_DATA) \
	$(schema_in_files) \
	$(autostart_in_files) \
	$(service_in_files) \
	krb5-auth-dialog.1.in \
	$(NULL)

CLEANFILES = $(schema_DATA) $(service_DATA)
DISTCLEANFILES = \
	krb5-auth-dialog.desktop \
	$(NULL)

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
		GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
	done \
	fi
endif

