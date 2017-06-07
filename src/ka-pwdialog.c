/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2009,2013 Guido Guenther <agx@sigxcpu.org>
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

#include <glib/gi18n.h>

#include "ka-applet-priv.h"
#include "ka-kerberos.h"
#include "ka-pwdialog.h"
#include "ka-entry-buffer.h"

struct _KaPwDialog {
    GtkDialog parent;

    KaPwDialogPrivate *priv;
};

struct _KaPwDialogClass {
    GtkDialogClass parent;
};

struct _KaPwDialogPrivate {
    /* The password dialog */
    GtkWidget *status_label;    /* the wrong password/timeout label */
    GtkWidget *krb_label;       /* krb5 passwort prompt label */
    GtkWidget *entry_hbox;      /* hbox for the pw entry */
    GtkWidget *pw_entry;        /* password entry field */

    gboolean persist;           /* don't hide the dialog when creds are still valid */
    gboolean grabbed;           /* keyboard grabbed? */
    GtkWidget *error_dialog;    /* error dialog */
};

G_DEFINE_TYPE_WITH_PRIVATE (KaPwDialog, ka_pwdialog, GTK_TYPE_DIALOG);

static void
ka_pwdialog_init (KaPwDialog *pwdialog)
{
    pwdialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (pwdialog,
                                                  KA_TYPE_PWDIALOG,
                                                  KaPwDialogPrivate);

    gtk_widget_init_template (GTK_WIDGET (pwdialog));
}

static void
ka_pwdialog_finalize (GObject *object)
{
    KaPwDialog *pwdialog = KA_PWDIALOG (object);
    GObjectClass *parent_class = G_OBJECT_CLASS (ka_pwdialog_parent_class);

    gtk_widget_destroy (pwdialog->priv->error_dialog);
    pwdialog->priv->error_dialog = NULL;

    parent_class->finalize (object);
}


static GObject *
ka_pwdialog_constructor (GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_params)
{
    GObject *object;
    guint idx;
    GParamSpec *pspec;
    GValue *value;

    for (idx = 0; idx < n_construct_properties; idx++)
    {
        pspec = construct_params[idx].pspec;
        if (g_strcmp0 (pspec->name, "use-header-bar") != 0)
            continue;

        /* GtkDialog uses "-1" to imply an unset value for this property */
        value = construct_params[idx].value;
        if (g_value_get_int (value) == -1)
            g_value_set_int (value, 1);

        break;
    }

    object = G_OBJECT_CLASS (ka_pwdialog_parent_class)->constructor (type,
                                                                     n_construct_properties,
                                                                     construct_params);

    return object;
}


static void
ka_pwdialog_class_init (KaPwDialogClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->finalize = ka_pwdialog_finalize;
    object_class->constructor = ka_pwdialog_constructor;

    /* Bind class to template
     */
    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/krb5-auth-dialog/ui/ka-pwdialog.ui");

    gtk_widget_class_bind_template_child_private (widget_class, KaPwDialog, status_label);
    gtk_widget_class_bind_template_child_private (widget_class, KaPwDialog, krb_label);
    gtk_widget_class_bind_template_child_private (widget_class, KaPwDialog, entry_hbox);
}


static void add_password_entry (KaPwDialogPrivate *priv)
{
    KaEntryBuffer *buffer = ka_entry_buffer_new ();

    priv->pw_entry =
        GTK_WIDGET (gtk_entry_new_with_buffer (GTK_ENTRY_BUFFER (buffer)));
    gtk_entry_set_visibility (GTK_ENTRY (priv->pw_entry), FALSE);
    g_object_unref (buffer);

    gtk_container_add (GTK_CONTAINER (priv->entry_hbox), priv->pw_entry);
    gtk_entry_set_activates_default (GTK_ENTRY (priv->pw_entry), TRUE);
    gtk_widget_show (priv->pw_entry);
}


static GtkWidget *
ka_error_dialog_new (void)
{
    GtkWidget *dialog =
        gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                _("%s Error"), KA_NAME);

    gtk_window_set_title (GTK_WINDOW (dialog), _(KA_NAME));
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);
    return dialog;
}


KaPwDialog *
ka_pwdialog_new (void)
{
    KaPwDialog *pwdialog;
    gboolean use_header;

    g_object_get (gtk_settings_get_default (), "gtk-dialogs-use-header", &use_header, NULL);
    pwdialog = g_object_new (KA_TYPE_PWDIALOG, "use-header-bar", use_header, NULL);

    pwdialog->priv->error_dialog = ka_error_dialog_new ();
    add_password_entry (pwdialog->priv);
    return pwdialog;
}

static gboolean
grab_keyboard (GtkWidget *win, GdkEvent * event, gpointer data)
{
    KaPwDialog *pwdialog = KA_PWDIALOG (data);
    GdkGrabStatus status;
    GdkWindow *window = gtk_widget_get_window(win);

    gtk_grab_add(win);
    if (!pwdialog->priv->grabbed) {
        status = gdk_seat_grab (gdk_display_get_default_seat(
                                    gdk_window_get_display (window)),
                                window,
                                GDK_SEAT_CAPABILITY_ALL,
                                TRUE,
                                NULL,
                                event,
                                NULL,
                                NULL);

        if (status == GDK_GRAB_SUCCESS)
            pwdialog->priv->grabbed = TRUE;
        else
            g_message ("could not grab keyboard: %d", (int) status);
    }
    return FALSE;
}


static gboolean
ungrab_keyboard (GtkWidget *win,
                 GdkEvent *event G_GNUC_UNUSED, gpointer data)
{
    KaPwDialog *pwdialog = KA_PWDIALOG (data);
    GdkWindow *window = gtk_widget_get_window(win);

    if (pwdialog->priv->grabbed) {
        gtk_grab_remove (win);
        gdk_seat_ungrab (gdk_display_get_default_seat(
                             gdk_window_get_display (window)));

        pwdialog->priv->grabbed = FALSE;
    }
    return FALSE;
}


static gboolean
window_state_changed (GtkWidget *win, GdkEventWindowState *event,
                      gpointer data)
{
    GdkWindowState state = gdk_window_get_state (gtk_widget_get_window (win));

    if (state & GDK_WINDOW_STATE_WITHDRAWN ||
        state & GDK_WINDOW_STATE_ICONIFIED ||
        state & GDK_WINDOW_STATE_FULLSCREEN ||
        state & GDK_WINDOW_STATE_MAXIMIZED)
        ungrab_keyboard (win, (GdkEvent *) event, data);
    else
        grab_keyboard (win, (GdkEvent *) event, data);

    return FALSE;
}


gint
ka_pwdialog_run (KaPwDialog *self)
{
    /* cleanup old error dialog, if present (e.g. user didn't acknowledge
     * the error but clicked the tray icon again) */
    if (self->priv->error_dialog)
        gtk_widget_hide (self->priv->error_dialog);

    /* make sure we pop up on top */
    gtk_window_set_keep_above (GTK_WINDOW (self), TRUE);

    /*
     * grab the keyboard so that people don't accidentally type their
     * passwords in other windows.
     */
    g_signal_connect (self, "map-event", G_CALLBACK (grab_keyboard), self);
    g_signal_connect (self, "unmap-event", G_CALLBACK (ungrab_keyboard),
                      self);
    g_signal_connect (self, "window-state-event",
                      G_CALLBACK (window_state_changed), self);

    gtk_widget_grab_focus (self->priv->pw_entry);
    gtk_widget_show (GTK_WIDGET (self));
    return gtk_dialog_run (GTK_DIALOG (self));
}


void
ka_pwdialog_error (KaPwDialog *self, const char *msg)
{
    GtkWidget *dialog = self->priv->error_dialog;

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              _
                                              ("Couldn't acquire Kerberos ticket: '%s'"),
                                              _(msg));
    gtk_widget_show (GTK_WIDGET (dialog));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_hide (dialog);
}


void
ka_pwdialog_set_persist (KaPwDialog *pwdialog, gboolean persist)
{
    pwdialog->priv->persist = persist;
}

void
ka_pwdialog_hide (const KaPwDialog *pwdialog, gboolean force)
{
    KA_DEBUG ("PW Dialog persist: %d", pwdialog->priv->persist);
    if (!pwdialog->priv->persist || force)
        gtk_widget_hide (GTK_WIDGET (pwdialog));
}

const gchar *
ka_pwdialog_get_password (KaPwDialog *pwdialog)
{
    return gtk_entry_get_text (GTK_ENTRY (pwdialog->priv->pw_entry));
}

gboolean
ka_pwdialog_status_update (KaPwDialog *pwdialog)
{
    gchar *expiry_text;
    gchar *expiry_markup;
    int minutes_left = ka_tgt_valid_seconds () / 60;

    g_return_val_if_fail (pwdialog != NULL, FALSE);
    if (minutes_left > 0) {
        expiry_text =
            g_strdup_printf (ngettext
                             ("Your credentials expire in %d minute",
                              "Your credentials expire in %d minutes",
                              minutes_left), minutes_left);
    } else {
        expiry_text = g_strdup_printf ("<span foreground=\"red\">%s</span>",
                                       _("Your credentials have expired"));
    }
    expiry_markup =
        g_strdup_printf ("<span size=\"smaller\" style=\"italic\">%s</span>",
                         expiry_text);
    gtk_label_set_markup (GTK_LABEL (pwdialog->priv->status_label),
                          expiry_markup);
    g_free (expiry_text);
    g_free (expiry_markup);

    return TRUE;
}

void
ka_pwdialog_setup (KaPwDialog *pwdialog, const gchar *krb5prompt,
                   gboolean invalid_auth)
{
    KaPwDialogPrivate *priv = pwdialog->priv;
    gchar *wrong_markup = NULL;
    gchar *prompt;
    int pw4len;

    if (krb5prompt == NULL) {
        prompt = g_strdup (_("Please enter your Kerberos password:"));
    } else {
        /* Kerberos's prompts are a mess, and basically impossible to
         * translate.  There's basically no way short of doing a lot of
         * string parsing to translate them.  The most common prompt is
         * "Password for $uid:".  We special case that one at least.  We
         * cannot do any of the fancier strings (like challenges),
         * though. */
        pw4len = strlen ("Password for ");
        if (strncmp (krb5prompt, "Password for ", pw4len) == 0) {
            gchar *uid = (gchar *) (krb5prompt + pw4len);

            prompt =
                g_strdup_printf (_("Please enter the password for '%s':"),
                                 uid);
        } else {
            prompt = g_strdup (krb5prompt);
        }
    }

    /* Clear the password entry field */
    gtk_entry_set_text (GTK_ENTRY (priv->pw_entry), "");

    /* Use the prompt label that krb5 provides us */
    gtk_label_set_text (GTK_LABEL (priv->krb_label), prompt);

    /* Add our extra message hints */
    if (invalid_auth) {
        wrong_markup =
            g_strdup_printf
            ("<span size=\"smaller\" style=\"italic\">%s</span>",
             _("The password you entered is invalid"));
        gtk_label_set_markup (GTK_LABEL (priv->status_label), wrong_markup);
    } else
        ka_pwdialog_status_update (pwdialog);

    g_free (wrong_markup);
    g_free (prompt);
}
