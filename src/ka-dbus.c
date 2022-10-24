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

#define DBUS_INTERFACE_NAME "org.gnome.KrbAuthDialog"

typedef struct {
    GDBusConnection *connection;
    const char      *object_path;
    GDBusNodeInfo   *introspection_data;
    guint            obj_id;
} DBusData;
static DBusData dbus_data;

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
ka_dbus_signal_cb (KaApplet *applet, gchar *princ, guint when, gpointer  user_data)
{
    g_autoptr (GError) error = NULL;
    gchar *signal_name = user_data;

    if (!g_dbus_connection_emit_signal (dbus_data.connection,
                                        NULL,
                                        dbus_data.object_path,
                                        DBUS_INTERFACE_NAME,
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


static const GDBusInterfaceVTable interface_vtable =
{
  .method_call = ka_dbus_handle_method_call,
};


void
ka_dbus_disconnect (void)
{
    if (dbus_data.obj_id) {
        g_dbus_connection_unregister_object (dbus_data.connection, dbus_data.obj_id);
        dbus_data.obj_id = 0;
    }

    if (dbus_data.introspection_data) {
        g_dbus_node_info_unref (dbus_data.introspection_data);
        dbus_data.introspection_data = NULL;
    }

    dbus_data.connection = NULL;
    dbus_data.object_path = NULL;
}


gboolean
ka_dbus_connect (KaApplet *applet, GDBusConnection *connection, const char *object_path)
{
    g_autoptr (GError) err = NULL;

    g_return_val_if_fail (applet != 0, FALSE);
    g_return_val_if_fail (connection, FALSE);
    g_return_val_if_fail (object_path, FALSE);

    dbus_data.connection = connection;
    dbus_data.object_path = object_path;
    dbus_data.introspection_data = g_dbus_node_info_new_for_xml (ka_dbus_introspection_xml,
                                                                 NULL);

    dbus_data.obj_id = g_dbus_connection_register_object (dbus_data.connection,
                                                          object_path,
                                                          dbus_data.introspection_data->interfaces[0],
                                                          &interface_vtable,
                                                          applet,
                                                          NULL,
                                                          &err);
    if (dbus_data.obj_id == 0) {
        g_warning ("Failed to register DBus object: %s", err->message);
        return FALSE;
    }

    g_object_connect (applet,
                      "signal::krb-tgt-acquired", ka_dbus_signal_cb, "krb_tgt_acquired",
                      "signal::krb-tgt-renewed", ka_dbus_signal_cb, "krb_tgt_renewed",
                      "signal::krb-tgt-expired", ka_dbus_signal_cb, "krb_tgt_expired",
                      NULL);

    return TRUE;
}

/*
 * vim:ts=4:sts=4:sw=4:et:
 */
