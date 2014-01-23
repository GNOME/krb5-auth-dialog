/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "ka-plugin-dummy.h"
#include <gmodule.h>

G_DEFINE_TYPE (KaPluginDummy, ka_plugin_dummy, KA_TYPE_PLUGIN)
#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN_DUMMY, KaPluginDummyPrivate))

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_dummy_new ());
}

typedef struct _KaPluginDummyPrivate KaPluginDummyPrivate;

struct _KaPluginDummyPrivate {
    gulong handlers[3];
};

static void
event_cb (gpointer *applet, gchar *princ, guint when, gpointer user_data)
{
    g_message ("%s %s @%d", user_data, princ, when);
}

static void
ka_plugin_dummy_finalize (GObject *object)
{
    G_OBJECT_CLASS (ka_plugin_dummy_parent_class)->finalize (object);
}

static void
ka_plugin_dummy_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginDummyPrivate *priv = GET_PRIVATE (self);

    priv->handlers[0] = g_signal_connect (applet,
                                          "krb-tgt-expired",
                                          G_CALLBACK (event_cb), "Expired");
    priv->handlers[1] = g_signal_connect (applet,
                                          "krb-tgt-acquired",
                                          G_CALLBACK (event_cb), "Acquired");
    priv->handlers[2] = g_signal_connect (applet,
                                          "krb-tgt-renewed",
                                          G_CALLBACK (event_cb), "Renewed");
}

static void
ka_plugin_dummy_deactivate (KaPlugin *self, KaApplet *applet)
{
    int i;
    KaPluginDummyPrivate *priv = GET_PRIVATE (self);

    for (i = 0; i < G_N_ELEMENTS (priv->handlers); i++)
        g_signal_handler_disconnect (applet, priv->handlers[i]);
}

static void
ka_plugin_dummy_class_init (KaPluginDummyClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KaPluginDummyPrivate));

    plugin_class->activate = ka_plugin_dummy_activate;
    plugin_class->deactivate = ka_plugin_dummy_deactivate;
    object_class->finalize = ka_plugin_dummy_finalize;
}

static void
ka_plugin_dummy_init (KaPluginDummy *self)
{
}

KaPluginDummy *
ka_plugin_dummy_new (void)
{
    return g_object_new (KA_TYPE_PLUGIN_DUMMY, KA_PLUGIN_PROP_NAME,
                         "dummy", NULL);
}
