/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008,2009,2013 Guido Guenther <agx@sigxcpu.org>
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
#include "config.h"

#include <string.h>

#include "ka-applet-priv.h"
#include "ka-settings.h"

#define do_binding(NAME) \
    g_settings_bind(settings, \
        KA_SETTING_KEY_##NAME,\
        applet,               \
        KA_PROP_NAME_##NAME,  \
        G_SETTINGS_BIND_DEFAULT)

static void
ka_setup_bindings(KaApplet* applet,
                  GSettings* settings)
{
    do_binding(PRINCIPAL);
    do_binding(PK_USERID);
    do_binding(PK_ANCHORS);
    do_binding(PW_PROMPT_MINS);
    do_binding(TGT_FORWARDABLE);
    do_binding(TGT_PROXIABLE);
    do_binding(TGT_RENEWABLE);
    do_binding(CONF_TICKETS);
}

#undef do_binding

GSettings*
ka_settings_init (KaApplet* applet)
{
    GSettings *settings;

    settings = g_settings_new (KA_SETTING_SCHEMA);

    ka_setup_bindings(applet, settings);
    return settings;
}
