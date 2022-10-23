/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008,2009,2011 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "ka-applet-priv.h"
#include "ka-kerberos.h"
#include "ka-dbus.h"

static GDBusConnection *dbus_connection;
static const char *dbus_object_path = "/org/gnome/KrbAuthDialog";
static const char *dbus_interface_name = "org.gnome.KrbAuthDialog";
static GDBusNodeInfo *introspection_data;


gboolean
ka_dbus_acquire_tgt (KaApplet *applet,
                     const gchar *principal)
{
    gboolean success;

    KA_DEBUG ("Getting TGT for '%s'", principal);
    success = ka_check_credentials (applet, principal);
    return success;
}


gboolean
ka_dbus_destroy_ccache (KaApplet *applet)
{
    gboolean success;

    KA_DEBUG ("Destroying ticket cache");
    success = ka_destroy_ccache (applet);
    return success;
}


static const gchar ka_dbus_introspection_xml[] =
    "<node>"
    "  <interface name='org.gnome.KrbAuthDialog'>"
    "    <method name='acquireTgt'>"
    "      <arg type='s' name='principal' direction='in' />"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <method name='destroyCCache'>"
    "      <arg type='b' name='success' direction='out'/>"
    "    </method>"
    "    <signal name='krb_tgt_acquired'>"
    "       <arg type='s' name='principal' direction ='out'/>"
    "       <arg type='u' name='expiry' direction ='out'/>"
    "    </signal>"
    "    <signal name='krb_tgt_renewed'>"
    "       <arg type='s' name='principal' direction ='out'/>"
    "       <arg type='u' name='expiry' direction ='out'/>"
    "    </signal>"
    "    <signal name='krb_tgt_expired'>"
    "       <arg type='s' name='principal' direction ='out'/>"
    "       <arg type='u' name='expiry' direction ='out'/>"
    "    </signal>"
    "  </interface>"
    "</node>";


static void
ka_dbus_handle_method_call (GDBusConnection       *connection G_GNUC_UNUSED,
                            const gchar           *sender G_GNUC_UNUSED,
                            const gchar           *object_path G_GNUC_UNUSED,
                            const gchar           *interface_name G_GNUC_UNUSED,
                            const gchar           *method_name,
                            GVariant              *parameters,
                            GDBusMethodInvocation *invocation,
                            gpointer               user_data)
{
    KaApplet *applet = user_data;
    gboolean ret;

    g_warn_if_fail (applet != NULL);

    if (g_strcmp0 (method_name, "acquireTgt") == 0) {
            const char *principal;

            g_variant_get (parameters, "(s)", &principal);
            ret = ka_dbus_acquire_tgt (applet, principal);
            g_dbus_method_invocation_return_value (invocation,
                                                   g_variant_new("(b)", ret));
    } else if (g_strcmp0 (method_name, "destroyCCache") == 0) {
            ret = ka_dbus_destroy_ccache (applet);
            g_dbus_method_invocation_return_value (invocation,
                                                   g_variant_new("(b)", ret));
    }
}

/* Emit DBus signals */
static void
ka_dbus_signal_cb (gpointer *applet G_GNUC_UNUSED,
                   gchar *princ,
                   guint when, gpointer user_data)
{
    g_autoptr (GError) error = NULL;
    gchar *signal_name = user_data;

    if (!g_dbus_connection_emit_signal (dbus_connection,
                                        NULL,
                                        dbus_object_path,
                                        dbus_interface_name,
                                        signal_name,
                                        g_variant_new ("(su)",
                                                       princ,
                                                       when),
                                        &error)) {
        g_warning ("Failed to emit DBus signal %s: %s",
                   signal_name,
                   error->message);
    }
}


static void
ka_dbus_connect_signals(KaApplet *applet)
{
    g_object_connect (applet,
                      "signal::krb-tgt-acquired", ka_dbus_signal_cb, "krb_tgt_acquired",
                      "signal::krb-tgt-renewed", ka_dbus_signal_cb, "krb_tgt_renewed",
                      "signal::krb-tgt-expired", ka_dbus_signal_cb, "krb_tgt_expired",
                      NULL);
}


static const GDBusInterfaceVTable interface_vtable =
{
  .method_call = ka_dbus_handle_method_call,
};


static gboolean
ka_dbus_register (KaApplet *applet)
{
    guint id;

    introspection_data = g_dbus_node_info_new_for_xml (
        ka_dbus_introspection_xml,
        NULL);

    id = g_dbus_connection_register_object (dbus_connection,
                                        dbus_object_path,
                                        introspection_data->interfaces[0],
                                        &interface_vtable,
                                        applet,
                                        NULL,  /* user_data_free_func */
                                        NULL); /* GError** */

    g_return_val_if_fail(id, FALSE);
    ka_dbus_connect_signals (applet);
    return TRUE;
}


void
ka_dbus_disconnect (void)
{
    if (introspection_data) {
        g_dbus_node_info_unref (introspection_data);
        introspection_data = NULL;
    }

    dbus_connection = NULL;
}


gboolean
ka_dbus_connect (KaApplet *applet)
{
    g_return_val_if_fail (applet != 0, FALSE);

    dbus_connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
    g_return_val_if_fail (dbus_connection != NULL, FALSE);

    return ka_dbus_register(applet);
}

/*
 * vim:ts=4:sts=4:sw=4:et:
 */

