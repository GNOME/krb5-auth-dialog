/*
 * (C) 2008,2009,2010,2013,2021 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>
#include <glib-unix.h>

#include "ka-applet-priv.h"
#include "ka-dbus.h"
#include "ka-kerberos.h"
#include "ka-settings.h"
#include "ka-tools.h"
#include "ka-main-window.h"
#include "ka-marshallers.h"
#include "ka-plugin-loader.h"
#include "ka-preferences.h"

#include <signal.h>

#define NOTIFY_SECONDS 300

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
    KA_PROP_LAST_PROP
};
static GParamSpec *props[KA_PROP_LAST_PROP];

enum _KaAppletSignalNumber {
    KA_SIGNAL_ACQUIRED_TGT,     /* New TGT acquired */
    KA_SIGNAL_RENEWED_TGT,      /* TGT got renewed */
    KA_SIGNAL_EXPIRED_TGT,      /* TGT expired or ticket cache got destroyed */
    KA_CCACHE_CHANGED,          /* The credential cache changed */
    KA_SIGNAL_COUNT
};
static guint signals[KA_SIGNAL_COUNT];

const gchar *ka_signal_names[] = {
    "krb-tgt-acquired",
    "krb-tgt-renewed",
    "krb-tgt-expired",
    "krb-ccache-changed",
};

struct _KaApplet {
    AdwApplication parent;

    KaPwDialog *pwdialog;       /* the password dialog */
    KaPreferences *prefs;       /* the prefs dialog */
    KaMainWindow *main_window;  /* The main application window */
    int pw_prompt_secs;         /* when to start sending notifications */
    KaPluginLoader *loader;     /* Plugin loader */

    /* command line and env handling */
    gboolean startup_ccache;    /* ccache found on startup */
    gboolean auto_run;          /* only start with valid ccache */

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

G_DEFINE_TYPE (KaApplet, ka_applet, ADW_TYPE_APPLICATION);

static gboolean is_initialized;

static void
ka_applet_signal_emit (KaApplet *self, guint signum, krb5_timestamp expiry)
{
    g_autofree char *princ = NULL;

    princ = ka_unparse_name ();
    if (!princ)
        return;

    g_signal_emit (self, signals[signum], 0, princ, (guint32) expiry);
}


static void
ka_applet_activate (GApplication *application)
{
    KaApplet *self = KA_APPLET(application);

    if (is_initialized) {
        gboolean show_conf_tickets;

        KA_DEBUG ("Main window activated");
        g_object_get(self, KA_PROP_NAME_CONF_TICKETS, &show_conf_tickets, NULL);
        ka_main_window_show (self->main_window, show_conf_tickets);
    } else
        is_initialized = TRUE;
}


static int
ka_applet_command_line (GApplication            *application,
                        GApplicationCommandLine *cmdline G_GNUC_UNUSED)
{
    KaApplet *self = KA_APPLET(application);
    KA_DEBUG ("Evaluating command line");

    if (!self->startup_ccache && self->auto_run) {
        g_application_quit (G_APPLICATION (self));
    } else
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
    g_option_context_parse (context, &argc, argv, &error);

    if (error) {
        g_print ("%s\n%s\n", error->message, help_msg);
        g_clear_error (&error);
        *exit_status = 1;
    } else {
        self->auto_run = auto_run;
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
    ka_preferences_run (self->prefs);
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

    g_application_quit (G_APPLICATION (self));
}


static gboolean
on_shutdown_signal (gpointer data)
{
    KaApplet *self = KA_APPLET (data);

    g_message ("Caught shutdown signal");
    g_application_quit (G_APPLICATION (self));

    return G_SOURCE_REMOVE;
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

    KA_DEBUG ("Primary application");

    G_APPLICATION_CLASS (ka_applet_parent_class)->startup (application);

    self->startup_ccache = ka_kerberos_init (self);
    self->main_window = ka_main_window_new (self);
    gtk_window_set_transient_for(GTK_WINDOW(self->pwdialog), GTK_WINDOW (self->main_window));

    self->prefs = ka_preferences_new (self);

    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     app_entries, G_N_ELEMENTS (app_entries),
                                     self);
}

static gboolean
ka_applet_dbus_register (GApplication    *application,
                         GDBusConnection *connection,
                         const gchar     *object_path,
                         GError         **error)
{
  KaApplet *self = KA_APPLET (application);

  G_APPLICATION_CLASS (ka_applet_parent_class)->dbus_register (application,
                                                               connection,
                                                               object_path,
                                                               error);

  ka_dbus_connect (self, connection, object_path);
  return TRUE;
}


static void
ka_applet_dbus_unregister (GApplication    *application,
                           GDBusConnection *connection,
                           const gchar     *object_path)
{
  ka_dbus_disconnect ();

  G_APPLICATION_CLASS (ka_applet_parent_class)->dbus_unregister (application,
                                                                 connection,
                                                                 object_path);
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
        g_free (self->principal);
        self->principal = g_value_dup_string (value);
        KA_DEBUG ("%s: %s", pspec->name, self->principal);
        break;

    case KA_PROP_PK_USERID:
        g_free (self->pk_userid);
        self->pk_userid = g_value_dup_string (value);
        KA_DEBUG ("%s: %s", pspec->name, self->pk_userid);
        break;

    case KA_PROP_PK_ANCHORS:
        g_free (self->pk_anchors);
        self->pk_anchors = g_value_dup_string (value);
        KA_DEBUG ("%s: %s", pspec->name, self->pk_anchors);
        break;

    case KA_PROP_PW_PROMPT_MINS:
        self->pw_prompt_secs = g_value_get_uint (value) * 60;
        KA_DEBUG ("%s: %d", pspec->name, self->pw_prompt_secs / 60);
        break;

    case KA_PROP_TGT_FORWARDABLE:
        self->tgt_forwardable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->tgt_forwardable ? "True" : "False");
        break;

    case KA_PROP_TGT_PROXIABLE:
        self->tgt_proxiable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->tgt_proxiable ? "True" : "False");
        break;

    case KA_PROP_TGT_RENEWABLE:
        self->tgt_renewable = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->tgt_renewable ? "True" : "False");
        break;

    case KA_PROP_CONF_TICKETS:
        self->conf_tickets = g_value_get_boolean (value);
        KA_DEBUG ("%s: %s", pspec->name,
                  self->tgt_renewable ? "True" : "False");
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
        g_value_set_string (value, ka_applet_get_principal (self));
        break;

    case KA_PROP_PK_USERID:
        g_value_set_string (value, self->pk_userid);
        break;

    case KA_PROP_PK_ANCHORS:
        g_value_set_string (value, self->pk_anchors);
        break;

    case KA_PROP_PW_PROMPT_MINS:
        g_value_set_uint (value, self->pw_prompt_secs / 60);
        break;

    case KA_PROP_TGT_FORWARDABLE:
        g_value_set_boolean (value, self->tgt_forwardable);
        break;

    case KA_PROP_TGT_PROXIABLE:
        g_value_set_boolean (value, self->tgt_proxiable);
        break;

    case KA_PROP_TGT_RENEWABLE:
        g_value_set_boolean (value, self->tgt_renewable);
        break;

    case KA_PROP_CONF_TICKETS:
        g_value_set_boolean (value, self->conf_tickets);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ka_applet_constructed (GObject *object)
{
    KaApplet *self = KA_APPLET (object);

    G_OBJECT_CLASS (ka_applet_parent_class)->constructed (object);

    self->pwdialog = ka_pwdialog_new ();
    self->settings = ka_settings_init (self);
    self->loader = ka_plugin_loader_create (self);

    g_unix_signal_add (SIGTERM, on_shutdown_signal, self);
    g_unix_signal_add (SIGINT, on_shutdown_signal, self);
}


static void
ka_applet_dispose (GObject *object)
{
    KaApplet *self = KA_APPLET (object);

    g_clear_pointer (&self->pwdialog, ka_window_destroy);
    g_clear_pointer (&self->prefs, ka_window_destroy);
    g_clear_object (&self->loader);

    ka_kerberos_destroy ();

    G_OBJECT_CLASS (ka_applet_parent_class)->dispose (object);
}


static void
ka_applet_finalize (GObject *object)
{
    KaApplet *self = KA_APPLET (object);
    GObjectClass *parent_class = G_OBJECT_CLASS (ka_applet_parent_class);

    g_free (self->principal);
    g_free (self->pk_userid);
    g_free (self->pk_anchors);
    g_free (self->krb_msg);
    /* no need to free self */

    parent_class->finalize (object);
}

static void
ka_applet_init (KaApplet *applet)
{
    gtk_window_set_default_icon_name ("krb-valid-ticket");
}

static void
ka_applet_class_init (KaAppletClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GApplicationClass *application_class = G_APPLICATION_CLASS (klass);
    int i;

    object_class->constructed = ka_applet_constructed;;
    object_class->dispose = ka_applet_dispose;
    object_class->finalize = ka_applet_finalize;

    application_class->local_command_line = ka_applet_local_command_line;
    application_class->command_line = ka_applet_command_line;
    application_class->startup = ka_applet_startup;
    application_class->activate = ka_applet_activate;
    application_class->dbus_register = ka_applet_dbus_register;
    application_class->dbus_unregister = ka_applet_dbus_unregister;

    object_class->set_property = ka_applet_set_property;
    object_class->get_property = ka_applet_get_property;

    props[KA_PROP_PRINCIPAL] =
        g_param_spec_string (KA_PROP_NAME_PRINCIPAL,
                                 "Principal",
                                 "Get/Set Kerberos principal",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_PK_USERID] =
        g_param_spec_string (KA_PROP_NAME_PK_USERID,
                                 "PKinit identifier",
                                 "Get/Set Pkinit identifier",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_PK_ANCHORS] =
        g_param_spec_string (KA_PROP_NAME_PK_ANCHORS,
                                 "PKinit trust anchors",
                                 "Get/Set Pkinit trust anchors",
                                 "", G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_PW_PROMPT_MINS] =
        g_param_spec_uint (KA_PROP_NAME_PW_PROMPT_MINS,
                               "Password prompting interval",
                               "Password prompting interval in minutes",
                               0, G_MAXUINT, MINUTES_BEFORE_PROMPTING,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_TGT_FORWARDABLE] =
        g_param_spec_boolean (KA_PROP_NAME_TGT_FORWARDABLE,
                                  "Forwardable ticket",
                                  "whether to request forwardable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_TGT_PROXIABLE] =
        g_param_spec_boolean (KA_PROP_NAME_TGT_PROXIABLE,
                                  "Proxiable ticket",
                                  "whether to request proxiable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_TGT_RENEWABLE] =
        g_param_spec_boolean (KA_PROP_NAME_TGT_RENEWABLE,
                                  "Renewable ticket",
                                  "whether to request renewable tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    props[KA_PROP_CONF_TICKETS] =
        g_param_spec_boolean (KA_PROP_NAME_CONF_TICKETS,
                                  "Configuration tickets",
                                  "whether to show configuration tickets",
                                  FALSE,
                                  G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    g_object_class_install_properties (object_class, KA_PROP_LAST_PROP, props);

    for (i=0; i < KA_SIGNAL_COUNT-1; i++) {
        signals[i] = g_signal_new (ka_signal_names[i], G_OBJECT_CLASS_TYPE (klass),
                                   G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                   _ka_marshal_VOID__STRING_UINT,
                                   G_TYPE_NONE, 2,   /* number of parameters */
                                   G_TYPE_STRING, G_TYPE_UINT);
        g_signal_set_va_marshaller (signals[i],
                                    G_TYPE_FROM_CLASS (klass),
                                    _ka_marshal_VOID__STRING_UINTv);

    }
    signals[KA_CCACHE_CHANGED] = g_signal_new (
        ka_signal_names[KA_CCACHE_CHANGED],
        G_OBJECT_CLASS_TYPE (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}


KaApplet *
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
    GSettings *ns = g_settings_get_child (self->settings,
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
ka_applet_update_status (KaApplet *self, krb5_timestamp expiry)
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
            notify = get_notify_enabled (self, KA_SETTING_KEY_NOTIFY_VALID);
            if (notify) {
                self->notify_key = KA_SETTING_KEY_NOTIFY_VALID;

                if (self->krb_msg)
                    msg = self->krb_msg;
                else {
                    if (initial_notification)
                        msg = _("You have valid Kerberos credentials.");
                    else
                        msg = _("You've refreshed your Kerberos credentials.");
                }
                ka_send_event_notification (self,
                                            _("Network credentials valid"),
                                            msg,
                                            "krb-valid-ticket",
                                            FALSE);
            }
            ka_applet_signal_emit (self, KA_SIGNAL_ACQUIRED_TGT, expiry);
            expiry_notified = FALSE;
            g_free (self->krb_msg);
            self->krb_msg = NULL;
        } else {
            if (remaining < self->pw_prompt_secs
                && (now - last_warn) > NOTIFY_SECONDS
                && !self->renewable) {
                notify = get_notify_enabled (self,
                                             KA_SETTING_KEY_NOTIFY_EXPIRING);
                if (notify) {
                    self->notify_key =
                        KA_SETTING_KEY_NOTIFY_EXPIRING;
                    ka_send_event_notification (self,
                                                _("Network credentials expiring"),
                                                tooltip_text,
                                                "krb-expiring-ticket",
                                                TRUE);
                }
                last_warn = now;
            }
            /* ticket lifetime got longer e.g. by kinit -R */
            if (old_expiry && expiry > old_expiry)
                ka_applet_signal_emit (self, KA_SIGNAL_RENEWED_TGT, expiry);
        }
    } else {
        if (!expiry_notified) {
            notify = get_notify_enabled (self, KA_SETTING_KEY_NOTIFY_EXPIRED);
            if (notify) {
                self->notify_key = KA_SETTING_KEY_NOTIFY_EXPIRED;
                ka_send_event_notification (self,
                                            _("Network credentials expired"),
                                            _("Your Kerberos credentials have expired."),
                                            "krb-no-valid-ticket",
                                            TRUE);
            }
            ka_applet_signal_emit (self, KA_SIGNAL_EXPIRED_TGT, expiry);
            expiry_notified = TRUE;
            last_warn = 0;
        }
    }

    old_expiry = expiry;
    g_free (tooltip_text);
    initial_notification = FALSE;
    return 0;
}

guint
ka_applet_get_pw_prompt_secs (const KaApplet *self)
{
    return self->pw_prompt_secs;
}

void
ka_applet_set_tgt_renewable (KaApplet *self, gboolean renewable)
{
    self->renewable = renewable;
}

gboolean
ka_applet_get_tgt_renewable (const KaApplet *self)
{
    return self->renewable;
}

KaPwDialog *
ka_applet_get_pwdialog (const KaApplet *self)
{
    return self->pwdialog;
}

GSettings *
ka_applet_get_settings (const KaApplet *self)
{
    return self->settings;
}

void
ka_applet_set_msg (KaApplet *self, const char *msg)
{
    g_free (self->krb_msg);
    self->krb_msg = g_strdup (msg);
}

void
ka_applet_emit_renewed (KaApplet *self, krb5_timestamp expiry)
{
    g_autofree char *princ = NULL;

    g_return_if_fail (KA_IS_APPLET (self));

    ka_applet_signal_emit (self, KA_SIGNAL_RENEWED_TGT, expiry);
}

const char *
ka_applet_get_principal (KaApplet *self)
{
    g_assert (KA_IS_APPLET (self));

    return self->principal;
}
