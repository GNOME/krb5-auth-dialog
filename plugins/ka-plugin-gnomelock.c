/*
 * Copyright (C) 2013 Zoran Pericic <zpericic@netst.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ka-plugin-gnomelock.h"
#include <gmodule.h>
#include <gio/gio.h>

G_DEFINE_TYPE (KaPluginGnomeLock, ka_plugin_gnomelock, KA_TYPE_PLUGIN)
#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN_GNOMELOCK, KaPluginGnomeLockPrivate))

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_gnomelock_new ());
}

typedef struct _KaPluginGnomeLockPrivate KaPluginGnomeLockPrivate;

struct _KaPluginGnomeLockPrivate {
    gulong handler;
};

static void
event_cb (gpointer *applet, gchar *princ, guint when, gpointer user_data)
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
ka_plugin_gnomelock_finalize (GObject *object)
{
    G_OBJECT_CLASS (ka_plugin_gnomelock_parent_class)->finalize (object);
}

static void
ka_plugin_gnomelock_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginGnomeLockPrivate *priv = GET_PRIVATE (self);

    priv->handler = g_signal_connect (applet,
                                          "krb-tgt-expired",
                                          G_CALLBACK (event_cb), "Expired");
}

static void
ka_plugin_gnomelock_deactivate (KaPlugin *self, KaApplet *applet)
{
    int i;
    KaPluginGnomeLockPrivate *priv = GET_PRIVATE (self);

    g_signal_handler_disconnect (applet, priv->handler);
}

static void
ka_plugin_gnomelock_class_init (KaPluginGnomeLockClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KaPluginGnomeLockPrivate));

    plugin_class->activate = ka_plugin_gnomelock_activate;
    plugin_class->deactivate = ka_plugin_gnomelock_deactivate;
    object_class->finalize = ka_plugin_gnomelock_finalize;
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
