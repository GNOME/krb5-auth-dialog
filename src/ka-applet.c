/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008,2009,2010,2013 Guido Guenther <agx@sigxcpu.org>
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

#include "ka-applet-priv.h"
#include "ka-dbus.h"
#include "ka-kerberos.h"
#include "ka-settings.h"
#include "ka-tools.h"
#include "ka-main-window.h"
#include "ka-plugin-loader.h"
#include "ka-preferences.h"
#include "ka-closures.h"
#include <libnotify/notify.h>

#include <signal.h>

#define NOTIFY_SECONDS 300

enum ka_icon {
    inv_icon = 0,
    exp_icon,
    val_icon,
};

enum {
    KA_PROP_0 = 0,
    KA_PROP_PRINCIPAL,
    KA_PROP_PK_USERID,
    KA_PROP_PK_ANCHORS,
    KA_PROP_PW_PROMPT_MINS,
    KA_PROP_TGT_FORWARDABLE,
    KA_PROP_TGT_PROXIABLE,
    KA_PROP_TGT_RENEWABLE,
    KA_PROP_CONF_TICKETS,
};


enum {
    KA_DEBUG_NO_APP_MENU    = 1,  /* Disable gtk-shell-shows-app-menu gtk setting */
    KA_DEBUG_NO_HEADER_BAR  = 2,  /* Disable header-bar setting */
    KA_DEBUG_NO_PERSISTENCE = 4,  /* notification system does not support persistence */
};


const gchar *ka_signal_names[KA_SIGNAL_COUNT] = {
    "krb-tgt-acquired",
    "krb-tgt-renewed",
    "krb-tgt-expired",
    "krb-ccache-changed",
};


struct _KaApplet {
    GtkApplication parent;

    KaAppletPrivate *priv;
};

struct _KaAppletClass {
    GtkApplicationClass parent;

    guint signals[KA_SIGNAL_COUNT];
};

G_DEFINE_TYPE (KaApplet, ka_applet, GTK_TYPE_APPLICATION);

struct _KaAppletPrivate {
    GtkStatusIcon *tray_icon;   /* the tray icon */
    GtkWidget *context_menu;    /* the tray icon's context menu */
    const char *icons[3];       /* for invalid, expiring and valid tickts */
    gboolean ns_persistence;    /* does the notification server support persistence */

    KaPwDialog *pwdialog;       /* the password dialog */
    KaPreferences *prefs;       /* the prefs dialog */
    int pw_prompt_secs;         /* when to start sending notifications */
    KaPluginLoader *loader;     /* Plugin loader */

    /* command line and env handling */
    gboolean startup_ccache;    /* ccache found on startup */
    gboolean auto_run;          /* only start with valid ccache */
    gint debug_flags;           /* Debug options from environment */

    NotifyNotification *notification;   /* notification messages */
    char *krb_msg;              /* Additional banner delivered by Kerberos */

    /* GSettings options */
    const char *notify_key;     /* name of disable notification setting key */
    char *principal;            /* the principal to request */
    gboolean renewable;         /* credentials renewable? */
    char *pk_userid;            /* "userid" for pkint */
    char *pk_anchors;           /* trust anchors for pkint */
    gboolean tgt_forwardable;   /* request a forwardable ticket */
    gboolean tgt_renewable;     /* request a renewable ticket */
    gboolean tgt_proxiable;     /* request a proxiable ticket */
    gboolean conf_tickets;      /* whether to display configuration tickets */

    GSettings *settings;         /* GSettings client */
};


static void ka_close_notification (KaApplet *self);
static gboolean is_initialized;

static void
ka_applet_activate (GApplication *application G_GNUC_UNUSED)
{
    KaApplet *self = KA_APPLET(application);

    if (is_initialized) {
        KA_DEBUG ("Main window activated");
        ka_main_window_show (self);
    } else
        is_initialized = TRUE;
}


static int
ka_applet_command_line (GApplication            *application,
                        GApplicationCommandLine *cmdline G_GNUC_UNUSED)
{
    KaApplet *self = KA_APPLET(application);
    KA_DEBUG ("Evaluating command line");

    if (!self->priv->startup_ccache &&
        self->priv->auto_run)
        ka_applet_destroy (self);
    else
        ka_applet_activate (application);
    return 0;
}



static gint
ka_applet_local_command_line (GApplication *application,
                              gchar ***argv,
                              gint *exit_status)
{
    KaApplet *self = KA_APPLET(application);
    GOptionContext *context;
    GError *error = NULL;

    gint argc = g_strv_length (*argv);
    gint flags;
    gboolean auto_run = FALSE;

    const char *help_msg =
        "Run '" PACKAGE
        " --help' to see a full list of available command line options";
    const GOptionEntry options[] = {
        {"auto", 'a', 0, G_OPTION_ARG_NONE, &auto_run,
         "Only run if an initialized ccache is found", NULL},
        {NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}
    };

    KA_DEBUG ("Parsing local command line");
#if GLIB_CHECK_VERSION(2, 40, 0)
    flags = g_application_get_flags(application);
    g_application_set_flags(application, flags | G_APPLICATION_HANDLES_COMMAND_LINE);
#endif

    context = g_option_context_new ("- Kerberos 5 credential checking");
    g_option_context_add_main_entries (context, options, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    g_option_context_parse (context, &argc, argv, &error);

    if (error) {
        g_print ("%s\n%s\n", error->message, help_msg);
        g_clear_error (&error);
        *exit_status = 1;
    } else {
        self->priv->auto_run = auto_run;
        *exit_status = 0;
    }

    g_option_context_free (context);
    return FALSE;
}

GtkWindow *ka_applet_last_focused_window (KaApplet *self)
{
    GList *l = gtk_application_get_windows (GTK_APPLICATION(self));

    if (l != NULL )
        return g_list_first (l)->data;

    return NULL;
}


static void
action_remove_credentails_cache (GSimpleAction *action G_GNUC_UNUSED,
                                 GVariant *parameter G_GNUC_UNUSED,
                                 gpointer userdata)
{
    KaApplet *self = userdata;
    ka_destroy_ccache (self);
}


static void
action_list_tickets (GSimpleAction *action G_GNUC_UNUSED,
                     GVariant *parameter G_GNUC_UNUSED,
                     gpointer userdata)
{
    KaApplet *self = userdata;
    ka_main_window_show (self);
}


static void
action_preferences (GSimpleAction *action G_GNUC_UNUSED,
		    GVariant *parameter G_GNUC_UNUSED,
		    gpointer userdata)
{
    KaApplet *self = userdata;
    ka_preferences_run (self->priv->prefs);
}

static void
action_about (GSimpleAction *action G_GNUC_UNUSED,
              GVariant *parameter G_GNUC_UNUSED,
              gpointer userdata)
{
    KaApplet *self = KA_APPLET(userdata);

    ka_show_about (self);
}

static void
action_help (GSimpleAction *action G_GNUC_UNUSED,
              GVariant *parameter G_GNUC_UNUSED,
              gpointer userdata)
{
    KaApplet *self = KA_APPLET(userdata);
    GtkWindow *window = ka_applet_last_focused_window (self);

    ka_show_help (gtk_window_get_screen (window), NULL, NULL);
}

static void
action_quit (GSimpleAction *action G_GNUC_UNUSED,
             GVariant *parameter G_GNUC_UNUSED,
             gpointer userdata)
{
    KaApplet *self = KA_APPLET (userdata);

    ka_applet_destroy (self);
}


KaApplet *sigapplet;
static void
signal_handler (int signum)
{
    g_message ("Caught signal %d", signum);
    if (sigapplet)
        ka_applet_destroy (sigapplet);
}


static void
setup_signal_handlers (KaApplet *applet)
{
    struct sigaction sa;

    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigapplet = applet;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


static void
ka_applet_handle_debug(KaApplet *self)
{
    const gchar* debug;
    gchar **debug_opts, **opt;

    debug = g_getenv ("KRB5_AUTH_DIALOG_DEBUG");
    if (!debug)
        return;

    debug_opts = g_strsplit(debug, ",", -1);
    for (opt = debug_opts; *opt != NULL; opt++) {
        if (!g_strcmp0(*opt, "no-app-menu")) {
            KA_DEBUG ("Disabling app menu Gtk setting as requested...");
            g_object_set (gtk_settings_get_default (),
                          "gtk-shell-shows-app-menu", FALSE,
                          NULL);
            self->priv->debug_flags |= KA_DEBUG_NO_APP_MENU;
        } else if (!g_strcmp0(*opt, "no-header-bar")) {
            KA_DEBUG ("Disabling use-header-bar Gtk setting  as requested...");
            g_object_set (gtk_settings_get_default (),
                          "gtk-dialogs-use-header", FALSE,
                          NULL);
            self->priv->debug_flags |= KA_DEBUG_NO_HEADER_BAR;
        } else if (!g_strcmp0(*opt, "no-persistence")) {
            self->priv->debug_flags |= KA_DEBUG_NO_PERSISTENCE;
        } else {
            g_warning ("Unhandled debug options %s", *opt);
        }
    }

    g_strfreev (debug_opts);
}


static GActionEntry app_entries[] = {
    { "preferences", action_preferences, NULL, NULL, NULL, {0} },
    { "about", action_about, NULL, NULL, NULL, {0} },
    { "help", action_help, NULL, NULL, NULL, {0} },
    { "quit", action_quit, NULL, NULL, NULL, {0} },
};


static void
ka_applet_app_menu_create(KaApplet *self)

{
    GMenuModel *app_menu;
    GtkBuilder *builder;

    builder = gtk_builder_new_from_resource ("/org/gnome/krb5-auth-dialog/ui/app-menu.ui");
    app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu"));

    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     app_entries, G_N_ELEMENTS (app_entries),
                                     self);

    g_assert (app_menu != NULL);
    gtk_application_set_app_menu (GTK_APPLICATION(self),
                                  app_menu);

    g_object_unref (builder);
}

static void
ka_applet_startup (GApplication *application)
{
    KaApplet *self = KA_APPLET (application);

    KA_DEBUG ("Primary application");

    G_APPLICATION_CLASS (ka_applet_parent_class)->startup (application);

    self->priv->startup_ccache = ka_kerberos_init (self);
    ka_main_window_create (self);
    self->priv->prefs = ka_preferences_new (self);

    ka_applet_app_menu_create(self);
}

static void
ka_applet_set_property (GObject *object,
                        guint property_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
    KaApplet *self = KA_APPLET (object);

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

    case KA_PROP_PW_PROMPT_MINS:
        self->priv->pw_prompt_secs = g_value_get_uint (value) * 60;
        KA_DEBUG ("%s: %d", pspec->name, self->priv->pw_prompt_secs / 60);
        break;

    case KA_PROP_TGT_FORWARDABLE:
        self->priv->tgt_forwardable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->priv->tgt_forwardable ? "True" : "False");
        break;

    case KA_PROP_TGT_PROXIABLE:
        self->priv->tgt_proxiable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->priv->tgt_proxiable ? "True" : "False");
        break;

    case KA_PROP_TGT_RENEWABLE:
        self->priv->tgt_renewable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->priv->tgt_renewable ? "True" : "False");
        break;

    case KA_PROP_CONF_TICKETS:
        self->priv->conf_tickets = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->priv->tgt_renewable ? "True" : "False");
        break;

    default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ka_applet_get_property (GObject *object,
                        guint property_id,
                        GValue *value,
                        GParamSpec *pspec)
{
    KaApplet *self = KA_APPLET (object);

    switch (property_id) {
    case KA_PROP_PRINCIPAL:
        g_value_set_string (value, self->priv->principal);
        break;

    case KA_PROP_PK_USERID:
        g_value_set_string (value, self->priv->pk_userid);
        break;

    case KA_PROP_PK_ANCHORS:
        g_value_set_string (value, self->priv->pk_anchors);
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

    case KA_PROP_CONF_TICKETS:
        g_value_set_boolean (value, self->priv->conf_tickets);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
ka_applet_dispose (GObject *object)
{
    KaApplet *applet = KA_APPLET (object);
    GObjectClass *parent_class = G_OBJECT_CLASS (ka_applet_parent_class);

    ka_close_notification (applet);

    if (applet->priv->tray_icon) {
        g_object_unref (applet->priv->tray_icon);
        applet->priv->tray_icon = NULL;
    }
    if (applet->priv->pwdialog) {
        gtk_widget_destroy (GTK_WIDGET(applet->priv->pwdialog));
        applet->priv->pwdialog = NULL;
    }
    if (applet->priv->loader) {
        g_object_unref (applet->priv->loader);
        applet->priv->loader = NULL;
    }

    if (parent_class->dispose != NULL)
        parent_class->dispose (object);
}


static void
ka_applet_finalize (GObject *object)
{
    KaApplet *applet = KA_APPLET (object);
    GObjectClass *parent_class = G_OBJECT_CLASS (ka_applet_parent_class);

    g_free (applet->priv->principal);
    g_free (applet->priv->pk_userid);
    g_free (applet->priv->pk_anchors);
    g_free (applet->priv->krb_msg);
    /* no need to free applet->priv */

    if (parent_class->finalize != NULL)
        parent_class->finalize (object);
}

static void
ka_applet_init (KaApplet *applet)
{
    applet->priv = G_TYPE_INSTANCE_GET_PRIVATE (applet,
                                                KA_TYPE_APPLET,
                                                KaAppletPrivate);
}

static void
ka_applet_class_init (KaAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;
    int i;

    object_class->dispose = ka_applet_dispose;
    object_class->finalize = ka_applet_finalize;
    g_type_class_add_private (klass, sizeof (KaAppletPrivate));

    G_APPLICATION_CLASS (klass)->local_command_line =   \
        ka_applet_local_command_line;
    G_APPLICATION_CLASS (klass)->command_line = ka_applet_command_line;
    G_APPLICATION_CLASS (klass)->startup = ka_applet_startup;

    object_class->set_property = ka_applet_set_property;
    object_class->get_property = ka_applet_get_property;

    pspec = g_param_spec_string (KA_PROP_NAME_PRINCIPAL,
                                 "Principal",
                                 "Get/Set Kerberos principal",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class, KA_PROP_PRINCIPAL, pspec);

    pspec = g_param_spec_string (KA_PROP_NAME_PK_USERID,
                                 "PKinit identifier",
                                 "Get/Set Pkinit identifier",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class, KA_PROP_PK_USERID, pspec);

    pspec = g_param_spec_string (KA_PROP_NAME_PK_ANCHORS,
                                 "PKinit trust anchors",
                                 "Get/Set Pkinit trust anchors",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class, KA_PROP_PK_ANCHORS, pspec);

    pspec = g_param_spec_uint (KA_PROP_NAME_PW_PROMPT_MINS,
                               "Password prompting interval",
                               "Password prompting interval in minutes",
                               0, G_MAXUINT, MINUTES_BEFORE_PROMPTING,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class,
                                     KA_PROP_PW_PROMPT_MINS, pspec);

    pspec = g_param_spec_boolean (KA_PROP_NAME_TGT_FORWARDABLE,
                                  "Forwardable ticket",
                                  "wether to request forwardable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class,
                                     KA_PROP_TGT_FORWARDABLE, pspec);

    pspec = g_param_spec_boolean (KA_PROP_NAME_TGT_PROXIABLE,
                                  "Proxiable ticket",
                                  "wether to request proxiable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class,
                                     KA_PROP_TGT_PROXIABLE, pspec);

    pspec = g_param_spec_boolean (KA_PROP_NAME_TGT_RENEWABLE,
                                  "Renewable ticket",
                                  "wether to request renewable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class,
                                     KA_PROP_TGT_RENEWABLE, pspec);

    pspec = g_param_spec_boolean (KA_PROP_NAME_CONF_TICKETS,
                                  "Configuration tickets",
                                  "wether to show configuration tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
    g_object_class_install_property (object_class,
                                     KA_PROP_CONF_TICKETS, pspec);

    for (i=0; i < KA_SIGNAL_COUNT-1; i++) {
        guint signalId;

        signalId = g_signal_new (ka_signal_names[i], G_OBJECT_CLASS_TYPE (klass),
                                 G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                 ka_closure_VOID__STRING_UINT,
                                 G_TYPE_NONE, 2,   /* number of parameters */
                                 G_TYPE_STRING, G_TYPE_UINT);
        klass->signals[i] = signalId;
    }
    klass->signals[KA_CCACHE_CHANGED] = g_signal_new (
        ka_signal_names[KA_CCACHE_CHANGED],
        G_OBJECT_CLASS_TYPE (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}


static KaApplet *
ka_applet_new (void)
{
    return g_object_new (KA_TYPE_APPLET,
                         "application-id", "org.gnome.KrbAuthDialog",
                         NULL);
}


/* determine the new tooltip text */
static char *
ka_applet_tooltip_text (int remaining)
{
    int hours, minutes;
    gchar *tooltip_text;

    if (remaining > 0) {
        if (remaining >= 3600) {
            hours = remaining / 3600;
            minutes = (remaining % 3600) / 60;
            /* Translators: First number is hours, second number is minutes */
            tooltip_text =
                g_strdup_printf (_("Your credentials expire in %.2d:%.2dh"),
                                 hours, minutes);
        } else {
            minutes = remaining / 60;
            tooltip_text =
                g_strdup_printf (ngettext
                                 ("Your credentials expire in %d minute",
                                  "Your credentials expire in %d minutes",
                                  minutes), minutes);
        }
    } else
        tooltip_text = g_strdup (_("Your credentials have expired"));
    return tooltip_text;
}


/* determine the current icon */
static const char *
ka_applet_select_icon (KaApplet *applet, int remaining)
{
    enum ka_icon status_icon = inv_icon;

    if (remaining > 0) {
        if (remaining < applet->priv->pw_prompt_secs &&
            !applet->priv->renewable)
            status_icon = exp_icon;
        else
            status_icon = val_icon;
    }

    return applet->priv->icons[status_icon];
}


static gboolean
ka_tray_icon_is_embedded (KaApplet *self)
{
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    if (self->priv->tray_icon
        && gtk_status_icon_is_embedded (self->priv->tray_icon))
        return TRUE;
    else
        return FALSE;
G_GNUC_END_IGNORE_DEPRECATIONS
}


static gboolean
ka_show_notification (KaApplet *applet)
{
    /* wait for the panel to be settled before showing a bubble */
    if (applet->priv->ns_persistence
        || ka_tray_icon_is_embedded (applet)) {
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
        g_timeout_add_seconds (5, (GSourceFunc) ka_show_notification, applet);
    }
    return FALSE;
}


/* Callback to handle disabling of notification */
static void
ka_notify_disable_action_cb (NotifyNotification *notification G_GNUC_UNUSED,
                              gchar *action,
                              gpointer user_data)
{
    KaApplet *self = KA_APPLET (user_data);
    GSettings *ns = g_settings_get_child (self->priv->settings,
                                              KA_SETTING_CHILD_NOTIFY);

    if (strcmp (action, "dont-show-again") == 0) {
        KA_DEBUG ("turning of notification %s", self->priv->notify_key);
        if (!g_settings_set_boolean (ns,
                                     self->priv->notify_key, FALSE)) {
            g_warning("Failed to set %s", self->priv->notify_key);
        }
        self->priv->notify_key = NULL;
    } else {
        g_warning ("unkonwn action for callback");
    }
    g_object_unref (ns);
}


/* Callback to handle ticket related actions */
static void
ka_notify_ticket_action_cb (NotifyNotification *notification G_GNUC_UNUSED,
                            gchar *action,
                            gpointer user_data)
{
    KaApplet *self = KA_APPLET (user_data);

    g_return_if_fail (self != NULL);

    if (strcmp (action, "ka-acquire-tgt") == 0) {
        KA_DEBUG ("Getting new tgt");
        ka_grab_credentials (self);
    } else if (strcmp (action, "ka-remove-ccache") == 0) {
        KA_DEBUG ("Removing ccache");
        ka_destroy_ccache (self);
    } else if (strcmp (action, "ka-list-tickets") == 0) {
        KA_DEBUG ("Showing main window");
        ka_main_window_show (self);
    } else {
        g_warning ("unkonwn action for callback");
    }
}


static void
ka_close_notification (KaApplet *self)
{
    GError *error = NULL;

    if (self->priv->notification != NULL) {
        if (!notify_notification_close (self->priv->notification, &error)) {
            if (error)
                g_warning ("Cannot close notification %s", error->message);
            else
                g_warning ("Cannot close notification");
        }
        g_object_unref (self->priv->notification);
        g_clear_error (&error);
        self->priv->notification = NULL;
    }
}

static void
ka_send_event_notification (KaApplet *self,
                            const char *summary,
                            const char *message,
                            const char *icon,
                            gboolean get_ticket_action)
{
    NotifyNotification *notification;
    const char *hint;
    gint timeout;

    g_return_if_fail (self != NULL);
    g_return_if_fail (summary != NULL);
    g_return_if_fail (message != NULL);

    if (!notify_is_initted ())
        notify_init (KA_NAME);

    if (self->priv->notification) {
        notification = self->priv->notification;
        notify_notification_update (notification, summary, message, icon);
    } else {
        notification = self->priv->notification =
            notify_notification_new (summary, message, icon);
        notify_notification_set_urgency (notification, NOTIFY_URGENCY_NORMAL);
    }

    notify_notification_set_hint (notification,
                                  "desktop-entry",
                                  g_variant_new_string (PACKAGE));
    if (self->priv->ns_persistence) {
        hint = "resident";
        timeout = NOTIFY_EXPIRES_NEVER;

        notify_notification_set_timeout (notification, timeout);
        notify_notification_clear_hints (notification);
        notify_notification_set_hint (notification,
                                      hint,
                                      g_variant_new_boolean (TRUE));
    }

    notify_notification_clear_actions(notification);

    if (self->priv->ns_persistence) {
        notify_notification_add_action (notification,
                                        "ka-list-tickets",
                                        _("List Tickets"),
                                        (NotifyActionCallback)
                                        ka_notify_ticket_action_cb,
                                        self,
                                        NULL);
    }

    if (get_ticket_action) {
        notify_notification_add_action (notification,
                                        "ka-acquire-tgt",
                                        _("Get Ticket"),
                                        (NotifyActionCallback)
                                        ka_notify_ticket_action_cb,
                                        self,
                                        NULL);
    } else {
        if (!self->priv->ns_persistence) {
            notify_notification_add_action (notification,
                                            "dont-show-again",
                                            _("Don't show me this again"),
                                            (NotifyActionCallback)
                                            ka_notify_disable_action_cb, self,
                                            NULL);
        } else {
            notify_notification_add_action (notification,
                                        "ka-remove-ccache",
                                        _("Remove Credentials Cache"),
                                        (NotifyActionCallback)
                                        ka_notify_ticket_action_cb,
                                        self,
                                        NULL);
        }
    }
    ka_show_notification (self);
}


static void
ka_update_tray_icon (KaApplet *self, const char *icon, const char *tooltip)
{
    if (self->priv->tray_icon) {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_status_icon_set_from_icon_name (self->priv->tray_icon, icon);
        gtk_status_icon_set_tooltip_text (self->priv->tray_icon, tooltip);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
}

/* check whether a given notification is enabled */
static gboolean
get_notify_enabled (KaApplet *self, const char *key)
{
    gboolean ret;
    GSettings *ns = g_settings_get_child (self->priv->settings,
                                          KA_SETTING_CHILD_NOTIFY);
    ret = g_settings_get_boolean (ns, key);
    g_object_unref (ns);
    return ret;
}

/*
 * update the tray icon's tooltip and icon
 * and notify listeners about acquired/expiring tickets via signals
 */
int
ka_applet_update_status (KaApplet *applet, krb5_timestamp expiry)
{
    int now = time (0);
    int remaining = expiry - now;
    static int last_warn = 0;
    static gboolean expiry_notified = FALSE;
    static gboolean initial_notification = TRUE;
    static krb5_timestamp old_expiry = 0;
    gboolean notify = TRUE;
    const char *status_icon = ka_applet_select_icon (applet, remaining);
    char *tooltip_text = ka_applet_tooltip_text (remaining);

    if (remaining > 0) {
        if (expiry_notified || initial_notification) {
            const char* msg;
            notify = get_notify_enabled (applet, KA_SETTING_KEY_NOTIFY_VALID);
            if (notify) {
                applet->priv->notify_key = KA_SETTING_KEY_NOTIFY_VALID;

                if (applet->priv->krb_msg)
                    msg = applet->priv->krb_msg;
                else {
                    if (initial_notification)
                        msg = _("You have valid Kerberos credentials.");
                    else
                        msg = _("You've refreshed your Kerberos credentials.");
                }
                ka_send_event_notification (applet,
                                            _("Network credentials valid"),
                                            msg,
                                            "krb-valid-ticket",
                                            FALSE);
            }
            ka_applet_signal_emit (applet, KA_SIGNAL_ACQUIRED_TGT, expiry);
            expiry_notified = FALSE;
            g_free (applet->priv->krb_msg);
            applet->priv->krb_msg = NULL;
        } else {
            if (remaining < applet->priv->pw_prompt_secs
                && (now - last_warn) > NOTIFY_SECONDS
                && !applet->priv->renewable) {
                notify = get_notify_enabled (applet,
                                             KA_SETTING_KEY_NOTIFY_EXPIRING);
                if (notify) {
                    applet->priv->notify_key =
                        KA_SETTING_KEY_NOTIFY_EXPIRING;
                    ka_send_event_notification (applet,
                                                _("Network credentials expiring"),
                                                tooltip_text,
                                                "krb-expiring-ticket",
                                                TRUE);
                }
                last_warn = now;
            }
            /* ticket lifetime got longer e.g. by kinit -R */
            if (old_expiry && expiry > old_expiry)
                ka_applet_signal_emit (applet, KA_SIGNAL_RENEWED_TGT, expiry);
        }
    } else {
        if (!expiry_notified) {
            notify = get_notify_enabled (applet, KA_SETTING_KEY_NOTIFY_EXPIRED);
            if (notify) {
                applet->priv->notify_key = KA_SETTING_KEY_NOTIFY_EXPIRED;
                ka_send_event_notification (applet,
                                            _("Network credentials expired"),
                                            _("Your Kerberos credentials have expired."),
                                            "krb-no-valid-ticket",
                                            TRUE);
            }
            ka_applet_signal_emit (applet, KA_SIGNAL_EXPIRED_TGT, expiry);
            expiry_notified = TRUE;
            last_warn = 0;
        }
    }

    old_expiry = expiry;
    ka_update_tray_icon(applet, status_icon, tooltip_text);
    g_free (tooltip_text);
    initial_notification = FALSE;
    return 0;
}


static GActionEntry trayicon_entries[] = {
    { "remove_credentials_cache", action_remove_credentails_cache, NULL, NULL, NULL, {0} },
    { "list_tickets", action_list_tickets, NULL, NULL, NULL, {0} },
    { "preferences", action_preferences, NULL, NULL, NULL, {0} },
    { "about", action_about, NULL, NULL, NULL, {0} },
    { "help", action_help, NULL, NULL, NULL, {0} },
    { "quit", action_quit, NULL, NULL, NULL, {0} },
};

/* The tray icon's context menu */
static gboolean
ka_applet_create_tray_icon_context_menu (KaApplet *self)
{
    GtkBuilder *builder;
    GMenuModel *model;
    GSimpleActionGroup *group;

    builder = gtk_builder_new_from_resource (
        "/org/gnome/krb5-auth-dialog/ui/tray-icon-menu.ui");
    model = G_MENU_MODEL (
        gtk_builder_get_object (builder, "tray-icon-context-menu"));

    self->priv->context_menu = gtk_menu_new_from_model (model);
    gtk_widget_show_all (self->priv->context_menu);

    group = g_simple_action_group_new ();
    g_action_map_add_action_entries (G_ACTION_MAP (group),
                                     trayicon_entries, G_N_ELEMENTS (trayicon_entries),
                                     self);

    gtk_widget_insert_action_group (GTK_WIDGET(self->priv->context_menu),
                                    "trayicon",
                                    G_ACTION_GROUP(group));

    g_object_unref (builder);
    return TRUE;
}


static void
ka_tray_icon_on_menu (GtkStatusIcon *status_icon G_GNUC_UNUSED,
                      guint button,
                      guint activate_time,
                      gpointer user_data)
{
    KaApplet *applet = KA_APPLET (user_data);

    KA_DEBUG ("Trayicon right clicked: %d", applet->priv->pw_prompt_secs);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gtk_menu_popup (GTK_MENU (applet->priv->context_menu), NULL, NULL,
                    gtk_status_icon_position_menu, applet->priv->tray_icon,
                    button, activate_time);
G_GNUC_END_IGNORE_DEPRECATIONS
}


static gboolean
ka_tray_icon_on_click (GtkStatusIcon *status_icon G_GNUC_UNUSED,
                       gpointer data)
{
    KaApplet *applet = KA_APPLET (data);

    KA_DEBUG ("Trayicon clicked: %d", applet->priv->pw_prompt_secs);
    ka_grab_credentials (applet);
    return TRUE;
}


static gboolean
ka_applet_create_tray_icon (KaApplet *self)
{
    GtkStatusIcon *tray_icon;

    if (self->priv->ns_persistence)
        return FALSE;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    tray_icon = self->priv->tray_icon = gtk_status_icon_new ();
    gtk_status_icon_set_from_icon_name (tray_icon,
                                        self->priv->icons[exp_icon]);
    gtk_status_icon_set_tooltip_text (tray_icon, PACKAGE);
    gtk_status_icon_set_title (tray_icon, KA_NAME);
G_GNUC_END_IGNORE_DEPRECATIONS

    g_signal_connect (G_OBJECT (tray_icon), "activate",
                      G_CALLBACK (ka_tray_icon_on_click), self);
    g_signal_connect (G_OBJECT (tray_icon),
                      "popup-menu",
                      G_CALLBACK (ka_tray_icon_on_menu), self);
    ka_applet_create_tray_icon_context_menu (self);
    return TRUE;
}

static int
ka_applet_setup_icons (KaApplet *applet)
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
ka_applet_get_pw_prompt_secs (const KaApplet *applet)
{
    return applet->priv->pw_prompt_secs;
}

void
ka_applet_set_tgt_renewable (KaApplet *applet, gboolean renewable)
{
    applet->priv->renewable = renewable;
}

gboolean
ka_applet_get_tgt_renewable (const KaApplet *applet)
{
    return applet->priv->renewable;
}

KaPwDialog *
ka_applet_get_pwdialog (const KaApplet *applet)
{
    return applet->priv->pwdialog;
}

GSettings *
ka_applet_get_settings (const KaApplet *self)
{
    return self->priv->settings;
}

void
ka_applet_set_msg (KaApplet *self, const char *msg)
{
    g_free (self->priv->krb_msg);
    self->priv->krb_msg = g_strdup (msg);
}

void
ka_applet_signal_emit (KaApplet *this,
                       KaAppletSignalNumber signum,
                       krb5_timestamp expiry)
{
    KaAppletClass *klass = KA_APPLET_GET_CLASS (this);
    char *princ;

    princ = ka_unparse_name ();
    if (!princ)
        return;

    g_signal_emit (this, klass->signals[signum], 0, princ, (guint32) expiry);
    g_free (princ);
}


static void
ka_ns_check_persistence (KaApplet *self)
{
    GList   *caps;
    GList   *l;
    gboolean is_autostart = g_getenv("DESKTOP_AUTOSTART_ID") ? TRUE : FALSE;
    gint    seconds = 5;

    self->priv->ns_persistence = FALSE;

    if (self->priv->debug_flags & KA_DEBUG_NO_PERSISTENCE)
        return;

    do {
        caps = notify_get_server_caps ();
        if (caps == NULL)
            g_warning ("Failed to read server caps");
        else {
            l = g_list_find_custom (caps, "persistence", (GCompareFunc)strcmp);
            if (l != NULL) {
                self->priv->ns_persistence = TRUE;
                KA_DEBUG ("Notification server supports persistence.");
            }
            g_list_foreach (caps, (GFunc) g_free, NULL);
            g_list_free (caps);
        }
        /* During session start we have to wait until the shell is fully up
         * to reliably detect the persistence property (#642666) */
        if (is_autostart && !self->priv->ns_persistence) {
            sleep(1);
            seconds--;
        } else
            break;
    } while (seconds);
}


/* undo what was done on startup() */
void
ka_applet_destroy (KaApplet* self)
{
    GList *windows, *first;

    ka_dbus_disconnect ();
    windows = gtk_application_get_windows (GTK_APPLICATION(self));
    if (windows) {
        first = g_list_first (windows);
        gtk_application_remove_window(GTK_APPLICATION (self),
                                      GTK_WINDOW (first->data));
    }

    gtk_widget_destroy (GTK_WIDGET(self->priv->prefs));
    self->priv->prefs = NULL;

    gtk_widget_destroy (GTK_WIDGET(self->priv->context_menu));
    self->priv->context_menu = NULL;

    ka_kerberos_destroy ();
}


/* create the tray icon applet */
KaApplet *
ka_applet_create ()
{
    KaApplet *applet = ka_applet_new ();

    ka_applet_handle_debug(applet);

    if (!(ka_applet_setup_icons (applet)))
        g_error ("Failure to setup icons");
    gtk_window_set_default_icon_name (applet->priv->icons[val_icon]);

    applet->priv->pwdialog = ka_pwdialog_new ();
    g_return_val_if_fail (applet->priv->pwdialog != NULL, NULL);

    applet->priv->settings = ka_settings_init (applet);
    g_return_val_if_fail (applet->priv->settings != NULL, NULL);

    applet->priv->loader = ka_plugin_loader_create (applet);
    g_return_val_if_fail (applet->priv->loader != NULL, NULL);

    g_return_val_if_fail (ka_dbus_connect (applet), NULL);

    ka_ns_check_persistence(applet);
    ka_applet_create_tray_icon (applet);

    return applet;
}

int
main (int argc, char *argv[])
{
    KaApplet *applet;
    int ret = 0;

    textdomain (PACKAGE);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    bindtextdomain (PACKAGE, LOCALE_DIR);

    g_set_application_name (KA_NAME);

    gtk_init (&argc, &argv);
    applet = ka_applet_create ();
    if (!applet)
        return 1;

    setup_signal_handlers(applet);
    ret = g_application_run (G_APPLICATION(applet), argc, argv);
    g_object_unref (applet);
    return ret;
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
