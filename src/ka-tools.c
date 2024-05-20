/*
 * Copyright (C) 2009 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>
#include "ka-tools.h"

void
ka_show_help (GtkWindow* window, const char* chapter)
{
  GError *error = NULL;
  const char *section = "";
  char *url;

  if (chapter)
	section = chapter;

  url = g_strdup_printf("help:krb5-auth-dialog%s", section);

  gtk_show_uri (window, url, GDK_CURRENT_TIME);

  if (error) {
      GtkWidget *message_dialog;

      message_dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         _("There was an error displaying help:\n%s"),
                                         error->message);
      gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);

      g_signal_connect (message_dialog, "response",
                        G_CALLBACK (ka_window_destroy),
                        NULL);

      gtk_widget_show (message_dialog);
      g_clear_error (&error);
  }
  g_free (url);
}


void
ka_show_about (KaApplet *applet)
{
    const gchar *developers[] = {
        "Christopher Aillon <caillon@redhat.com>",
        "Jonathan Blandford <jrb@redhat.com>",
        "Colin Walters <walters@verbum.org>",
        "Guido Günther <agx@sigxcpu.org>",
        NULL
    };

    adw_show_about_window (gtk_application_get_active_window (GTK_APPLICATION (g_application_get_default ())),
                           "application-name", _(KA_NAME),
                           "application-icon", "krb-valid-ticket",
                           "version", VERSION,
                           "copyright", "Copyright (C) 2004,2005,2006 Red Hat, Inc.,"
                                        "2008-2011,2014,2021,2022 Guido Günther",
                           "website", "https://honk.sigxcpu.org/piki/projects/krb5-auth-dialog/",
                           "issue-url", "https://gitlab.gnome.org/GNOME/krb5-auth-dialog/-/issues/",
                           "license-type", GTK_LICENSE_GPL_2_0,
                           "developers", developers,
                           "translator-credits", _("translator-credits"),
                           NULL);
}


void
ka_window_destroy (gpointer window)
{
    g_assert (GTK_IS_WINDOW (window));
    gtk_window_destroy (GTK_WINDOW (window));
}


void
ka_editable_set_text (gpointer editable, const char *text)
{
    gtk_editable_set_text (GTK_EDITABLE(editable), text);
}

const char *
ka_editable_get_text (gpointer editable)
{
    return gtk_editable_get_text (GTK_EDITABLE (editable));
}
