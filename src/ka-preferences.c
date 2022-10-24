/*
 * Copyright (C) 2011,2014,2022 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"
#include "ka-preferences.h"

#include "ka-settings.h"
#include "ka-tools.h"

#include <glib/gi18n.h>

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

struct _KaPreferences {
    AdwPreferencesWindow parent;
    
    GtkWidget *dialog;
    GtkWidget *principal_entry;
    GtkWidget *pkuserid_entry;
    GtkWidget *pkuserid_button;
    GtkWidget *smartcard_toggle;
    GtkWidget *pkanchors_entry;
    GtkWidget *pkanchors_button;
    GtkWidget *prompt_mins_entry;
    GtkWidget *forwardable_toggle;
    GtkWidget *proxiable_toggle;
    GtkWidget *renewable_toggle;

    GSettings *settings;

    KaApplet *applet;
};

G_DEFINE_FINAL_TYPE (KaPreferences, ka_preferences, ADW_TYPE_PREFERENCES_WINDOW);

enum {
    PROP_0,
    PROP_APPLET,
};


static void
ka_preferences_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  KaPreferences *self = KA_PREFERENCES (object);

  switch (prop_id) {
  case PROP_APPLET:
      self->applet = g_value_get_object (value);
      break;
  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
ka_preferences_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  KaPreferences *self = KA_PREFERENCES (object);

  switch (prop_id) {
  case PROP_APPLET:
      g_value_set_object (value, self->applet);
      break;

  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
ka_preferences_principal_notify (KaPreferences *self,
                                 gchar *key)
{
    const char *principal;

    principal = g_settings_get_string (self->settings, key);

    if (!principal || !strlen(principal))
        ka_editable_set_text (self->principal_entry, "");
    else {
        const char *old_principal;

        old_principal = ka_editable_get_text (self->principal_entry);
        if (!old_principal || (old_principal && strcmp (old_principal, principal)))
            ka_editable_set_text (self->principal_entry, principal);
    }
}

static void
ka_preferences_principal_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *principal;
    KaPreferences *self = KA_PREFERENCES (userdata);

    principal = ka_editable_get_text (entry);

    if (principal && strlen(principal))
        g_object_set (self->applet, KA_PROP_NAME_PRINCIPAL, principal, NULL);
    else
        g_object_set (self->applet, KA_PROP_NAME_PRINCIPAL, "", NULL);
}


static void
ka_preferences_setup_principal_entry (KaPreferences *self)
{
    char *principal = NULL;

    g_object_get (self->applet, KA_PROP_NAME_PRINCIPAL, &principal, NULL);
    if (!principal)
        g_warning ("Getting principal failed");
    if (principal && strlen(principal))
        ka_editable_set_text (self->principal_entry, principal);
    g_free (principal);

    g_signal_connect (self->principal_entry, "changed",
                      G_CALLBACK (ka_preferences_principal_changed), self);
}


static void
ka_preferences_pkuserid_notify (KaPreferences *self,
                                gchar *key)
{
    const char *pkuserid;

    pkuserid = g_settings_get_string (self->settings, key);

    if (pkuserid && strlen(pkuserid)) {
        const char *old_pkuserid;

        old_pkuserid = ka_editable_get_text (self->pkuserid_entry);
        if (!old_pkuserid || (old_pkuserid && strcmp (old_pkuserid, pkuserid)))
            ka_editable_set_text (self->pkuserid_entry, pkuserid);
    } else {
        ka_editable_set_text (self->pkuserid_entry, "");
    }
}


static void
ka_preferences_pkuserid_changed (GtkEntry *entry,
                                 gpointer  userdata)
{
    const char *pkuserid;
    KaPreferences *self = KA_PREFERENCES (userdata);

    pkuserid = ka_editable_get_text (entry);

    if (pkuserid && strlen(pkuserid))
        g_object_set (self->applet, KA_PROP_NAME_PK_USERID, pkuserid, NULL);
    else
        g_object_set (self->applet, KA_PROP_NAME_PK_USERID, "", NULL);
}


static void
ka_preferences_setup_pkuserid_entry (KaPreferences *self)
{
    char *pkuserid = NULL;

    g_object_get (self->applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pkuserid failed");
    if (pkuserid && strlen(pkuserid))
        ka_editable_set_text (self->pkuserid_entry, pkuserid);
    if (pkuserid)
        g_free (pkuserid);

    g_signal_connect (self->pkuserid_entry, "changed",
                      G_CALLBACK (ka_preferences_pkuserid_changed), self);
}

static void
ka_preferences_pkanchors_notify (KaPreferences *self,
                                 gchar *key)
{
    const char *pkanchors;

    pkanchors = g_settings_get_string (self->settings, key);

    if (pkanchors && strlen(pkanchors)) {
        const char *old_pkanchors;

        old_pkanchors = ka_editable_get_text (self->pkanchors_entry);
        if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors,
                                                        pkanchors)))
            ka_editable_set_text (self->pkanchors_entry, pkanchors);
    } else {
        ka_editable_set_text (self->pkanchors_entry, "");
    }
}

static void
ka_preferences_pkanchors_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *pkanchors;
    KaPreferences *self = KA_PREFERENCES (userdata);

    pkanchors = ka_editable_get_text (entry);

    if (pkanchors && strlen(pkanchors))
        g_object_set (self->applet, KA_PROP_NAME_PK_ANCHORS, pkanchors, NULL);
    else
        g_object_set (self->applet, KA_PROP_NAME_PK_ANCHORS, "", NULL);
}


static void
ka_preferences_setup_pkanchors_entry (KaPreferences *self)
{
    char *pkanchors = NULL;

    g_object_get (self->applet, KA_PROP_NAME_PK_ANCHORS, &pkanchors, NULL);
    if (!pkanchors)
        g_warning ("Getting pkanchors failed");

    if (pkanchors && strlen(pkanchors))
        ka_editable_set_text (self->pkanchors_entry, pkanchors);
    if (pkanchors)
        g_free (pkanchors);

    g_signal_connect (self->pkanchors_entry, "changed",
                      G_CALLBACK (ka_preferences_pkanchors_changed), self);
}


static void
on_smartcard_toggled_active_changd (GtkWidget  *toggle,
                                    GParamSpec *pspec,
                                    gpointer    userdata)
{
    gboolean smartcard = gtk_switch_get_active (GTK_SWITCH (toggle));
    static gchar *old_path = NULL;
    KaPreferences *self = KA_PREFERENCES (userdata);

    if (smartcard) {
        const char *path;

        path = ka_editable_get_text (self->pkuserid_entry);
        if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
            g_free (old_path);
            old_path = g_strdup (path);
        }

        g_object_set (self->applet, KA_PROP_NAME_PK_USERID, PKINIT_SMARTCARD, NULL);
    } else {
        if (old_path && strlen(old_path))
            g_object_set (self->applet, KA_PROP_NAME_PK_USERID, old_path, NULL);
        else
            g_object_set (self->applet, KA_PROP_NAME_PK_USERID, "", NULL);
    }
}


static void
ka_preferences_setup_smartcard_toggle (KaPreferences *self)
{
    g_autofree char *pkuserid = NULL;
    gboolean active = FALSE;

    g_object_get(self->applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pk userid failed");

    g_signal_connect (self->smartcard_toggle, "notify::active",
                      G_CALLBACK (on_smartcard_toggled_active_changd), self);

    if (g_strcmp0 (pkuserid, PKINIT_SMARTCARD) == 0)
        active = TRUE;

    gtk_switch_set_active (GTK_SWITCH (self->smartcard_toggle), active);
}


static void
on_file_chooser_response (GtkDialog* dialog, gint response_id, gpointer user_data)
{
    GtkFileChooser *filechooser = GTK_FILE_CHOOSER (dialog);
    GtkEntry *entry = GTK_ENTRY (user_data);
    g_autofree gchar *filename = NULL;

    if (response_id == GTK_RESPONSE_ACCEPT) {
        g_autoptr(GFile) file = gtk_file_chooser_get_file (filechooser);
        filename = g_file_get_path (file);
    }

    ka_window_destroy (filechooser);

    if (filename) {
        g_autofree gchar *cert = g_strconcat (PKINIT_FILE, filename, NULL);
        ka_editable_set_text (entry, cert);
    }
}


static void
ka_preferences_browse_certs (KaPreferences *self, GtkEntry *entry)
{
    GtkWidget *filechooser;
    g_autoptr(GtkFileFilter) cert_filter = g_object_ref_sink (gtk_file_filter_new ());
    g_autoptr(GtkFileFilter) all_filter = g_object_ref_sink (gtk_file_filter_new ());
    const gchar *current;

    filechooser = gtk_file_chooser_dialog_new(_("Choose Certificate"),
                                              GTK_WINDOW (self),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                                              _("_Open"), GTK_RESPONSE_ACCEPT,
                                              NULL);

    current = ka_editable_get_text (entry);
    if (current && g_str_has_prefix (current, PKINIT_FILE) &&
        strlen(current) > strlen (PKINIT_FILE)) {
        g_autoptr(GFile) file = g_file_new_for_path (
            (const gchar*)&current[strlen(PKINIT_FILE)]);

        gtk_file_chooser_set_file (GTK_FILE_CHOOSER(filechooser), file, NULL);
    }

    gtk_file_filter_add_mime_type (cert_filter, "application/x-x509-ca-cert");
    gtk_file_filter_set_name (cert_filter, _("X509 Certificates"));
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filechooser), cert_filter);

    gtk_file_filter_add_pattern (all_filter, "*");
    gtk_file_filter_set_name (all_filter, _("all files"));
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filechooser), all_filter);

    gtk_window_set_modal (GTK_WINDOW (filechooser), TRUE);
    g_signal_connect (filechooser, "response",
                      G_CALLBACK (on_file_chooser_response), entry);
    gtk_window_present (GTK_WINDOW(filechooser));
}

static void
ka_preferences_browse_pkuserids (GtkButton *button G_GNUC_UNUSED,
                                 gpointer userdata)
{
    KaPreferences *self = KA_PREFERENCES (userdata);
    ka_preferences_browse_certs (self, GTK_ENTRY(self->pkuserid_entry));
}

static void
ka_preferences_browse_pkanchors(GtkButton *button G_GNUC_UNUSED,
                                gpointer userdata)
{
    KaPreferences *self = KA_PREFERENCES (userdata);
    ka_preferences_browse_certs (self, GTK_ENTRY(self->pkanchors_entry));
}

static void
ka_preferences_setup_pkuserid_button (KaPreferences *self)
{
    g_signal_connect (self->pkuserid_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkuserids), self);

}

static void
ka_preferences_setup_pkanchors_button (KaPreferences *self)
{
    g_signal_connect (self->pkanchors_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkanchors), self);

}


static void
ka_preferences_setup_ticket_toggle (KaPreferences *self,
                                    GtkWidget     *toggle,
                                    const char    *prop_name)
{
    g_object_bind_property (self->applet,
                            prop_name,
                            toggle,
                            "active",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}


static void
ka_preferences_prompt_mins_changed (GtkSpinButton *button,
                                    gpointer userdata)
{
    gint prompt_mins;
    KaPreferences *self = KA_PREFERENCES (userdata);

    prompt_mins = gtk_spin_button_get_value_as_int (button);
    g_object_set (self->applet, KA_PROP_NAME_PW_PROMPT_MINS, prompt_mins, NULL);
}


static void
ka_preferences_prompt_mins_notify (KaPreferences *self,
                                   gchar *key)
{
    gint prompt_mins;

    prompt_mins = g_settings_get_int (self->settings, key);
    if (prompt_mins != gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON (self->prompt_mins_entry)))
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->prompt_mins_entry),
                                   prompt_mins);
}

static void
ka_preferences_setup_prompt_mins_entry (KaPreferences *self)
{
    gint prompt_mins;

    g_object_get (self->applet, KA_PROP_NAME_PW_PROMPT_MINS, &prompt_mins, NULL);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->prompt_mins_entry),
                               prompt_mins);

    g_signal_connect (self->prompt_mins_entry, "value-changed",
                      G_CALLBACK (ka_preferences_prompt_mins_changed), self);
}


/* GSettings changed, update the prefs dialog */
static void
ka_preferences_settings_changed (GSettings *settings G_GNUC_UNUSED,
                                 gchar *key,
                                 gpointer userdata)
{
    KaPreferences *self = KA_PREFERENCES (userdata);

    if (!g_strcmp0 (key, KA_SETTING_KEY_PRINCIPAL))
        ka_preferences_principal_notify (self, key);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PK_USERID))
        ka_preferences_pkuserid_notify (self, key);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PK_ANCHORS))
        ka_preferences_pkanchors_notify (self, key);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PW_PROMPT_MINS))
        ka_preferences_prompt_mins_notify (self, key);
}


static void
ka_preferences_constructed (GObject *object)
{
  KaPreferences *self = KA_PREFERENCES (object);

  G_OBJECT_CLASS (ka_preferences_parent_class)->constructed (object);

  g_assert_nonnull (self->applet);
  self->settings = ka_applet_get_settings(self->applet);
  ka_preferences_setup_principal_entry (self);
  ka_preferences_setup_pkuserid_entry (self);
  ka_preferences_setup_pkuserid_button (self);
  ka_preferences_setup_smartcard_toggle (self);
  ka_preferences_setup_pkanchors_entry (self);
  ka_preferences_setup_pkanchors_button (self);

  ka_preferences_setup_ticket_toggle (self, self->forwardable_toggle, KA_PROP_NAME_TGT_FORWARDABLE);
  ka_preferences_setup_ticket_toggle (self, self->proxiable_toggle, KA_PROP_NAME_TGT_PROXIABLE);
  ka_preferences_setup_ticket_toggle (self, self->renewable_toggle, KA_PROP_NAME_TGT_RENEWABLE);
  ka_preferences_setup_prompt_mins_entry (self);

  g_signal_connect (ka_applet_get_settings(self->applet),
                    "changed",
                    G_CALLBACK (ka_preferences_settings_changed),
                    self);
}


static void
ka_preferences_init (KaPreferences *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
}


static void
ka_preferences_class_init (KaPreferencesClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->set_property = ka_preferences_set_property;
    object_class->get_property = ka_preferences_get_property;
    object_class->constructed = ka_preferences_constructed;

    g_object_class_install_property (object_class,
                                     PROP_APPLET,
                                     g_param_spec_object ("applet",
                                                          "Applet",
                                                          "The applet we configure",
                                                          KA_TYPE_APPLET,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_STATIC_STRINGS));
    /* Bind class to template
     */
    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/krb5-auth-dialog/ui/ka-preferences.ui");

    gtk_widget_class_bind_template_child (widget_class, KaPreferences, principal_entry);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, pkuserid_button);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, pkuserid_entry);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, smartcard_toggle);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, pkanchors_entry);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, pkanchors_button);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, prompt_mins_entry);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, forwardable_toggle);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, renewable_toggle);
    gtk_widget_class_bind_template_child (widget_class, KaPreferences, proxiable_toggle);
}


KaPreferences*
ka_preferences_new (KaApplet *applet)
{
    KaPreferences *self;

    self = g_object_new (KA_TYPE_PREFERENCES, "applet", applet, NULL);
    return self;
}


void
ka_preferences_run (KaPreferences *self)
{
    GtkWindow *parent = ka_applet_last_focused_window (self->applet);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW(self),
                                      GTK_WINDOW(parent));

    gtk_window_set_modal (GTK_WINDOW (self), TRUE);
    gtk_window_present (GTK_WINDOW(self));
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
