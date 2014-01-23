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

#include "ka-plugin-pam.h"
#include <gmodule.h>

#include <security/pam_appl.h>

G_DEFINE_TYPE (KaPluginPam, ka_plugin_pam, KA_TYPE_PLUGIN)
#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN_PAM, KaPluginPamPrivate))

typedef struct _KaPluginPamPrivate KaPluginPamPrivate;

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_pam_new ());
}

struct _KaPluginPamPrivate {
    gulong handlers[2];
};

static void
ka_plugin_pam_finalize (GObject *object)
{
    G_OBJECT_CLASS (ka_plugin_pam_parent_class)->finalize (object);
}

static int
simple_conv (int n, const struct pam_message **msg, struct pam_response **resp,
             void *data)
{
    return (PAM_CONV_ERR);
}

static struct pam_conv simplepamconv = { simple_conv, NULL };

static void
renewed_event_cb (gpointer *applet, gchar *princ, guint when,
                  gpointer user_data)
{
    const char *user;
    pam_handle_t *pamh = NULL;
    int retval = 0;

    user = g_get_user_name ();
    retval = pam_start ("ka-plugin-pam", user, &simplepamconv, &pamh);
    if (retval)
        goto out;

    retval = pam_setcred (pamh, PAM_ESTABLISH_CRED);
    if (retval)
        goto out;

out:
    if (retval)
        g_warning ("PAM plugin: %s", pam_strerror (pamh, retval));

    if (pamh)
        pam_end (pamh, PAM_SUCCESS);
}

static void
ka_plugin_pam_activate (KaPlugin *self, KaApplet *applet)
{
    KaPluginPamPrivate *priv = GET_PRIVATE (self);

    priv->handlers[1] = g_signal_connect (applet,
                                          "krb-tgt-acquired",
                                          G_CALLBACK (renewed_event_cb), NULL);
    priv->handlers[0] = g_signal_connect (applet,
                                          "krb-tgt-renewed",
                                          G_CALLBACK (renewed_event_cb), NULL);
}


static void
ka_plugin_pam_deactivate (KaPlugin *self, KaApplet *applet)
{
    KaPluginPamPrivate *priv = GET_PRIVATE (self);

    g_signal_handler_disconnect (applet, priv->handlers[0]);
    g_signal_handler_disconnect (applet, priv->handlers[1]);
}

static void
ka_plugin_pam_class_init (KaPluginPamClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KaPluginPamPrivate));

    plugin_class->activate = ka_plugin_pam_activate;
    plugin_class->deactivate = ka_plugin_pam_deactivate;
    object_class->finalize = ka_plugin_pam_finalize;
}

static void
ka_plugin_pam_init (KaPluginPam *self)
{
}

KaPluginPam *
ka_plugin_pam_new (void)
{
    return g_object_new (KA_TYPE_PLUGIN_PAM, KA_PLUGIN_PROP_NAME, "pam", NULL);
}
