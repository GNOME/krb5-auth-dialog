/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2009 Guido Guenther <agx@sigxcpu.org>
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

#ifndef KA_TOOLS
#define KA_TOOLS

#include <gtk/gtk.h>
#include <ka-applet-priv.h>

G_BEGIN_DECLS

void ka_show_help (GtkWindow* window, const char* section);
void ka_show_about (KaApplet *applet);

G_END_DECLS

#endif
