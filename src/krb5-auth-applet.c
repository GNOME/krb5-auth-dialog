/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
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

#include "krb5-auth-applet.h"
#include "krb5-auth-dialog.h"
#include "krb5-auth-notify.h"

#define NOTIFY_SECONDS 300

enum ka_icon {
	inv_icon = 0,
	exp_icon,
	val_icon,
};


/* determine the new tooltip text */
static char*
ka_tooltip_text(Krb5AuthApplet* applet, int remaining) 
{
	int hours, minutes;
	gchar* tooltip_text;

	if (remaining > 0) {
		if (remaining >= 3600) {
			hours = remaining / 3600;
			minutes = (remaining % 3600) / 60;
			tooltip_text = g_strdup_printf (_("Your credentials expire in %.2d:%.2dh"), hours, minutes);
		} else {
			minutes = remaining / 60;
			tooltip_text = g_strdup_printf (ngettext(
							"Your credentials expire in %d minute",
							"Your credentials expire in %d minutes",
							minutes), minutes);
		}
	} else
		tooltip_text = g_strdup (_("Your credentials have expired"));
	return tooltip_text;
}


/* determine the current icon */
static const char*
ka_select_icon(Krb5AuthApplet* applet, int remaining)
{
	enum ka_icon tray_icon = inv_icon;

	if (remaining > 0) {
		if (remaining < applet->pw_prompt_secs &&
		    !applet->renewable)
		    	tray_icon = exp_icon;
		else			
			tray_icon = val_icon;
	}

	return applet->icons[tray_icon];
}


/* update the tray icon's tooltip and icon */
int
ka_update_status(Krb5AuthApplet* applet, krb5_timestamp expiry)
{
	int now = time(0);
	int remaining = expiry - now;
	static int last_warn = 0;
	static gboolean expiry_notified = FALSE;
	const char* tray_icon = ka_select_icon (applet, remaining);
	char* tooltip_text = ka_tooltip_text (applet, remaining);

	if (remaining > 0) {
		if (expiry_notified) {
			ka_send_event_notification (applet,
						_("Network credentials valid"),
						_("Your Kerberos credentials have been refreshed."), NULL);
			expiry_notified = FALSE;
		} else if (remaining < applet->pw_prompt_secs && (now - last_warn) > NOTIFY_SECONDS &&
		           !applet->renewable) {
			ka_send_event_notification (applet,
						_("Network credentials expiring"),
						tooltip_text, NULL);
			last_warn = now;
		}
	} else {
		if (!expiry_notified) {
			ka_send_event_notification (applet,
						_("Network credentials expired"),
						_("Your Kerberos credentails have expired."), NULL);
			expiry_notified = TRUE;
			last_warn = 0;
		}
	}

	gtk_status_icon_set_from_icon_name (applet->tray_icon, tray_icon);
	gtk_status_icon_set_tooltip (applet->tray_icon, tooltip_text);
	g_free(tooltip_text);
	return 0;
}


static void
ka_menu_add_separator_item (GtkWidget* menu)
{
        GtkWidget* menu_item;

        menu_item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
        gtk_widget_show (menu_item);
}


/* Free all resources and quit */
static void
ka_quit_applet (GtkMenuItem* menuitem, gpointer user_data)
{
	Krb5AuthApplet* applet = (Krb5AuthApplet*) user_data;

	g_free (applet->principal);
	g_free (applet);
	gtk_main_quit ();
}


static void
ka_about_dialog (GtkMenuItem* menuitem, gpointer user_data)
{
	gchar* authors[] = {  "Christopher Aillon <caillon@redhat.com>",
			      "Colin Walters <walters@verbum.org>",
		              "Guido Günther <agx@sigxpcu.org>",
			      NULL };
	gtk_show_about_dialog (NULL,
                              "authors", authors,
                              "version", VERSION,
                              "copyright", "Copyright (C) 2004,2005,2006 Red Hat, Inc.,\n2008 Guido Günther",
                               NULL);
}


/* The tray icon's context menu */
static GtkWidget*
ka_create_context_menu (Krb5AuthApplet* applet)
{
	GtkWidget* menu;
	GtkWidget* menu_item;
	GtkWidget* image;

	menu = gtk_menu_new ();

	/* kdestroy */
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("Remove Credentials _Cache"));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (ka_destroy_cache), applet);
        image = gtk_image_new_from_stock (GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	ka_menu_add_separator_item (menu);

	/* About item */
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("_About"));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (ka_about_dialog), applet);
	image = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	ka_menu_add_separator_item (menu);

	/* Quit */
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("_Quit"));
	g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (ka_quit_applet), applet);
        image = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	gtk_widget_show_all (menu);

	return menu;
}


static void
ka_tray_icon_on_menu (GtkStatusIcon* status_icon, guint button,
                       guint activate_time, gpointer user_data)
{
	Krb5AuthApplet* applet = (Krb5AuthApplet*) user_data;

	KA_DEBUG("Trayicon right clicked: %d", applet->pw_prompt_secs);
	gtk_menu_popup (GTK_MENU (applet->context_menu), NULL, NULL,
	                gtk_status_icon_position_menu, applet->tray_icon,
			button, activate_time);
}


static gboolean
ka_tray_icon_on_click (GtkStatusIcon* status_icon, gpointer data)
{
	Krb5AuthApplet* applet = (Krb5AuthApplet*) data;
	g_return_val_if_fail (applet != NULL, FALSE);

	KA_DEBUG("Trayicon clicked: %d", applet->pw_prompt_secs);
	ka_grab_credentials (applet);
	return TRUE;
}


gboolean
ka_show_tray_icon (Krb5AuthApplet* applet)
{
	g_return_val_if_fail (applet != NULL, FALSE);
	g_return_val_if_fail (applet->tray_icon != NULL, FALSE);

	gtk_status_icon_set_visible (applet->tray_icon, applet->show_trayicon);
	return TRUE;
}


static GtkStatusIcon*
ka_create_tray_icon (Krb5AuthApplet* applet)
{
	GtkStatusIcon* tray_icon;

	tray_icon = gtk_status_icon_new ();
	g_signal_connect (G_OBJECT(tray_icon), "activate",
                          G_CALLBACK(ka_tray_icon_on_click), applet);
	g_signal_connect (G_OBJECT(tray_icon),
			  "popup-menu",
			  G_CALLBACK(ka_tray_icon_on_menu), applet);
        gtk_status_icon_set_from_icon_name (tray_icon, applet->icons[exp_icon]);
        gtk_status_icon_set_tooltip (tray_icon, PACKAGE);
        return tray_icon;
}


int
ka_setup_icons (Krb5AuthApplet* applet)
{
	applet->icons[val_icon] = "krb-valid-ticket";
	applet->icons[exp_icon] = "krb-expiring-ticket";
	applet->icons[inv_icon] = "krb-no-valid-ticket";
	return TRUE;
}


/* create the tray icon applet */
Krb5AuthApplet*
ka_create_applet()
{
	Krb5AuthApplet* applet = g_malloc0 (sizeof(Krb5AuthApplet));

	if (!(ka_setup_icons (applet)))
		g_error ("Failure to setup icons");
	if (!(applet->tray_icon = ka_create_tray_icon (applet)))
		g_error ("Failure to create tray icon");
	if (!(applet->context_menu = ka_create_context_menu (applet)))
		g_error ("Failure to create context menu");
	gtk_window_set_default_icon_name (applet->icons[val_icon]);
	ka_show_tray_icon (applet);

	return applet;
}

