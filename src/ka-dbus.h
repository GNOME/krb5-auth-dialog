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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KA_DBUS_H
#define KA_DBUS_H

#include <glib.h>
#include "ka-applet-priv.h"

gboolean ka_dbus_connect (KaApplet *applet);
void ka_dbus_disconnect (void);
gboolean ka_dbus_acquire_tgt (KaApplet *applet,
                              const gchar *principal);
gboolean ka_dbus_destroy_ccache (KaApplet *applet);

#endif /* KA_DBUS_H */
