/*
 * (C) 2009,2013 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include "ka-applet-priv.h"
#include "ka-kerberos.h"
#include "ka-pwdialog.h"
#include "ka-tools.h"

struct _KaPwDialog {
    GtkDialog parent;

    /* The password dialog */
    GtkWidget *status_label;    /* the wrong password/timeout label */
    GtkWidget *krb_label;       /* krb5 passwort prompt label */
    GtkWidget *pw_entry;        /* password entry field */

    gboolean persist;           /* don't hide the dialog when creds are still valid */
    GtkWidget *error_dialog;    /* error dialog */
};

G_DEFINE_FINAL_TYPE (KaPwDialog, ka_pwdialog, GTK_TYPE_DIALOG)

static void
ka_pwdialog_init (KaPwDialog *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}

static void
ka_pwdialog_finalize (GObject *object)
{
    KaPwDialog *pwdialog = KA_PWDIALOG (object);

    ka_window_destroy (pwdialog->error_dialog);
    pwdialog->error_dialog = NULL;

    G_OBJECT_CLASS (ka_pwdialog_parent_class)->finalize (object);
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

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/krb5-auth-dialog/ui/ka-pwdialog.ui");

    gtk_widget_class_bind_template_child (widget_class, KaPwDialog, status_label);
    gtk_widget_class_bind_template_child (widget_class, KaPwDialog, krb_label);
    gtk_widget_class_bind_template_child (widget_class, KaPwDialog, pw_entry);
}


static GtkWidget *
ka_error_dialog_new (void)
{
    GtkWidget *dialog =
        gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                _("%s Error"), KA_NAME);

    gtk_window_set_title (GTK_WINDOW (dialog), _(KA_NAME));
    g_signal_connect (dialog,
                      "response",
                      G_CALLBACK (gtk_widget_hide),
                      NULL);
    return dialog;
}


KaPwDialog *
ka_pwdialog_new (void)
{
    KaPwDialog *pwdialog;
    gboolean use_header;

    g_object_get (gtk_settings_get_default (), "gtk-dialogs-use-header", &use_header, NULL);
    pwdialog = g_object_new (KA_TYPE_PWDIALOG, "use-header-bar", use_header, NULL);

    pwdialog->error_dialog = ka_error_dialog_new ();
    return pwdialog;
}

int response;


static void
on_response (GtkDialog *dialog, int response_id, GMainLoop *main_loop)
{
    response = response_id;
    g_main_loop_quit (main_loop);
}


gint
ka_pwdialog_run (KaPwDialog *self)
{
    int id;
    g_autoptr (GMainLoop) dialog_main_loop = NULL;

    /* cleanup old error dialog, if present (e.g. user didn't acknowledge
     * the error but clicked the tray icon again) */
    if (self->error_dialog)
        gtk_widget_hide (self->error_dialog);

    gtk_widget_grab_focus (self->pw_entry);

    /* FIXME: we use a separate main loop as the Kerberos code wants
       us to return a response. This is not worse than it was before -
       it's just that gtk_dialog_run() going a way in GTK4 makes this
       more obvious. */
    dialog_main_loop = g_main_loop_new (NULL, FALSE);
    id = g_signal_connect (self, "response", G_CALLBACK (on_response), dialog_main_loop);

    gtk_widget_show (GTK_WIDGET (self));
    g_main_loop_run (dialog_main_loop);

    g_signal_handler_disconnect (self, id);

    return response;
}


void
ka_pwdialog_error (KaPwDialog *self, const char *msg)
{
    GtkWidget *dialog = self->error_dialog;

    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              _
                                              ("Couldn't acquire Kerberos ticket: '%s'"),
                                              _(msg));
    gtk_widget_show (GTK_WIDGET (dialog));
}


void
ka_pwdialog_set_persist (KaPwDialog *pwdialog, gboolean persist)
{
    pwdialog->persist = persist;
}

void
ka_pwdialog_hide (const KaPwDialog *pwdialog, gboolean force)
{
    KA_DEBUG ("PW Dialog persist: %d", pwdialog->persist);
    if (!pwdialog->persist || force)
        gtk_widget_hide (GTK_WIDGET (pwdialog));
}

const gchar *
ka_pwdialog_get_password (KaPwDialog *pwdialog)
{
    return ka_editable_get_text (pwdialog->pw_entry);
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
    gtk_label_set_markup (GTK_LABEL (pwdialog->status_label),
                          expiry_markup);
    g_free (expiry_text);
    g_free (expiry_markup);

    return TRUE;
}

void
ka_pwdialog_setup (KaPwDialog *self, const gchar *krb5prompt, gboolean invalid_auth)
{
    gchar *wrong_markup = NULL;
    gchar *prompt;
    int pw4len;

    g_assert (KA_IS_PWDIALOG (self));

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
    ka_editable_set_text (self->pw_entry, "");

    /* Use the prompt label that krb5 provides us */
    gtk_label_set_text (GTK_LABEL (self->krb_label), prompt);

    /* Add our extra message hints */
    if (invalid_auth) {
        wrong_markup =
            g_strdup_printf
            ("<span size=\"smaller\" style=\"italic\">%s</span>",
             _("The password you entered is invalid"));
        gtk_label_set_markup (GTK_LABEL (self->status_label), wrong_markup);
    } else
        ka_pwdialog_status_update (self);

    g_free (wrong_markup);
    g_free (prompt);
}
