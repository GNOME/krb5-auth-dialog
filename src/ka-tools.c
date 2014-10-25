/*
 * Copyright (C) 2009 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.        See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gi18n.h>
#include "ka-tools.h"

void
ka_show_help (GdkScreen* screen, const char* chapter, GtkWindow* window)
{
  GError *error = NULL;
  const char *section = "";
  char *url;

  if (chapter)
	section = chapter;

  url = g_strdup_printf("ghelp:krb5-auth-dialog%s", section);

  gtk_show_uri (screen, url, gtk_get_current_event_time (), &error);

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
                  G_CALLBACK (gtk_widget_destroy),
                  NULL);

      gtk_widget_show (message_dialog);
      g_clear_error (&error);
  }
  g_free (url);
}


void 
ka_show_about (KaApplet *applet)
{
    GtkWindow *parent = ka_applet_last_focused_window (applet);

    const gchar *authors[] = {
        "Christopher Aillon <caillon@redhat.com>",
        "Jonathan Blandford <jrb@redhat.com>",
        "Colin Walters <walters@verbum.org>",
        "Guido Günther <agx@sigxcpu.org>",
        NULL
    };

    gtk_show_about_dialog (parent,
                           "authors", authors,
                           "version", VERSION,
                           "logo-icon-name", "krb-valid-ticket",
                           "copyright",
                           "Copyright (C) 2004,2005,2006 Red Hat, Inc.,\n"
                           "2008-2011,2014 Guido Günther",
                           "website-label", PACKAGE " website",
                           "website",
                           "https://honk.sigxcpu.org/piki/projects/krb5-auth-dialog/",
                           "license", "GNU General Public License Version 2",
                           /* Translators: add the translators of your language here */
                           "translator-credits", _("translator-credits"),
                           NULL);
}
