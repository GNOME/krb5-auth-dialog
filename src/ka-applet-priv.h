/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2010 Guido Guenther <agx@sigxcpu.org>
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

/* "Private" header - functions not exported to plugins */

#ifndef KA_APPLET_PRIV_H
#define KA_APPLET_PRIV_H

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gconf/gconf-client.h>
#include <krb5.h>

#include "config.h"
#include "ka-applet.h"
#include "ka-pwdialog.h"

G_BEGIN_DECLS

#define KA_NAME _("Network Authentication")

/* signals emitted by KaApplet */
typedef enum {
  KA_SIGNAL_ACQUIRED_TGT,	/* New TGT acquired */
  KA_SIGNAL_RENEWED_TGT,	/* TGT got renewed */
  KA_SIGNAL_EXPIRED_TGT,	/* TGT expired or ticket cache got destroyed */
  KA_SIGNAL_COUNT
} KaAppletSignalNumber;

/* public functions */
gboolean ka_applet_get_show_trayicon(const KaApplet* applet);
void ka_applet_set_tgt_renewable(KaApplet* applet, gboolean renewable);
gboolean ka_applet_get_tgt_renewable(const KaApplet* applet);
guint ka_applet_get_pw_prompt_secs(const KaApplet* applet);
KaPwDialog* ka_applet_get_pwdialog(const KaApplet* applet);
GConfClient* ka_applet_get_gconf_client(const KaApplet* applet);
void ka_applet_signal_emit(KaApplet* applet, KaAppletSignalNumber signum,
                           krb5_timestamp expiry);

/* create the applet */
KaApplet* ka_applet_create(void);
/* update tooltip and icon */
int ka_applet_update_status(KaApplet* applet, krb5_timestamp expiry);

G_END_DECLS

#ifdef ENABLE_DEBUG
#define KA_DEBUG(fmt,...) \
    g_printf ("DEBUG: %s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define KA_DEBUG(fmt,...) \
    do { } while (0)
#endif /* !ENABLE_DEBUG */

#endif
