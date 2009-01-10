INCLUDES = \
	-DGLADEDIR=\""$(gladedir)/"\" \
	-DLOCALE_DIR=\""$(localedir)/"\"

bin_PROGRAMS = krb5-auth-dialog
man_MANS = krb5-auth-dialog.1

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = krb5-auth-dialog.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

schemadir = $(sysconfdir)/gconf/schemas
schema_in_files = $(PACKAGE).schemas.in
schema_DATA = $(PACKAGE).schemas

%.schemas: $(srcdir)/%.schemas.in
	sed -e "s,::PACKAGE::,$(PACKAGE)," < $< > $@

krb5_auth_dialog_SOURCES =	\
	krb5-auth-dialog.c      \
	krb5-auth-dialog.h      \
	krb5-auth-applet.c	\
	krb5-auth-applet.h	\
	krb5-auth-gconf.c	\
	krb5-auth-gconf.h	\
	krb5-auth-notify.c	\
	krb5-auth-notify.h	\
	krb5-auth-dbus.c	\
	krb5-auth-dbus.h	\
	dummy-strings.c

krb5_auth_dialog_LDADD = \
			../gtksecentry/libgtksecentry.a \
			../secmem/libsecmem.a \
			@NETWORK_MANAGER_LIBS@ \
			@KRB5_LIBS@ \
			@LIBNOTIFY_LIBS@ \
			@DBUS_LIBS@ \
			@GCONF_LIBS@ \
			@GLADE_LIBS@ \
			@GTK_LIBS@

AM_CPPFLAGS = -I $(top_srcdir)/gtksecentry/ -I $(top_srcdir)/secmem/

gladedir = $(datadir)/krb5-auth-dialog
glade_DATA =			\
	krb5-auth-dialog.glade

EXTRA_DIST =		\
	$(glade_DATA)   \
	$(schema_in_files) \
	$(autostart_in_files) \
	krb5-auth-dialog.1.in

CLEANFILES = $(schema_DATA)
DISTCLEANFILES = krb5-auth-dialog.desktop
