/* Krb5 Auth Applet -- Acquire and release kerberos tickets
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef KRB5_AUTH_APPLET_H
#define KRB5_AUTH_APPLET_H

#include <glib-object.h>
#include <glib/gprintf.h>
#include <krb5.h>

#include "config.h"
#include "krb5-auth-pwdialog.h"

G_BEGIN_DECLS

#define KA_TYPE_APPLET            (ka_applet_get_type ())
#define KA_APPLET(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_APPLET, KaApplet))
#define KA_APPLET_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_APPLET, KaAppletClass))
#define KA_IS_APPLET(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_APPLET))
#define KA_IS_APPLET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_APPLET))
#define KA_APPLET_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_APPLET, KaAppletClass))

typedef struct _KaApplet        KaApplet;
typedef struct _KaAppletClass   KaAppletClass;
typedef struct _KaAppletPrivate KaAppletPrivate;

GType ka_applet_get_type (void);

/* public functions */
gboolean ka_applet_get_show_trayicon(const KaApplet* applet);
void ka_applet_set_tgt_renewable(KaApplet* applet, gboolean renewable);
gboolean ka_applet_get_tgt_renewable(const KaApplet* applet);
guint ka_applet_get_pw_prompt_secs(const KaApplet* applet);
KaPwDialog* ka_applet_get_pwdialog(const KaApplet* applet);

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
