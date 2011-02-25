/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2009,2011 Guido Guenther <agx@sigxcpu.org>
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
 *
 */

#include "config.h"

#include "ka-applet-priv.h"
#include "ka-dialog.h"
#include "ka-dbus.h"

static guint dbus_owner_id;
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

static gchar* ka_dbus_signal_name (const gchar *name)
{
    gchar *c;
    gchar *signal_name = g_strdup(name);

   /* The DBus signal names use underscores */
   for (c = signal_name; *c != '\0'; c++ ) {
       if (*c == '-')
           *c = '_';
   }

   return signal_name;
}

/* Emit DBus signals */
static void
ka_dbus_signal_cb (gpointer *applet G_GNUC_UNUSED,
                   gchar *princ,
                   guint when, gpointer user_data)
{
    GError *error = NULL;
    gchar *signal_name;

    signal_name = ka_dbus_signal_name(user_data);
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
        g_clear_error (&error);
    }
    g_free (signal_name);
}


static void
ka_dbus_connect_signals(KaApplet *applet)
{
    int i;

    for (i = 0; i < KA_SIGNAL_COUNT; i++) {
        g_signal_connect (applet, ka_signal_names[i],
                          G_CALLBACK (ka_dbus_signal_cb),
                          (gpointer)ka_signal_names[i]);
    }
}


static const GDBusInterfaceVTable interface_vtable =
{
  .method_call = ka_dbus_handle_method_call,
};


static void
ka_dbus_on_name_acquired (GDBusConnection *connection,
                          const gchar *name G_GNUC_UNUSED,
                          gpointer user_data)
{
    KaApplet *applet = user_data;
    guint id;

    introspection_data = g_dbus_node_info_new_for_xml (
        ka_dbus_introspection_xml,
        NULL);

    id = g_dbus_connection_register_object (connection,
                                        "/org/gnome/KrbAuthDialog",
                                        introspection_data->interfaces[0],
                                        &interface_vtable,
                                        applet,
                                        NULL,  /* user_data_free_func */
                                        NULL); /* GError** */
    if (!id)
        g_error ("Failed to regiester DBus object");
    ka_dbus_connect_signals (applet);

    dbus_connection = connection;
}


static void
ka_dbus_on_name_lost (GDBusConnection *connection G_GNUC_UNUSED,
                      const gchar *name G_GNUC_UNUSED,
                      gpointer user_data)
{
    KaApplet *applet = user_data;

    g_warning ("Cannot acquire DBUS name");
    ka_applet_destroy (applet);
}


void
ka_dbus_disconnect ()
{
    if (introspection_data) {
        g_dbus_node_info_unref (introspection_data);
        introspection_data = NULL;
    }

    if (dbus_owner_id) {
        g_bus_unown_name (dbus_owner_id);
        dbus_owner_id = 0;
    }
    dbus_connection = NULL;
}


gboolean
ka_dbus_connect (KaApplet *applet)
{
    g_return_val_if_fail (applet != 0, FALSE);

    dbus_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                    "org.gnome.KrbAuthDialog",
                                    G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                                    NULL,
                                    ka_dbus_on_name_acquired,
                                    ka_dbus_on_name_lost,
                                    applet,
                                    NULL);
    return TRUE;
}

/*
 * vim:ts=4:sts=4:sw=4:et:
 */
