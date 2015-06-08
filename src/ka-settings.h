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

#ifndef KA_GCONF_H
#define KA_GCONF_H

#include "ka-applet-priv.h"

#define KA_SETTING_SCHEMA               "org.gnome.KrbAuthDialog"
#define KA_SETTING_KEY_PRINCIPAL        "principal"
#define KA_SETTING_KEY_PK_USERID        "pk-userid"
#define KA_SETTING_KEY_PK_ANCHORS       "pk-anchors"
#define KA_SETTING_KEY_PW_PROMPT_MINS   "prompt-minutes"
#define KA_SETTING_KEY_TGT_FORWARDABLE  "forwardable"
#define KA_SETTING_KEY_TGT_RENEWABLE    "renewable"
#define KA_SETTING_KEY_TGT_PROXIABLE    "proxiable"
#define KA_SETTING_KEY_CONF_TICKETS     "conf-tickets"
#define KA_SETTING_CHILD_NOTIFY         "notify"
#define KA_SETTING_KEY_NOTIFY_VALID     "valid"
#define KA_SETTING_KEY_NOTIFY_EXPIRED   "expired"
#define KA_SETTING_KEY_NOTIFY_EXPIRING  "expiring"
#define KA_SETTING_CHILD_PLUGINS        "plugins"
#define KA_SETTING_KEY_PLUGINS_ENABLED  "enabled"

GSettings* ka_settings_init (KaApplet* applet);

#endif
