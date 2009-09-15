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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include "krb5-auth-tools.h"

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
      g_error_free (error);
  }
  g_free (url);
}

