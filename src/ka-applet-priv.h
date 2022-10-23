/*
 * (C) 2008,2010,2011 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
/* "Private" header - functions not exported to plugins */

#pragma once

#include <glib-object.h>
#include <glib/gprintf.h>
#include <krb5.h>

#include "config.h"
#include "ka-applet.h"
#include "ka-pwdialog.h"

G_BEGIN_DECLS
#define KA_NAME _("Kerberos Authentication")

/* properties */
#define KA_PROP_NAME_PRINCIPAL       "principal"
#define KA_PROP_NAME_PK_USERID       "pk-userid"
#define KA_PROP_NAME_PK_ANCHORS      "pk-anchors"
#define KA_PROP_NAME_PW_PROMPT_MINS  "pw-prompt-mins"
#define KA_PROP_NAME_TGT_FORWARDABLE "tgt-forwardable"
#define KA_PROP_NAME_TGT_PROXIABLE   "tgt-proxiable"
#define KA_PROP_NAME_TGT_RENEWABLE   "tgt-renewable"
#define KA_PROP_NAME_CONF_TICKETS    "conf-tickets"

/* public functions */
void ka_applet_set_tgt_renewable (KaApplet *self, gboolean renewable);
gboolean ka_applet_get_tgt_renewable (const KaApplet *self);
guint ka_applet_get_pw_prompt_secs (const KaApplet *self);
KaPwDialog *ka_applet_get_pwdialog (const KaApplet *self);
GSettings *ka_applet_get_settings (const KaApplet *self);
void ka_applet_emit_renewed (KaApplet *self, krb5_timestamp expiry);
void ka_applet_set_msg (KaApplet *self, const char *msg);
GtkWindow* ka_applet_last_focused_window(KaApplet *self);
/* update tooltip and icon */
int ka_applet_update_status (KaApplet *self, krb5_timestamp expiry);

#define KA_DEBUG(fmt,...) \
    g_debug ("%s: " fmt, __func__, ##__VA_ARGS__)

G_END_DECLS
