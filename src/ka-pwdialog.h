/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KA_PWDIALOG_H
#define KA_PWDIALOG_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "config.h"

G_BEGIN_DECLS
#define KA_TYPE_PWDIALOG      (ka_pwdialog_get_type ())
G_DECLARE_FINAL_TYPE          (KaPwDialog, ka_pwdialog, KA, PWDIALOG, GtkDialog)

/* public functions */
KaPwDialog *ka_pwdialog_new (void);

/* setup everything for the next prompting */
void ka_pwdialog_setup (KaPwDialog *self, const gchar *krb5prompt,
                        gboolean invalid_auth);
gint ka_pwdialog_run (KaPwDialog *self);
void ka_pwdialog_hide (const KaPwDialog *self, gboolean force);
void ka_pwdialog_set_persist (KaPwDialog *self, gboolean persist);
void ka_pwdialog_error (KaPwDialog *self, const char *msg);

/* update the expiry information in the status entry */
gboolean ka_pwdialog_status_update (KaPwDialog *self);
const gchar *ka_pwdialog_get_password (KaPwDialog *self);

G_END_DECLS
#endif
