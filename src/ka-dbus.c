/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2009 Guido Guenther <agx@sigxcpu.org>
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

#include <dbus/dbus-glib.h>
#include "ka-applet-priv.h"
#include "ka-dialog.h"
#include "ka-dbus.h"
#include "ka-dbus-glue.h"

static DBusGConnection *session;

gboolean
ka_dbus_acquire_tgt (KaApplet *applet,
		     const gchar *principal, DBusGMethodInvocation *context)
{
	 gboolean success;

	 KA_DEBUG("Getting TGT for '%s'", principal);
	 success = ka_check_credentials(applet, principal);
	 dbus_g_method_return(context, success);
	 return TRUE;
}


gboolean
ka_dbus_destroy_ccache(KaApplet* applet, DBusGMethodInvocation *context)
{
	gboolean success;

	KA_DEBUG("Destroying ticket cache");
	success = ka_destroy_ccache (applet);
	dbus_g_method_return(context, success);
	return TRUE;
}


gboolean
ka_dbus_service(KaApplet* applet)
{
	dbus_g_connection_register_g_object (session,
				       "/org/gnome/KrbAuthDialog",
				       G_OBJECT(applet));
	return TRUE;
}


gboolean
ka_dbus_connect(unsigned int* status)
{
	guint request_name_reply;
	unsigned int flags;
	DBusGProxy *bus_proxy;
	GError *error = NULL;

	/* Connect to the session bus so we get exit-on-disconnect semantics. */
	session = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (session == NULL) {
		g_error ("couldn't connect to session bus: %s", (error) ? error->message : "(null)");
		*status = 1;
		g_clear_error (&error);
		return FALSE;
	}
	flags = DBUS_NAME_FLAG_DO_NOT_QUEUE;
	bus_proxy = dbus_g_proxy_new_for_name (session,
					       "org.freedesktop.DBus",
					       "/org/freedesktop/DBus",
					       "org.freedesktop.DBus");

	dbus_g_object_type_install_info(KA_TYPE_APPLET,
					&dbus_glib_krb5_auth_dialog_object_info);

	if (!dbus_g_proxy_call (bus_proxy,
				"RequestName",
				&error,
				G_TYPE_STRING,
				"org.gnome.KrbAuthDialog",
				G_TYPE_UINT,
				flags,
				G_TYPE_INVALID,
				G_TYPE_UINT,
				&request_name_reply,
				G_TYPE_INVALID)) {
		g_warning ("Failed to invoke RequestName: %s",
			   error->message);
	}
	g_clear_error (&error);
	g_object_unref (bus_proxy);

	if (request_name_reply == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER
	    || request_name_reply == DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER)
		;
	else if (request_name_reply == DBUS_REQUEST_NAME_REPLY_EXISTS
		 || request_name_reply == DBUS_REQUEST_NAME_REPLY_IN_QUEUE) {
		*status = 0;
		return FALSE;
	} else {
		g_assert_not_reached();
	}
	return TRUE;
}
