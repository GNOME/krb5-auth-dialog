/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
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

#ifndef KRB5_AUTH_DBUS_H
#define KRB5_AUTH_DBUS_H

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "krb5-auth-applet.h"

gboolean ka_dbus_connect(unsigned int* status);
gboolean ka_dbus_service(KaApplet* applet);
gboolean ka_dbus_acquire_tgt (KaApplet *applet,
			      const gchar *principal,
			      DBusGMethodInvocation *context);
gboolean ka_dbus_destroy_ccache(KaApplet* applet,
			        DBusGMethodInvocation *context);

#endif /* KRB5_AUTH_DBUS_H */
