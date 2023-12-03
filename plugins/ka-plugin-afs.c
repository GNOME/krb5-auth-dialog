/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin-afs.h"
#include <gmodule.h>

/* Plugin entry point */
G_MODULE_EXPORT KaPlugin *ka_plugin_create (void);

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_afs_new ());
}

typedef struct _KaPluginAfsPrivate {
    gulong handlers[2];
} KaPluginAfsPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (KaPluginAfs, ka_plugin_afs, KA_TYPE_PLUGIN)

static void
event_cb (KaApplet *applet, gchar *princ, guint when, gpointer user_data G_GNUC_UNUSED)
{
    GError *err = NULL;
    gboolean ret;
    int i;
    const char *afslog_cmds[] = { "aklog", "afslog" };

    for (i = 0; i < G_N_ELEMENTS (afslog_cmds); i++) {
        ret = g_spawn_command_line_async (afslog_cmds[i], &err);
        if (!ret) {
            if (G_SPAWN_ERROR_NOENT != err->code)
                g_warning ("%s", err->message);
        } else
            break;
        g_clear_error (&err);
    }

    if (!ret)
        g_warning ("Couldn't run any afslog command");
}

static void
ka_plugin_afs_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginAfsPrivate *priv = ka_plugin_afs_get_instance_private (KA_PLUGIN_AFS (self));

    priv->handlers[0] = g_signal_connect (applet,
                                          "krb-tgt-acquired",
                                          G_CALLBACK (event_cb), NULL);
    priv->handlers[1] = g_signal_connect (applet,
                                          "krb-tgt-renewed",
                                          G_CALLBACK (event_cb), NULL);
}

static void
ka_plugin_afs_deactivate (KaPlugin *self, KaApplet *applet)
{
    int i;
    KaPluginAfsPrivate *priv = ka_plugin_afs_get_instance_private (KA_PLUGIN_AFS (self));

    for (i = 0; i < G_N_ELEMENTS (priv->handlers); i++)
        g_signal_handler_disconnect (applet, priv->handlers[i]);
}

static void
ka_plugin_afs_class_init (KaPluginAfsClass *klass)
{
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    plugin_class->activate = ka_plugin_afs_activate;
    plugin_class->deactivate = ka_plugin_afs_deactivate;
}

static void
ka_plugin_afs_init (KaPluginAfs *self)
{
}

KaPluginAfs *
ka_plugin_afs_new (void)
{
    return g_object_new (KA_TYPE_PLUGIN_AFS, KA_PLUGIN_PROP_NAME, "afs", NULL);
}
