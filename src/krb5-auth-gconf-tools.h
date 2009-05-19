/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2009 Guido Guenther <agx@sigxcpu.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef KRB5_AUTH_GCONF_TOOLS_H
#define KRB5_AUTH_GCONF_TOOLS_H

#include "config.h"

#include <gconf/gconf-client.h>

#define KA_GCONF_PATH			"/apps/" PACKAGE
#define KA_GCONF_KEY_PRINCIPAL		KA_GCONF_PATH "/principal"
#define KA_GCONF_KEY_PK_USERID		KA_GCONF_PATH "/pk_userid"
#define KA_GCONF_KEY_PK_ANCHORS		KA_GCONF_PATH "/pk_anchors"
#define KA_GCONF_KEY_PROMPT_MINS	KA_GCONF_PATH "/prompt_minutes"
#define KA_GCONF_KEY_SHOW_TRAYICON	KA_GCONF_PATH "/show_trayicon"
#define KA_GCONF_KEY_FORWARDABLE	KA_GCONF_PATH "/forwardable"
#define KA_GCONF_KEY_RENEWABLE		KA_GCONF_PATH "/renewable"
#define KA_GCONF_KEY_PROXIABLE		KA_GCONF_PATH "/proxiable"
#define KA_GCONF_KEY_NOTIFY_VALID	KA_GCONF_PATH "/notify/valid"
#define KA_GCONF_KEY_NOTIFY_EXPIRED	KA_GCONF_PATH "/notify/expired"
#define KA_GCONF_KEY_NOTIFY_EXPIRING	KA_GCONF_PATH "/notify/expiring"

gboolean ka_gconf_get_string (GConfClient* client, const char* key, char** value);
gboolean ka_gconf_get_int (GConfClient* client, const char* key, int* value);
gboolean ka_gconf_get_bool (GConfClient* client, const char* key, gboolean* value);
gboolean ka_gconf_set_bool (GConfClient* client, const char* key, gboolean value);

#endif
