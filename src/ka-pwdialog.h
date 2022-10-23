/*
 * (C) 2009 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
