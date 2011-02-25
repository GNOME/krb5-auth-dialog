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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ka-plugin-afs.h"
#include <gmodule.h>

G_DEFINE_TYPE (KaPluginAfs, ka_plugin_afs, KA_TYPE_PLUGIN)
#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN_AFS, KaPluginAfsPrivate))

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_afs_new ());
}

typedef struct _KaPluginAfsPrivate KaPluginAfsPrivate;

struct _KaPluginAfsPrivate {
    gulong handlers[2];
};

static void
event_cb (gpointer *applet, gchar *princ, guint when, gpointer user_data G_GNUC_UNUSED)
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
ka_plugin_afs_finalize (GObject *object)
{
    G_OBJECT_CLASS (ka_plugin_afs_parent_class)->finalize (object);
}

static void
ka_plugin_afs_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginAfsPrivate *priv = GET_PRIVATE (self);

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
    KaPluginAfsPrivate *priv = GET_PRIVATE (self);

    for (i = 0; i < G_N_ELEMENTS (priv->handlers); i++)
        g_signal_handler_disconnect (applet, priv->handlers[i]);
}

static void
ka_plugin_afs_class_init (KaPluginAfsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KaPluginAfsPrivate));

    plugin_class->activate = ka_plugin_afs_activate;
    plugin_class->deactivate = ka_plugin_afs_deactivate;
    object_class->finalize = ka_plugin_afs_finalize;
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
