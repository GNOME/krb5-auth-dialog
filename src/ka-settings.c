/*
 * (C) 2008,2009,2013 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
