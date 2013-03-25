/* -*- c-file-style: "linux"; c-basic-offset: 4; indent-tabs-mode: nil; -*- *
 *
 * Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2009,2011,2013 Guido Guenther <agx@sigxcpu.org>
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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ka-main-window.h"
#include "ka-kerberos.h"
#include "ka-tools.h"
#include "ka-preferences.h"

static GtkListStore *tickets;
static GtkApplicationWindow *main_window;

static void
ccache_changed_cb (KaApplet* applet,
                   gpointer user_data G_GNUC_UNUSED)
{
    gboolean conf_tickets;

    KA_DEBUG("Refreshing ticket list");
    g_object_get(applet, KA_PROP_NAME_CONF_TICKETS, &conf_tickets, NULL);
    ka_get_service_tickets (tickets, !conf_tickets);
}


GtkApplicationWindow *
ka_main_window_create (KaApplet *applet, GtkBuilder *xml)
{
    GtkCellRenderer *text_renderer, *toggle_renderer;
    GtkTreeView *tickets_view;

    tickets = gtk_list_store_new (N_COLUMNS,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_BOOLEAN,
                                  G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

    main_window =
        GTK_APPLICATION_WINDOW (gtk_builder_get_object (xml,
                                                        "krb5_main_window"));
    g_object_set(main_window, "application", applet, NULL);

    tickets_view =
        GTK_TREE_VIEW (gtk_builder_get_object (xml, "krb5_tickets_treeview"));
    gtk_tree_view_set_model (GTK_TREE_VIEW (tickets_view),
                             GTK_TREE_MODEL (tickets));

    text_renderer = gtk_cell_renderer_text_new ();
    toggle_renderer = gtk_cell_renderer_toggle_new ();

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("Principal"),
                                                text_renderer,
                                                "text",
                                                PRINCIPAL_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("Start Time"),
                                                text_renderer,
                                                "text",
                                                START_TIME_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("End Time"),
                                                text_renderer,
                                                "markup",
                                                END_TIME_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                      /* Translators: this is an abbreviation for forwardable */
                                                _("Fwd"),
                                                toggle_renderer,
                                                "active",
                                                FORWARDABLE_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                        /* Translators: this is an abbreviation for proxiable */
                                                _("Proxy"),
                                                toggle_renderer,
                                                "active",
                                                PROXIABLE_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                        /* Translators: this is an abbreviation for renewable */
                                                _("Renew"),
                                                toggle_renderer,
                                                "active",
                                                RENEWABLE_COLUMN,
                                                NULL);

    g_signal_connect (applet, "krb-ccache-changed",
                      G_CALLBACK(ccache_changed_cb),
                      NULL);

    return main_window;
}

void
ka_main_window_show (KaApplet *applet)
{
    gboolean conf_tickets;

    g_object_get(applet, KA_PROP_NAME_CONF_TICKETS, &conf_tickets, NULL);
    if (ka_get_service_tickets (tickets, !conf_tickets)) {
        gtk_window_present (GTK_WINDOW(main_window));
    } else {
        GtkWidget *message_dialog;

        message_dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 _
                                                 ("Error displaying service ticket information"));
        gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);

        g_signal_connect (message_dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (message_dialog);
    }
}

void
ka_main_window_hide ()
{
    KA_DEBUG("Hiding main window");
    gtk_widget_hide (GTK_WIDGET(main_window));
}
