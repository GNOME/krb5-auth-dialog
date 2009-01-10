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

#ifndef KRB5_AUTH_APPLET_H
#define KRB5_AUTH_APPLET_H

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY */
#include <krb5.h>

#include "config.h"

typedef struct {
	GtkStatusIcon* tray_icon;	/* the tray icon */
	GtkWidget* context_menu;	/* the tray icon's context menu */
	const char* icons[3]; 		/* for invalid, expiring and valid tickts */
	gboolean show_trayicon;		/* show the trayicon */

	/* The password dialog */
	GtkWidget* pw_dialog;		/* the password dialog itself */
	GladeXML*  pw_xml;		/* the dialog's glade xml */
	GtkWidget* pw_wrong_label;	/* the wrong password/timeout label */
	int	   pw_prompt_secs;	/* when to start prompting for a password */
	gboolean   pw_dialog_persist;	/* don't hide the dialog when creds are still valid */

#ifdef HAVE_LIBNOTIFY
	NotifyNotification* notification;/* notification messages */
#endif /* HAVE_LIBNOTIFY */
	char* principal;		/* the principal to request */
	gboolean renewable;		/* credentials renewable? */
	char* pk_userid;		/* "userid" for pkint */
} Krb5AuthApplet;

Krb5AuthApplet* ka_create_applet();
/* update tooltip and icon */
int ka_update_status(Krb5AuthApplet* applet, krb5_timestamp expiry);
/* show or hide the tray icon */
gboolean ka_show_tray_icon(Krb5AuthApplet* applet);

#ifdef ENABLE_DEBUG
#define KA_DEBUG(fmt,...) \
    g_printf ("DEBUG: %s: " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define KA_DEBUG(fmt,...) \
    do { } while (0)
#endif /* !ENABLE_DEBUG */

#endif
