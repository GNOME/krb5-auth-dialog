/* Krb5 Auth Applet -- Acquire and release kerberos tickets
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "krb5-auth-tickets.h"
#include "krb5-auth-dialog.h"

static GtkListStore *tickets;
static GtkWidget    *tickets_dialog;

GtkWidget*
ka_tickets_dialog_create(GtkBuilder *xml)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *tickets_view;

	tickets = gtk_list_store_new (N_COLUMNS,
				      G_TYPE_STRING,
				      G_TYPE_STRING,
				      G_TYPE_STRING);

	tickets_dialog = GTK_WIDGET (gtk_builder_get_object (xml, "krb5_tickets_dialog"));
	tickets_view = GTK_TREE_VIEW (gtk_builder_get_object (xml, "krb5_tickets_treeview"));
	gtk_tree_view_set_model(GTK_TREE_VIEW(tickets_view), GTK_TREE_MODEL(tickets));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes("Principal",
							  renderer,
							  "text",
							  PRINCIPAL_COLUMN,
							  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tickets_view), column);
	column = gtk_tree_view_column_new_with_attributes("Start Time",
							  renderer,
							  "text",
							  START_TIME_COLUMN,
							  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tickets_view), column);
	column = gtk_tree_view_column_new_with_attributes("End Time",
							  renderer,
							  "markup",
							  END_TIME_COLUMN,
							  NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tickets_view), column);
	return tickets_dialog;
}

void
ka_tickets_dialog_run()
{
	if (ka_get_service_tickets(tickets)) {
		gtk_widget_show(tickets_dialog);
		gtk_dialog_run(GTK_DIALOG(tickets_dialog));
		gtk_widget_hide(tickets_dialog);
	} else {
		GtkWidget *message_dialog;

		message_dialog = gtk_message_dialog_new (NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_("Error displaying service ticket information"));
		gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);

		g_signal_connect (message_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (message_dialog);
	}
}

