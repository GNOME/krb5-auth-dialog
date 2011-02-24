/* Krb5 Auth Applet -- Acquire and release kerberos tickets
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef KA_PWDIALOG_H
#define KA_PWDIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "config.h"

G_BEGIN_DECLS
#define KA_TYPE_PWDIALOG            (ka_pwdialog_get_type ())
#define KA_PWDIALOG(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PWDIALOG, KaPwDialog))
#define KA_PWDIALOG_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PWDIALOG, KaPwDialogClass))
#define KA_IS_PWDIALOG(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PWDIALOG))
#define KA_IS_PWDIALOG_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PWDIALOG))
#define KA_PWDIALOG_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PWDIALOG, KaPwDialogClass))
    typedef struct _KaPwDialog KaPwDialog;
typedef struct _KaPwDialogClass KaPwDialogClass;
typedef struct _KaPwDialogPrivate KaPwDialogPrivate;

GType ka_pwdialog_get_type (void);

/* public functions */
KaPwDialog *ka_pwdialog_create (GtkBuilder *xml);

/* setup everything for the next prompting */
void ka_pwdialog_setup (KaPwDialog *pwdialog, const gchar *krb5prompt,
                        gboolean invalid_auth);
gint ka_pwdialog_run (KaPwDialog *pwdialog);
void ka_pwdialog_hide (const KaPwDialog *pwdialog, gboolean force);
void ka_pwdialog_set_persist (KaPwDialog *pwdialog, gboolean persist);
void ka_pwdialog_error (KaPwDialog *pwdialog, const char *msg);

/* update the expiry information in the status entry */
gboolean ka_pwdialog_status_update (KaPwDialog *pwdialog);
const gchar *ka_pwdialog_get_password (KaPwDialog *dialog);

G_END_DECLS
#endif
