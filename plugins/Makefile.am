NULL =

pkglibdir = $(KA_PLUGINS_DIR)

if ENABLE_PAM_PLUGIN
  PAM_PLUGIN = libka-plugin-pam.la
else
  PAM_PLUGIN =
endif

AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(DISABLE_DEPRECATED) \
	-DG_LOG_DOMAIN="\"KrbAuthDialog\"" \
	$(NULL)

pkglib_LTLIBRARIES = \
	libka-plugin-dummy.la \
	$(PAM_PLUGIN) \
	libka-plugin-afs.la \
	libka-plugin-gnomelock.la \
	$(NULL)

# Dummy example plugin
libka_plugin_dummy_la_SOURCES = \
	ka-plugin-dummy.c \
	ka-plugin-dummy.h \
	$(NULL)

libka_plugin_dummy_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(GTK_CFLAGS)  \
	$(NULL)

libka_plugin_dummy_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(NULL)

# PAM plugin
if ENABLE_PAM_PLUGIN
libka_plugin_pam_la_SOURCES = \
	ka-plugin-pam.c \
	ka-plugin-pam.h \
	$(NULL)

libka_plugin_pam_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(GTK_CFLAGS)       \
	$(NULL)

libka_plugin_pam_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(PAM_LIBS) \
	$(NULL)
endif

# AFS plugin
libka_plugin_afs_la_SOURCES = \
	ka-plugin-afs.c \
	ka-plugin-afs.h \
	$(NULL)

libka_plugin_afs_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(GTK_CFLAGS)       \
	$(NULL)

libka_plugin_afs_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(NULL)

# GnomeLock plugin
libka_plugin_gnomelock_la_SOURCES = \
	ka-plugin-gnomelock.c \
	ka-plugin-gnomelock.h \
	$(NULL)

libka_plugin_gnomelock_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(GIO_CFLAGS)       \
	$(NULL)

libka_plugin_gnomelock_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(GIO_LIBS)       \
	$(NULL)
