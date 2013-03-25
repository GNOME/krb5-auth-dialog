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

gsettings_SCHEMAS = org.gnome.KrbAuthDialog.gschema.xml
@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
dist_convert_DATA = org.gnome.KrbAuthDialog.convert

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.gnome.KrbAuthDialog.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

@INTLTOOL_XML_RULE@
appdatadir       = $(datadir)/appdata
appdata_in_files = krb5-auth-dialog.appdata.xml.in
appdata_DATA     = $(appdata_in_files:.xml.in=.xml)

krb5_auth_dialog_SOURCES =	\
	ka-kerberos.c      	\
	ka-kerberos.h      	\
	ka-applet.c		\
	ka-applet-priv.h	\
	ka-applet.h		\
	ka-pwdialog.c    	\
	ka-pwdialog.h    	\
	ka-settings.c		\
	ka-settings.h		\
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
	resources.c		\
	$(NULL)

krb5_auth_dialog_CPPFLAGS = \
	-I$(top_srcdir)/secmem \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALE_DIR=\""$(localedir)/"\" \
	-DBIN_DIR=\""$(bindir)/"\" \
	-DKA_PLUGINS_DIR=\""$(KA_PLUGINS_DIR)"\" \
	$(AM_CPPFLAGS) \
	$(NULL)

krb5_auth_dialog_CFLAGS = \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
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
	$(GIO_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(NULL)

AM_CPPFLAGS = \
	-I $(top_srcdir)/secmem/ \
	$(DISABLE_DEPRECATED) \
	-DG_LOG_DOMAIN="\"KrbAuthDialog\"" \
	$(NULL)

EXTRA_DIST = \
	$(schema_in_files) \
	$(autostart_in_files) \
	$(service_in_files) \
	$(appdata_in_files) \
	org.gnome.KrbAuthDialog.gschema.xml \
	krb5-auth-dialog.1.in \
        krb5-auth-dialog.gresource.xml \
	resources/ui/app-menu.ui \
	resources/ui/ka-main-window.ui \
	resources/ui/ka-preferences.ui \
	resources/ui/ka-pwdialog.ui \
	resources/ui/tray-icon-menu.ui \
	$(NULL)

CLEANFILES = $(schema_DATA) \
	$(service_DATA) \
	$(appdata_DATA) \
	resources.c \
	$(NULL)

DISTCLEANFILES = \
	krb5-auth-dialog.desktop \
	$(NULL)

BUILT_SOURCES = \
        resources.c \
	$(NULL)

resources.c: krb5-auth-dialog.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir)/resources --generate-dependencies $(srcdir)/krb5-auth-dialog.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/krb5-auth-dialog.gresource.xml \
	    --target=$@ --sourcedir=$(srcdir)/resources --generate-source
