/*
 * Copyright (C) 2013 Zoran Pericic <zpericic@netst.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin-gnomelock.h"
#include <gmodule.h>
#include <gio/gio.h>

/* Plugin entry point */
G_MODULE_EXPORT KaPlugin *ka_plugin_create (void);

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_gnomelock_new ());
}

typedef struct _KaPluginGnomeLockPrivate {
    gulong handler;
} KaPluginGnomeLockPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (KaPluginGnomeLock, ka_plugin_gnomelock, KA_TYPE_PLUGIN)

static void
event_cb (KaApplet *applet, gchar *princ, guint when, gpointer user_data)
{
    GError *error = NULL;
    GDBusProxyFlags flags= G_DBUS_PROXY_FLAGS_NONE;
    GDBusProxy *proxy = NULL;

    proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                            flags,
                                            NULL, /* GDBusInterfaceInfo */
                                            "org.gnome.ScreenSaver",
                                            "/org/gnome/ScreenSaver",
                                            "org.gnome.ScreenSaver",
                                            NULL, /* GCancellable */
                                            &error
                                             );

    if (proxy == NULL) {
        g_warning ("Error creating proxy: %s\n", error->message);
        g_error_free (error);
        return;
    }

    g_dbus_proxy_call_sync (proxy,
                                            "Lock",
                                            NULL,
                                            G_DBUS_CALL_FLAGS_NONE,
                                            -1,
                                            NULL,
                                            &error);

    if (error) {
        g_warning ("Error calling lock: %s\n", error->message);
        g_error_free (error);
    }

    if (proxy != NULL)
        g_object_unref (proxy);
}

static void
ka_plugin_gnomelock_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginGnomeLockPrivate *priv =
        ka_plugin_gnomelock_get_instance_private (KA_PLUGIN_GNOMELOCK (self));

    priv->handler = g_signal_connect (applet,
                                          "krb-tgt-expired",
                                          G_CALLBACK (event_cb), "Expired");
}

static void
ka_plugin_gnomelock_deactivate (KaPlugin *self, KaApplet *applet)
{
    KaPluginGnomeLockPrivate *priv =
        ka_plugin_gnomelock_get_instance_private (KA_PLUGIN_GNOMELOCK (self));

    g_signal_handler_disconnect (applet, priv->handler);
}

static void
ka_plugin_gnomelock_class_init (KaPluginGnomeLockClass *klass)
{
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    plugin_class->activate = ka_plugin_gnomelock_activate;
    plugin_class->deactivate = ka_plugin_gnomelock_deactivate;
}

static void
ka_plugin_gnomelock_init (KaPluginGnomeLock *self)
{
}

KaPluginGnomeLock *
ka_plugin_gnomelock_new (void)
{
    return g_object_new (KA_TYPE_PLUGIN_GNOMELOCK, KA_PLUGIN_PROP_NAME,
                         "gnomelock", NULL);
}
