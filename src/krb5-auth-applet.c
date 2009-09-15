/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2009 Guido Guenther <agx@sigxcpu.org>
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
#include "krb5-auth-gconf-tools.h"
#include "krb5-auth-gconf.h"
#include "krb5-auth-tools.h"
#include "krb5-auth-tickets.h"
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#define NOTIFY_SECONDS 300

enum ka_icon {
	inv_icon = 0,
	exp_icon,
	val_icon,
};

enum
{
  KA_PROP_0 = 0,
  KA_PROP_PRINCIPAL,
  KA_PROP_PK_USERID,
  KA_PROP_PK_ANCHORS,
  KA_PROP_TRAYICON,
  KA_PROP_PW_PROMPT_MINS,
  KA_PROP_TGT_FORWARDABLE,
  KA_PROP_TGT_PROXIABLE,
  KA_PROP_TGT_RENEWABLE,
};

struct _KaApplet {
  GObject parent;

  KaAppletPrivate *priv;
};

struct _KaAppletClass {
  GObjectClass parent;
};

G_DEFINE_TYPE(KaApplet, ka_applet, G_TYPE_OBJECT);

struct _KaAppletPrivate
{
	GtkBuilder *uixml;
	GtkStatusIcon* tray_icon;	/* the tray icon */
	GtkWidget* context_menu;	/* the tray icon's context menu */
	const char* icons[3]; 		/* for invalid, expiring and valid tickts */
	gboolean show_trayicon;		/* show the trayicon */

	KaPwDialog *pwdialog;		/* the password dialog */
	int	   pw_prompt_secs;	/* when to start prompting for a password */

#ifdef HAVE_LIBNOTIFY
	NotifyNotification* notification;/* notification messages */
	const char* notify_gconf_key;	/* disable notification gconf key */
#endif /* HAVE_LIBNOTIFY */
	char* principal;		/* the principal to request */
	gboolean renewable;		/* credentials renewable? */
	char* pk_userid;		/* "userid" for pkint */
	char* pk_anchors;		/* trust anchors for pkint */
	gboolean tgt_forwardable;	/* request a forwardable ticket */
	gboolean tgt_renewable;		/* request a renewable ticket */
	gboolean tgt_proxiable;		/* request a proxiable ticket */

	GConfClient *gconf;		/* gconf client */
};

static void
ka_applet_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  KaApplet* self = KA_APPLET (object);

  switch (property_id) {
    case KA_PROP_PRINCIPAL:
	g_free (self->priv->principal);
	self->priv->principal = g_value_dup_string (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->principal);
	break;

     case KA_PROP_PK_USERID:
	g_free (self->priv->pk_userid);
	self->priv->pk_userid = g_value_dup_string (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->pk_userid);
	break;

     case KA_PROP_PK_ANCHORS:
	g_free (self->priv->pk_anchors);
	self->priv->pk_anchors = g_value_dup_string (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->pk_anchors);
	break;

    case KA_PROP_TRAYICON:
	self->priv->show_trayicon = g_value_get_boolean (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->show_trayicon ? "True" : "False");
	break;

    case KA_PROP_PW_PROMPT_MINS:
	self->priv->pw_prompt_secs = g_value_get_uint (value) * 60;
	KA_DEBUG ("%s: %d", pspec->name, self->priv->pw_prompt_secs/60);
	break;

    case KA_PROP_TGT_FORWARDABLE:
	self->priv->tgt_forwardable = g_value_get_boolean (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->tgt_forwardable ? "True" : "False");
	break;

    case KA_PROP_TGT_PROXIABLE:
	self->priv->tgt_proxiable = g_value_get_boolean (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->tgt_proxiable ? "True" : "False");
	break;

    case KA_PROP_TGT_RENEWABLE:
	self->priv->tgt_renewable = g_value_get_boolean (value);
	KA_DEBUG ("%s: %s", pspec->name, self->priv->tgt_renewable ? "True" : "False");
	break;

    default:
	/* We don't have any other property... */
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	break;
    }
}

static void
ka_applet_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  KaApplet *self = KA_APPLET (object);

  switch (property_id)
    {
    case KA_PROP_PRINCIPAL:
	g_value_set_string (value, self->priv->principal);
	break;

    case KA_PROP_PK_USERID:
	g_value_set_string (value, self->priv->pk_userid);
	break;

    case KA_PROP_PK_ANCHORS:
	g_value_set_string (value, self->priv->pk_anchors);
	break;

    case KA_PROP_TRAYICON:
	g_value_set_boolean (value, self->priv->show_trayicon);
	break;

    case KA_PROP_PW_PROMPT_MINS:
	g_value_set_uint (value, self->priv->pw_prompt_secs / 60);
	break;

    case KA_PROP_TGT_FORWARDABLE:
	g_value_set_boolean (value, self->priv->tgt_forwardable);
	break;

    case KA_PROP_TGT_PROXIABLE:
	g_value_set_boolean (value, self->priv->tgt_proxiable);
	break;

    case KA_PROP_TGT_RENEWABLE:
	g_value_set_boolean (value, self->priv->tgt_renewable);
	break;

    default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	break;
    }
}


static void
ka_applet_dispose(GObject* object)
{
	KaApplet* applet = KA_APPLET(object);
	GObjectClass *parent_class = G_OBJECT_CLASS (ka_applet_parent_class);

	if (applet->priv->tray_icon) {
		g_object_unref(applet->priv->tray_icon);
		applet->priv->tray_icon = NULL;
	}
	if (applet->priv->pwdialog) {
		g_object_unref(applet->priv->pwdialog);
		applet->priv->pwdialog = NULL;
	}
	if (applet->priv->uixml) {
		g_object_unref(applet->priv->uixml);
		applet->priv->uixml = NULL;
	}

	if (parent_class->dispose != NULL)
		parent_class->dispose (object);
}


static void
ka_applet_finalize(GObject *object)
{
	KaApplet* applet = KA_APPLET(object);
	GObjectClass *parent_class = G_OBJECT_CLASS (ka_applet_parent_class);

	g_free (applet->priv->principal);
	g_free (applet->priv->pk_userid);
	g_free (applet->priv->pk_anchors);
	/* no need to free applet->priv */

	if (parent_class->finalize != NULL)
		parent_class->finalize (object);
}

static void
ka_applet_init(KaApplet *applet)
{
	applet->priv = G_TYPE_INSTANCE_GET_PRIVATE(applet,
						   KA_TYPE_APPLET,
						   KaAppletPrivate);
}

static void
ka_applet_class_init(KaAppletClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	object_class->dispose = ka_applet_dispose;
	object_class->finalize = ka_applet_finalize;
	g_type_class_add_private(klass, sizeof(KaAppletPrivate));

	object_class->set_property = ka_applet_set_property;
	object_class->get_property = ka_applet_get_property;

	pspec = g_param_spec_string ("principal",
				     "Principal",
				     "Get/Set Kerberos principal",
				     "",
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_PRINCIPAL,
                                         pspec);

	pspec = g_param_spec_string ("pk-userid",
				     "PKinit identifier",
				     "Get/Set Pkinit identifier",
				     "",
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_PK_USERID,
                                         pspec);

	pspec = g_param_spec_string ("pk-anchors",
				     "PKinit trust anchors",
				     "Get/Set Pkinit trust anchors",
				     "",
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_PK_ANCHORS,
                                         pspec);

	pspec = g_param_spec_boolean("show-trayicon",
				     "Show tray icon",
				     "Show/Hide the tray icon",
				     TRUE,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_TRAYICON,
                                         pspec);

	pspec = g_param_spec_uint   ("pw-prompt-mins",
				     "Password prompting interval",
				     "Password prompting interval in minutes",
				     0, G_MAXUINT, MINUTES_BEFORE_PROMPTING,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_PW_PROMPT_MINS,
                                         pspec);

	pspec = g_param_spec_boolean("tgt-forwardable",
				     "Forwardable ticket",
				     "wether to request forwardable tickets",
				     FALSE,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_TGT_FORWARDABLE,
                                         pspec);

	pspec = g_param_spec_boolean("tgt-proxiable",
				     "Proxiable ticket",
				     "wether to request proxiable tickets",
				     FALSE,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_TGT_PROXIABLE,
                                         pspec);

	pspec = g_param_spec_boolean("tgt-renewable",
				     "Renewable ticket",
				     "wether to request renewable tickets",
				     FALSE,
				     G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
                                         KA_PROP_TGT_RENEWABLE,
                                         pspec);
}


static KaApplet*
ka_applet_new(void)
{
	return g_object_new (KA_TYPE_APPLET, NULL);
}


/* determine the new tooltip text */
static char*
ka_applet_tooltip_text(int remaining)
{
	int hours, minutes;
	gchar* tooltip_text;

	if (remaining > 0) {
		if (remaining >= 3600) {
			hours = remaining / 3600;
			minutes = (remaining % 3600) / 60;
			/* Translators: First number is hours, second number is minutes */
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
ka_applet_select_icon(KaApplet* applet, int remaining)
{
	enum ka_icon tray_icon = inv_icon;

	if (remaining > 0) {
		if (remaining < applet->priv->pw_prompt_secs &&
		    !applet->priv->renewable)
		    	tray_icon = exp_icon;
		else			
			tray_icon = val_icon;
	}

	return applet->priv->icons[tray_icon];
}


#ifdef HAVE_LIBNOTIFY
static gboolean
ka_show_notification (KaApplet *applet)
{
	/* wait for the panel to be settled before showing a bubble */
	if (gtk_status_icon_is_embedded (applet->priv->tray_icon)) {
		GError *error = NULL;
		gboolean ret;

		ret = notify_notification_show (applet->priv->notification, &error);
		if (!ret) {
			g_assert (error);
			g_assert (error->message);
			g_warning ("Failed to show notification: %s", error->message);
			g_clear_error (&error);
		}
	} else {
		g_timeout_add_seconds (5, (GSourceFunc)ka_show_notification, applet);
	}
	return FALSE;
}


static void
ka_notify_action_cb (NotifyNotification *notification G_GNUC_UNUSED,
	             gchar *action, gpointer user_data)
{
	KaApplet *self = KA_APPLET (user_data);

	if (strcmp (action, "dont-show-again") == 0) {
		KA_DEBUG ("turning of notification %s", self->priv->notify_gconf_key);
		ka_gconf_set_bool (self->priv->gconf,
				   self->priv->notify_gconf_key,
				   FALSE);
		self->priv->notify_gconf_key = NULL;
	} else {
		g_warning("unkonwn action for callback");
	}
}


static void
ka_send_event_notification (KaApplet *applet,
			    const char *summary,
			    const char *message,
			    const char *icon,
			    const char *action)
{
        const char *notify_icon;

	g_return_if_fail (applet != NULL);
	g_return_if_fail (summary != NULL);
	g_return_if_fail (message != NULL);

	if (!notify_is_initted ())
		notify_init (PACKAGE);

	if (applet->priv->notification != NULL) {
		notify_notification_close (applet->priv->notification, NULL);
		g_object_unref (applet->priv->notification);
	}

	notify_icon = icon ? icon : "krb-valid-ticket";

	applet->priv->notification = \
		notify_notification_new_with_status_icon(summary,
							 message,
							 notify_icon,
							 applet->priv->tray_icon);

	notify_notification_set_urgency (applet->priv->notification, NOTIFY_URGENCY_NORMAL);
	notify_notification_add_action (applet->priv->notification,
					action,
					_("Don't show me this again"),
					(NotifyActionCallback) ka_notify_action_cb,
					applet, NULL);
	ka_show_notification (applet);
}
#else
static void
ka_send_event_notification (KaApplet *applet G_GNUC_UNUSED,
			    const char *summary G_GNUC_UNUSED,
			    const char *message G_GNUC_UNUSED,
			    const char *icon G_GNUC_UNUSED,
			    const char *action G_GNUC_UNUSED)
{
}
#endif /* ! HAVE_LIBNOTIFY */


/* update the tray icon's tooltip and icon */
int
ka_applet_update_status(KaApplet* applet, krb5_timestamp expiry)
{
	int now = time(0);
	int remaining = expiry - now;
	static int last_warn = 0;
	static gboolean expiry_notified = FALSE;
	gboolean notify = TRUE;
	const char* tray_icon = ka_applet_select_icon (applet, remaining);
	char* tooltip_text = ka_applet_tooltip_text (remaining);

	if (remaining > 0) {
		if (expiry_notified) {
			ka_gconf_get_bool(applet->priv->gconf,
					  KA_GCONF_KEY_NOTIFY_VALID,
					  &notify);
			if (notify) {
				applet->priv->notify_gconf_key = KA_GCONF_KEY_NOTIFY_VALID;
				ka_send_event_notification (applet,
						_("Network credentials valid"),
						_("You've refreshed your Kerberos credentials."),
						"krb-valid-ticket",
						"dont-show-again");
			}
			expiry_notified = FALSE;
		} else if (remaining < applet->priv->pw_prompt_secs && (now - last_warn) > NOTIFY_SECONDS &&
			   !applet->priv->renewable) {
			ka_gconf_get_bool(applet->priv->gconf,
					  KA_GCONF_KEY_NOTIFY_EXPIRING,
					  &notify);
			if (notify) {
				applet->priv->notify_gconf_key = KA_GCONF_KEY_NOTIFY_EXPIRING;
				ka_send_event_notification (applet,
						_("Network credentials expiring"),
						tooltip_text,
						"krb-expiring-ticket",
						"dont-show-again");
			}
			last_warn = now;
		}
	} else {
		if (!expiry_notified) {
			ka_gconf_get_bool(applet->priv->gconf,
					  KA_GCONF_KEY_NOTIFY_EXPIRED,
					  &notify);
			if (notify) {
				applet->priv->notify_gconf_key = KA_GCONF_KEY_NOTIFY_EXPIRED;
				ka_send_event_notification (applet,
						_("Network credentials expired"),
						_("Your Kerberos credentails have expired."),
						"krb-no-valid-ticket",
						"dont-show-again");
			}
			expiry_notified = TRUE;
			last_warn = 0;
		}
	}

	gtk_status_icon_set_from_icon_name (applet->priv->tray_icon, tray_icon);
	gtk_status_icon_set_tooltip (applet->priv->tray_icon, tooltip_text);
	g_free(tooltip_text);
	return 0;
}


static void
ka_applet_menu_add_separator_item (GtkWidget* menu)
{
	GtkWidget* menu_item;

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);
}

static void
ka_applet_cb_preferences (GtkWidget* menuitem G_GNUC_UNUSED,
                          gpointer user_data G_GNUC_UNUSED)
{
	GError *error = NULL;

	g_spawn_command_line_async (BIN_DIR
				    G_DIR_SEPARATOR_S
				    "krb5-auth-dialog-preferences",
				    &error);
	if (error) {
		GtkWidget *message_dialog;

		message_dialog = gtk_message_dialog_new (NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_("There was an error launching the preferences dialog: %s"),
					error->message);
		gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);

		g_signal_connect (message_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (message_dialog);
		g_error_free (error);
	  }
}


/* Free all resources and quit */
static void
ka_applet_cb_quit (GtkMenuItem* menuitem G_GNUC_UNUSED, gpointer user_data)
{
	KaApplet* applet = KA_APPLET(user_data);

	g_object_unref (applet);
	gtk_main_quit ();
}


static void
ka_about_dialog_url_hook (GtkAboutDialog *about,
			  const gchar *alink,
			  gpointer data G_GNUC_UNUSED)
{
	GError *error = NULL;

	gtk_show_uri(gtk_window_get_screen (GTK_WINDOW (about)),
		     alink, gtk_get_current_event_time(), &error);

	if (error) {
		GtkWidget *message_dialog;

		message_dialog = gtk_message_dialog_new (GTK_WINDOW (about),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_("There was an error displaying %s:\n%s"),
					alink, error->message);
		gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);

		g_signal_connect (message_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (message_dialog);
		g_error_free (error);
	  }
}


static void
ka_applet_cb_about_dialog (GtkMenuItem* menuitem G_GNUC_UNUSED,
			   gpointer user_data G_GNUC_UNUSED)
{
	const gchar* authors[] = {
				"Christopher Aillon <caillon@redhat.com>",
				"Jonathan Blandford <jrb@redhat.com>",
				"Colin Walters <walters@verbum.org>",
				"Guido Günther <agx@sigxcpu.org>",
				NULL };

	gtk_about_dialog_set_url_hook (ka_about_dialog_url_hook, NULL, NULL);
	gtk_show_about_dialog (NULL,
			       "authors", authors,
			       "version", VERSION,
			       "logo-icon-name", "krb-valid-ticket",
			       "copyright",
			       "Copyright (C) 2004,2005,2006 Red Hat, Inc.,\n"
			       "2008,2009 Guido Günther",
			       "website-label", PACKAGE " website",
			       "website", "https://honk.sigxcpu.org/piki/projects/krb5-auth-dialog/",
			       "license", "GNU General Public License Version 2",
			       /* Translators: add the translators of your language here */
			       "translator-credits", _("translator-credits"),
			       NULL);
}


static void
ka_applet_cb_show_help (GtkMenuItem* menuitem G_GNUC_UNUSED,
			gpointer user_data)
{
	KaApplet *applet = KA_APPLET(user_data);

	ka_show_help (gtk_status_icon_get_screen(applet->priv->tray_icon), NULL, NULL);
}


static void
ka_applet_cb_destroy_ccache(GtkMenuItem* menuitem G_GNUC_UNUSED,
			    gpointer user_data)
{
	KaApplet *applet = KA_APPLET(user_data);
	ka_destroy_ccache(applet);
}

static void
ka_applet_cb_show_tickets(GtkMenuItem* menuitem G_GNUC_UNUSED,
			  gpointer user_data G_GNUC_UNUSED)
{
	ka_tickets_dialog_run();
}


/* The tray icon's context menu */
static gboolean
ka_applet_create_context_menu (KaApplet* applet)
{
	GtkWidget* menu;
	GtkWidget* menu_item;
	GtkWidget* image;

	menu = gtk_menu_new ();

	/* kdestroy */
	menu_item = gtk_image_menu_item_new_with_mnemonic (_("Remove Credentials _Cache"));
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_destroy_ccache), applet);
	image = gtk_image_new_from_stock (GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	ka_applet_menu_add_separator_item (menu);

	/* Ticket dialog */
	menu_item = gtk_image_menu_item_new_with_mnemonic("_List Tickets");
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_show_tickets), applet);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	/* Preferences */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_preferences), applet);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	/* About item */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_show_help), applet);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	/* About item */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_about_dialog), applet);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	ka_applet_menu_add_separator_item (menu);

	/* Quit */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	g_signal_connect (G_OBJECT (menu_item), "activate",
			  G_CALLBACK (ka_applet_cb_quit), applet);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	gtk_widget_show_all (menu);
	applet->priv->context_menu = menu;

	return TRUE;
}


static void
ka_tray_icon_on_menu (GtkStatusIcon* status_icon G_GNUC_UNUSED,
                      guint button,
                      guint activate_time,
                      gpointer user_data)
{
	KaApplet *applet = KA_APPLET(user_data);

	KA_DEBUG("Trayicon right clicked: %d", applet->priv->pw_prompt_secs);
	gtk_menu_popup (GTK_MENU (applet->priv->context_menu), NULL, NULL,
			gtk_status_icon_position_menu, applet->priv->tray_icon,
			button, activate_time);
}


static gboolean
ka_tray_icon_on_click (GtkStatusIcon* status_icon G_GNUC_UNUSED,
                       gpointer data)
{
	KaApplet *applet = KA_APPLET(data);

	KA_DEBUG("Trayicon clicked: %d", applet->priv->pw_prompt_secs);
	ka_grab_credentials (applet);
	return TRUE;
}


static gboolean
ka_applet_cb_show_trayicon (KaApplet* applet,
                            GParamSpec* property G_GNUC_UNUSED,
                            gpointer data G_GNUC_UNUSED)
{
	g_return_val_if_fail (applet != NULL, FALSE);
	g_return_val_if_fail (applet->priv->tray_icon != NULL, FALSE);

	gtk_status_icon_set_visible (applet->priv->tray_icon, applet->priv->show_trayicon);
	return TRUE;
}


static gboolean
ka_applet_create_tray_icon (KaApplet* applet)
{
	GtkStatusIcon* tray_icon;

	tray_icon = gtk_status_icon_new ();

	g_signal_connect (G_OBJECT(tray_icon), "activate",
			  G_CALLBACK(ka_tray_icon_on_click), applet);
	g_signal_connect (G_OBJECT(tray_icon),
			  "popup-menu",
			  G_CALLBACK(ka_tray_icon_on_menu), applet);
	gtk_status_icon_set_from_icon_name (tray_icon, applet->priv->icons[exp_icon]);
	gtk_status_icon_set_tooltip (tray_icon, PACKAGE);
        applet->priv->tray_icon = tray_icon;
	return TRUE;
}

static int
ka_applet_setup_icons (KaApplet* applet)
{
	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   DATA_DIR G_DIR_SEPARATOR_S "icons");
	applet->priv->icons[val_icon] = "krb-valid-ticket";
	applet->priv->icons[exp_icon] = "krb-expiring-ticket";
	applet->priv->icons[inv_icon] = "krb-no-valid-ticket";
	return TRUE;
}

guint
ka_applet_get_pw_prompt_secs(const KaApplet* applet)
{
	return applet->priv->pw_prompt_secs;
}

gboolean
ka_applet_get_show_trayicon(const KaApplet* applet)
{
	return applet->priv->show_trayicon;
}

void
ka_applet_set_tgt_renewable(KaApplet* applet, gboolean renewable)
{
	applet->priv->renewable = renewable;
}

gboolean
ka_applet_get_tgt_renewable(const KaApplet* applet)
{
	return applet->priv->renewable;
}

KaPwDialog*
ka_applet_get_pwdialog(const KaApplet* applet)
{
	return applet->priv->pwdialog;
}

/* create the tray icon applet */
KaApplet*
ka_applet_create()
{
	KaApplet* applet = ka_applet_new();
	GError *error = NULL;
	gboolean ret;

	if (!(ka_applet_setup_icons (applet)))
		g_error ("Failure to setup icons");
	if (!ka_applet_create_tray_icon (applet))
		g_error ("Failure to create tray icon");
	if (!ka_applet_create_context_menu (applet))
		g_error ("Failure to create context menu");
	gtk_window_set_default_icon_name (applet->priv->icons[val_icon]);
	g_signal_connect (applet, "notify::show-trayicon",
	                  G_CALLBACK (ka_applet_cb_show_trayicon), NULL);

	applet->priv->uixml = gtk_builder_new();
	ret = gtk_builder_add_from_file(applet->priv->uixml,
					KA_DATA_DIR G_DIR_SEPARATOR_S
				        PACKAGE ".xml", &error);
	if (!ret) {
		g_assert (error);
		g_assert (error->message);
		g_error ("Failed to load UI XML: %s", error->message);
	}
	applet->priv->pwdialog = ka_pwdialog_create(applet->priv->uixml);
	g_return_val_if_fail (applet->priv->pwdialog != NULL, NULL);

	applet->priv->gconf = ka_gconf_init (applet);
	g_return_val_if_fail (applet->priv->gconf != NULL, NULL);

	ka_tickets_dialog_create(applet->priv->uixml);

	return applet;
}

