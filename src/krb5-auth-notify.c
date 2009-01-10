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
 */

#include "config.h"

#ifdef HAVE_LIBNOTIFY

#include "krb5-auth-applet.h"
#include "krb5-auth-notify.h"

void
ka_send_event_notification (Krb5AuthApplet *applet,
			    NotifyUrgency urgency,
			    const char *summary,
			    const char *message,
			    const char *icon)
{
        const char *notify_icon;

        g_return_if_fail (applet != NULL);
        g_return_if_fail (summary != NULL);
        g_return_if_fail (message != NULL);

        if (!notify_is_initted ())
                notify_init (PACKAGE);

        if (applet->notification != NULL) {
                notify_notification_close (applet->notification, NULL);
                g_object_unref (applet->notification);
        }

        notify_icon = icon ? icon : "gtk-dialog-authentication";

        applet->notification = \
		notify_notification_new_with_status_icon(summary, message, notify_icon, applet->tray_icon);

        notify_notification_set_urgency (applet->notification, urgency);
        notify_notification_show (applet->notification, NULL);
}

#endif /* HAVE_LIBNOTIFY */
