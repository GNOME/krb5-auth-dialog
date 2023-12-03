/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin-pam.h"
#include <gmodule.h>

#include <security/pam_appl.h>

/* Plugin entry point */
G_MODULE_EXPORT KaPlugin *ka_plugin_create (void);

int ka_plugin_major_version = KA_PLUGIN_MAJOR_VERSION;
int ka_plugin_minor_version = KA_PLUGIN_MINOR_VERSION;

G_MODULE_EXPORT KaPlugin *
ka_plugin_create (void)
{
    return KA_PLUGIN (ka_plugin_pam_new ());
}

typedef struct _KaPluginPamPrivate {
    gulong handlers[2];
} KaPluginPamPrivate;

G_DEFINE_TYPE (KaPluginPam, ka_plugin_pam, KA_TYPE_PLUGIN)

static int
simple_conv (int n, const struct pam_message **msg, struct pam_response **resp,
             void *data)
{
    return (PAM_CONV_ERR);
}

static struct pam_conv simplepamconv = { simple_conv, NULL };

static void
renewed_event_cb (KaApplet *applet, gchar *princ, guint when, gpointer user_data)
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
    KaPluginPamPrivate *priv = ka_plugin_pam_get_instance_private (KA_PLUGIN_PAM (self));

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
    KaPluginPamPrivate *priv = ka_plugin_pam_get_instance_private (KA_PLUGIN_PAM (self));

    g_signal_handler_disconnect (applet, priv->handlers[0]);
    g_signal_handler_disconnect (applet, priv->handlers[1]);
}

static void
ka_plugin_pam_class_init (KaPluginPamClass *klass)
{
    KaPluginClass *plugin_class = KA_PLUGIN_CLASS (klass);

    plugin_class->activate = ka_plugin_pam_activate;
    plugin_class->deactivate = ka_plugin_pam_deactivate;
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
