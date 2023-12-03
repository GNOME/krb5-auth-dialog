/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin-dummy.h"
#include <gmodule.h>

/* Plugin entry point */
G_MODULE_EXPORT KaPlugin *ka_plugin_create (void);

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_dummy_new ());
}

typedef struct _KaPluginDummyPrivate {

    gulong handlers[3];
} KaPluginDummyPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (KaPluginDummy, ka_plugin_dummy, KA_TYPE_PLUGIN)

static void
event_cb (KaApplet *applet, gchar *princ, guint when, gpointer user_data)
{
    g_message ("%s %s @%d", (gchar*)user_data, princ, when);
}

static void
ka_plugin_dummy_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginDummyPrivate *priv = ka_plugin_dummy_get_instance_private (KA_PLUGIN_DUMMY (self));

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
    KaPluginDummyPrivate *priv = ka_plugin_dummy_get_instance_private (KA_PLUGIN_DUMMY (self));

    for (i = 0; i < G_N_ELEMENTS (priv->handlers); i++)
        g_signal_handler_disconnect (applet, priv->handlers[i]);
}

static void
ka_plugin_dummy_class_init (KaPluginDummyClass *klass)
{
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    plugin_class->activate = ka_plugin_dummy_activate;
    plugin_class->deactivate = ka_plugin_dummy_deactivate;
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
