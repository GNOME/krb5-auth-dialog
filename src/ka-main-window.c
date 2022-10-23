/*
 * (C) 2009,2011,2013,2022 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ka-main-window.h"
#include "ka-kerberos.h"
#include "ka-tools.h"
#include "ka-preferences.h"


struct _KaMainWindow {
    GtkApplicationWindow parent;

    GtkStack      *stack;
    GtkListStore  *tickets;
    GtkButton     *get_ticket_btn;
    GtkTreeView   *tickets_view;
};

G_DEFINE_TYPE (KaMainWindow, ka_main_window, GTK_TYPE_APPLICATION_WINDOW)

static void
ccache_changed_cb (KaApplet* applet, KaMainWindow *self)
{
    gboolean conf_tickets;

    KA_DEBUG("Refreshing ticket list");
    g_object_get(applet, KA_PROP_NAME_CONF_TICKETS, &conf_tickets, NULL);
    ka_get_service_tickets (self->tickets, !conf_tickets);
}


static void
enable_ticket_button_cb (gpointer* applet G_GNUC_UNUSED,
                         gchar *princ G_GNUC_UNUSED,
                         guint when G_GNUC_UNUSED,
                         KaMainWindow *self)
{
    gtk_widget_set_sensitive(GTK_WIDGET(self->get_ticket_btn), TRUE);
}


static void
disable_ticket_button_cb (gpointer* applet G_GNUC_UNUSED,
                          gchar *princ G_GNUC_UNUSED,
                          guint when G_GNUC_UNUSED,
                          KaMainWindow *self)
{
    gtk_widget_set_sensitive(GTK_WIDGET(self->get_ticket_btn), FALSE);
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


static void
ka_main_window_constructed (GObject *object)
{
    KaMainWindow *self = KA_MAIN_WINDOW(object);
    KaApplet *applet = KA_APPLET (g_application_get_default ());

    G_OBJECT_CLASS (ka_main_window_parent_class)->constructed (object);

    g_signal_connect (applet, "krb-ccache-changed",
                      G_CALLBACK(ccache_changed_cb),
                      self);

    g_signal_connect (applet, "krb-tgt-acquired",
                      G_CALLBACK (disable_ticket_button_cb),
                      self);

    g_signal_connect (applet, "krb-tgt-expired",
                      G_CALLBACK (enable_ticket_button_cb),
                      self);
}


static void
ka_main_window_class_init (KaMainWindowClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->constructed = ka_main_window_constructed;

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/krb5-auth-dialog/ui/ka-main-window.ui");
    gtk_widget_class_bind_template_child (widget_class, KaMainWindow, stack);
    gtk_widget_class_bind_template_child (widget_class, KaMainWindow, get_ticket_btn);
    gtk_widget_class_bind_template_child (widget_class, KaMainWindow, tickets_view);
}


static void
ka_main_window_init (KaMainWindow *self)
{
    GtkCellRenderer *text_renderer, *toggle_renderer;
    GtkTreeViewColumn *column;

    gtk_widget_init_template (GTK_WIDGET (self));

    self->tickets = gtk_list_store_new (N_COLUMNS,
                                        G_TYPE_STRING,
                                        G_TYPE_STRING,
                                        G_TYPE_STRING,
                                        G_TYPE_BOOLEAN,
                                        G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

    gtk_tree_view_set_model (GTK_TREE_VIEW (self->tickets_view),
                             GTK_TREE_MODEL (self->tickets));

    text_renderer = gtk_cell_renderer_text_new ();
    g_object_set (text_renderer,
                  "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                  NULL);
    toggle_renderer = gtk_cell_renderer_toggle_new ();

    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                                                _("Principal"),
                                                text_renderer,
                                                "text",
                                                PRINCIPAL_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                                                _("Start"),
                                                text_renderer,
                                                "text",
                                                START_TIME_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                                                _("End"),
                                                text_renderer,
                                                "markup",
                                                END_TIME_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                      /* Translators: this is an abbreviation for forwardable */
                                                _("Fwd"),
                                                toggle_renderer,
                                                "active",
                                                FORWARDABLE_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                        /* Translators: this is an abbreviation for proxiable */
                                                _("Proxy"),
                                                toggle_renderer,
                                                "active",
                                                PROXIABLE_COLUMN,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(self->tickets_view), -1,
                        /* Translators: this is an abbreviation for renewable */
                                                _("Renew"),
                                                toggle_renderer,
                                                "active",
                                                RENEWABLE_COLUMN,
                                                NULL);

    for (int i = PRINCIPAL_COLUMN; i <= END_TIME_COLUMN; i++) {
        column = gtk_tree_view_get_column(self->tickets_view, i);
        g_object_set (column, "expand", TRUE, NULL);
    }

    g_signal_connect(self->tickets, "row-inserted",
                     G_CALLBACK(on_row_inserted),
                     self->stack);
    g_signal_connect(self->tickets, "row-deleted",
                     G_CALLBACK(on_row_deleted),
                     self->stack);
    on_row_deleted(GTK_TREE_MODEL(self->tickets), NULL, self->stack);
}


KaMainWindow *
ka_main_window_new (KaApplet *ka_applet)
{
    return KA_MAIN_WINDOW (g_object_new (KA_TYPE_MAIN_WINDOW,
                                         "application", ka_applet,
                                         NULL));
}


void
ka_main_window_show (KaMainWindow *self, gboolean show_conf_tickets)
{
    if (ka_get_service_tickets (self->tickets, !show_conf_tickets)) {
        gtk_window_present (GTK_WINDOW(self));
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
                          G_CALLBACK (ka_window_destroy), NULL);
        gtk_widget_show (message_dialog);
    }
}
