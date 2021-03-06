/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008,2009,2010,2013,2021 Guido Guenther <agx@sigxcpu.org>
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

struct _KaAppletPrivate {
    const char *icons[3];       /* for invalid, expiring and valid tickts */

    KaPwDialog *pwdialog;       /* the password dialog */
    KaPreferences *prefs;       /* the prefs dialog */
    int pw_prompt_secs;         /* when to start sending notifications */
    KaPluginLoader *loader;     /* Plugin loader */

    /* command line and env handling */
    gboolean startup_ccache;    /* ccache found on startup */
    gboolean auto_run;          /* only start with valid ccache */
    gint debug_flags;           /* Debug options from environment */

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

G_DEFINE_TYPE_WITH_PRIVATE (KaApplet, ka_applet, GTK_TYPE_APPLICATION);

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
    flags = g_application_get_flags(application);
    g_application_set_flags(application, flags | G_APPLICATION_HANDLES_COMMAND_LINE);

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

    ka_show_help (window, NULL);
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
ka_remove_ccache_action (GSimpleAction *action G_GNUC_UNUSED,
                         GVariant *parameter G_GNUC_UNUSED,
                         gpointer userdata)
{
    KaApplet *self = KA_APPLET (userdata);
    KA_DEBUG ("Removing ccache");
    ka_destroy_ccache (self);
}


static void
ka_acquire_tgt_action (GSimpleAction *action G_GNUC_UNUSED,
                       GVariant *parameter G_GNUC_UNUSED,
                       gpointer userdata)
{
    KaApplet *self = KA_APPLET (userdata);
    KA_DEBUG ("Getting new tgt");
    ka_grab_credentials (self);
}


static GActionEntry app_entries[] = {
    { "preferences", action_preferences, NULL, NULL, NULL, {0} },
    { "about", action_about, NULL, NULL, NULL, {0} },
    { "help", action_help, NULL, NULL, NULL, {0} },
    { "quit", action_quit, NULL, NULL, NULL, {0} },
    { "acquire-ticket", ka_acquire_tgt_action , NULL, NULL, NULL, {0} },
    { "remove-ccache", ka_remove_ccache_action , NULL, NULL, NULL, {0} },
};


static void
ka_applet_startup (GApplication *application)
{
    KaApplet *self = KA_APPLET (application);
    GtkWindow *main_window;

    KA_DEBUG ("Primary application");

    G_APPLICATION_CLASS (ka_applet_parent_class)->startup (application);

    self->priv->startup_ccache = ka_kerberos_init (self);
    main_window = GTK_WINDOW(ka_main_window_create (self));
    gtk_window_set_transient_for(GTK_WINDOW(self->priv->pwdialog),
                                 main_window);

    self->priv->prefs = ka_preferences_new (self);

    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     app_entries, G_N_ELEMENTS (app_entries),
                                     self);
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

    if (applet->priv->pwdialog) {
        gtk_widget_destroy (GTK_WIDGET(applet->priv->pwdialog));
        applet->priv->pwdialog = NULL;
    }
    if (applet->priv->loader) {
        g_object_unref (applet->priv->loader);
        applet->priv->loader = NULL;
    }

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

    parent_class->finalize (object);
}

static void
ka_applet_init (KaApplet *applet)
{
    applet->priv = ka_applet_get_instance_private (applet);
}

static void
ka_applet_class_init (KaAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;
    int i;

    object_class->dispose = ka_applet_dispose;
    object_class->finalize = ka_applet_finalize;

    G_APPLICATION_CLASS (klass)->local_command_line =   \
        ka_applet_local_command_line;
    G_APPLICATION_CLASS (klass)->command_line = ka_applet_command_line;
    G_APPLICATION_CLASS (klass)->startup = ka_applet_startup;
    G_APPLICATION_CLASS (klass)->activate = ka_applet_activate;

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
                         "application-id", KA_APP_ID,
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
            tooltip_text =
                /* Translators: First number is hours, second number is minutes */
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


static void
ka_send_event_notification (KaApplet *self,
                            const char *summary,
                            const char *message,
                            const char *iconname,
                            gboolean get_ticket_action)
{
    g_autoptr (GNotification) notification = NULL;
    /* TODO: ka_appliet_select_icon ? */
    g_autoptr (GIcon) icon = g_icon_new_for_string (iconname, NULL);

    g_return_if_fail (self != NULL);
    g_return_if_fail (summary != NULL);
    g_return_if_fail (message != NULL);

    notification = g_notification_new (summary);
    g_notification_set_body (notification, message);
    g_notification_set_icon (notification, icon);

    if (get_ticket_action)
        g_notification_add_button (notification,
                                   _("Get Ticket"),
                                   "app.acquire-ticket");
    else
        g_notification_add_button (notification,
                                   _("Remove Credentials Cache"),
                                   "app.remove-ccache");

    g_application_send_notification (G_APPLICATION (self),
                                     PACKAGE,
                                     notification);
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
    g_free (tooltip_text);
    initial_notification = FALSE;
    return 0;
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

    ka_kerberos_destroy ();
}


KaApplet *
ka_applet_create (void)
{
    KaApplet *applet = ka_applet_new ();

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

    g_set_prgname (KA_APP_ID);
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
