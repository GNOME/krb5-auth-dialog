/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2010,2011 Guido Guenther <agx@sigxcpu.org>
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

/* "Private" header - functions not exported to plugins */

#ifndef KA_APPLET_PRIV_H
#define KA_APPLET_PRIV_H

#include <glib-object.h>
#include <glib/gprintf.h>
#include <krb5.h>

#include "config.h"
#include "ka-applet.h"
#include "ka-pwdialog.h"

G_BEGIN_DECLS
#define KA_NAME _("Kerberos Authentication")

/* signals emitted by KaApplet */
typedef enum {
    KA_SIGNAL_ACQUIRED_TGT,     /* New TGT acquired */
    KA_SIGNAL_RENEWED_TGT,      /* TGT got renewed */
    KA_SIGNAL_EXPIRED_TGT,      /* TGT expired or ticket cache got destroyed */
    KA_CCACHE_CHANGED,          /* The credential cache changed */
    KA_SIGNAL_COUNT
} KaAppletSignalNumber;

extern const gchar *ka_signal_names[];

/* public functions */
gboolean ka_applet_get_show_trayicon (const KaApplet *self);
void ka_applet_set_tgt_renewable (KaApplet *self, gboolean renewable);
gboolean ka_applet_get_tgt_renewable (const KaApplet *self);
guint ka_applet_get_pw_prompt_secs (const KaApplet *self);
KaPwDialog *ka_applet_get_pwdialog (const KaApplet *self);
GSettings *ka_applet_get_settings (const KaApplet *self);
void ka_applet_signal_emit (KaApplet *self, KaAppletSignalNumber signum,
                            krb5_timestamp expiry);
void ka_applet_set_msg (KaApplet *self, const char *msg);
GtkWindow* ka_applet_last_focused_window(KaApplet *self);

/* properties */
#define KA_PROP_NAME_PRINCIPAL       "principal"
#define KA_PROP_NAME_PK_USERID       "pk-userid"
#define KA_PROP_NAME_PK_ANCHORS      "pk-anchors"
#define KA_PROP_NAME_PW_PROMPT_MINS  "pw-prompt-mins"
#define KA_PROP_NAME_TGT_FORWARDABLE "tgt-forwardable"
#define KA_PROP_NAME_TGT_PROXIABLE   "tgt-proxiable"
#define KA_PROP_NAME_TGT_RENEWABLE   "tgt-renewable"
#define KA_PROP_NAME_CONF_TICKETS    "conf-tickets"

/* create the applet */
KaApplet *ka_applet_create (void);
/* destroy the applet */
void ka_applet_destroy (KaApplet *self);

/* update tooltip and icon */
int ka_applet_update_status (KaApplet *self, krb5_timestamp expiry);

G_END_DECLS

#define KA_DEBUG(fmt,...) \
    g_debug ("%s: " fmt, __func__, ##__VA_ARGS__)

#endif
