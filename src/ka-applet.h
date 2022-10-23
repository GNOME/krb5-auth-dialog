/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KA_APPLET_H
#define KA_APPLET_H

#include <adwaita.h>

G_BEGIN_DECLS

#define KA_TYPE_APPLET  (ka_applet_get_type ())
G_DECLARE_FINAL_TYPE    (KaApplet, ka_applet, KA, APPLET, AdwApplication)

KaApplet          *ka_applet_new                    (void);
const char        *ka_applet_get_principal          (KaApplet *self);

G_END_DECLS

#endif
