/* -*- c-file-style: "linux"; c-basic-offset: 4; indent-tabs-mode: nil; -*- *
 *
 * Krb5 Auth Applet -- Acquire and release Kerberos tickets
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
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
static GtkButton *ticket_btn;
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


static void
enable_ticket_button_cb (gpointer* applet G_GNUC_UNUSED,
                         gchar *princ G_GNUC_UNUSED,
                         guint when G_GNUC_UNUSED,
                         gpointer user_data)
{
    gboolean enable = GPOINTER_TO_UINT(user_data);
    KA_DEBUG ("Sensitive: %u", enable);
    gtk_widget_set_sensitive(GTK_WIDGET(ticket_btn), enable);
}


static void
ticket_btn_clicked(GtkButton* btn G_GNUC_UNUSED, gpointer user_data)
{
    KaApplet *applet = KA_APPLET(user_data);
    ka_grab_credentials (applet);
}


static void
on_row_inserted(GtkTreeModel *model,
                GtkTreePath  *unused,
                GtkTreeIter  *iter,
                GtkStack     *stack)
{
    gtk_stack_set_visible_child_name(stack, "tickets");
}


static void
on_row_deleted(GtkTreeModel *model,
               GtkTreePath  *unused,
               GtkStack     *stack)
{
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter_first(model, &iter))
        gtk_stack_set_visible_child_name(stack, "message");
}



GtkApplicationWindow *
ka_main_window_create (KaApplet *applet)
{
    GtkCellRenderer *text_renderer, *toggle_renderer;
    GtkTreeViewColumn *column;
    GtkTreeView *tickets_view;
    GtkStack *stack;
    GtkBuilder *builder;

    tickets = gtk_list_store_new (N_COLUMNS,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_STRING,
                                  G_TYPE_BOOLEAN,
                                  G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

    builder = gtk_builder_new_from_resource ("/org/gnome/krb5-auth-dialog/ui/ka-main-window.ui");
    main_window =
        GTK_APPLICATION_WINDOW (gtk_builder_get_object (builder,
                                                        "krb5_main_window"));
    gtk_builder_connect_signals (builder, NULL);
    g_object_set(main_window, "application", applet, NULL);

    tickets_view =
        GTK_TREE_VIEW (gtk_builder_get_object (builder, "krb5_tickets_treeview"));
    gtk_tree_view_set_model (GTK_TREE_VIEW (tickets_view),
                             GTK_TREE_MODEL (tickets));

    text_renderer = gtk_cell_renderer_text_new ();
    g_object_set (text_renderer,
                  "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                  NULL);
    toggle_renderer = gtk_cell_renderer_toggle_new ();

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("Principal"),
                                                text_renderer,
                                                "text",
                                                PRINCIPAL_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("Start"),
                                                text_renderer,
                                                "text",
                                                START_TIME_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tickets_view), -1,
                                                _("End"),
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

    for (int i = PRINCIPAL_COLUMN; i <= END_TIME_COLUMN; i++) {
        column = gtk_tree_view_get_column(tickets_view, i);
        g_object_set (column, "expand", TRUE, NULL);
    }

    stack = GTK_STACK (gtk_builder_get_object (builder, "stack"));
    g_signal_connect(tickets, "row-inserted",
                     G_CALLBACK(on_row_inserted),
                     stack);
    g_signal_connect(tickets, "row-deleted",
                     G_CALLBACK(on_row_deleted),
                     stack);
    on_row_deleted(GTK_TREE_MODEL(tickets), NULL, stack);

    g_signal_connect (applet, "krb-ccache-changed",
                      G_CALLBACK(ccache_changed_cb),
                      NULL);

    ticket_btn =
        GTK_BUTTON (gtk_builder_get_object (builder, "get_ticket_btn"));
    g_signal_connect(ticket_btn, "clicked", G_CALLBACK(ticket_btn_clicked), applet);

    g_signal_connect (applet, "krb-tgt-acquired",
                      G_CALLBACK (enable_ticket_button_cb),
                      GUINT_TO_POINTER(FALSE));

    g_signal_connect (applet, "krb-tgt-expired",
                      G_CALLBACK (enable_ticket_button_cb),
                      GUINT_TO_POINTER(TRUE));
    g_object_unref (builder);
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
ka_main_window_hide (void)
{
    KA_DEBUG("Hiding main window");
    gtk_widget_hide (GTK_WIDGET(main_window));
}
