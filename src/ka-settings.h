/*
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
