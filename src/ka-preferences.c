/*
 * Copyright (C) 2011,2014 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.        See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include "ka-preferences.h"

#include "ka-settings.h"
#include "ka-tools.h"

#include <glib/gi18n.h>

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

#define N_BINDINGS 3

struct _KaPreferences {
    GtkDialog parent;

    KaPreferencesPrivate *priv;
};

struct _KaPreferencesClass {
    GtkDialogClass parent;
};

struct _KaPreferencesPrivate {
    GtkWidget *dialog;
    GtkWidget *notebook;
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
    GBinding* bindings[N_BINDINGS];
    int       n_bindings;

    KaApplet *applet;
};

G_DEFINE_TYPE_WITH_PRIVATE (KaPreferences, ka_preferences, GTK_TYPE_DIALOG);

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
      self->priv->applet = g_value_get_object (value);
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
      g_value_set_object (value, self->priv->applet);
      break;

  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static GObject *
ka_preferences_constructor (GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_params)
{
    GObject *object;
    guint idx;
    GParamSpec *pspec;
    GValue *value;

    for (idx = 0; idx < n_construct_properties; idx++)
    {
        pspec = construct_params[idx].pspec;
        if (g_strcmp0 (pspec->name, "use-header-bar") != 0)
            continue;

        /* GtkDialog uses "-1" to imply an unset value for this property */
        value = construct_params[idx].value;
        if (g_value_get_int (value) == -1)
            g_value_set_int (value, 1);

        break;
    }

    object = G_OBJECT_CLASS (ka_preferences_parent_class)->constructor (type,
                                                                     n_construct_properties,
                                                                     construct_params);

    return object;
}


static void
ka_preferences_principal_notify (KaPreferences *self,
                                 gchar *key)
{
    const char *principal;

    principal = g_settings_get_string (self->priv->settings, key);

    if (!principal || !strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (self->priv->principal_entry), "");
    else {
        const char *old_principal;

        old_principal = gtk_entry_get_text (GTK_ENTRY (self->priv->principal_entry));
        if (!old_principal || (old_principal && strcmp (old_principal, principal)))
            gtk_entry_set_text (GTK_ENTRY (self->priv->principal_entry), principal);
    }
}

static void
ka_preferences_principal_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *principal;
    KaPreferences *self = KA_PREFERENCES (userdata);

    principal = gtk_entry_get_text (entry);

    if (principal && strlen(principal))
        g_object_set (self->priv->applet, KA_PROP_NAME_PRINCIPAL, principal, NULL);
    else
        g_object_set (self->priv->applet, KA_PROP_NAME_PRINCIPAL, "", NULL);
}


static void
ka_preferences_setup_principal_entry (KaPreferences *self)
{
    char *principal = NULL;

    g_object_get (self->priv->applet, KA_PROP_NAME_PRINCIPAL, &principal, NULL);
    if (!principal)
        g_warning ("Getting principal failed");
    if (principal && strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (self->priv->principal_entry), principal);
    g_free (principal);

    g_signal_connect (self->priv->principal_entry, "changed",
                      G_CALLBACK (ka_preferences_principal_changed), self);
}


static void
ka_preferences_pkuserid_notify (KaPreferences *self,
                                gchar *key)
{
    const char *pkuserid;

    pkuserid = g_settings_get_string (self->priv->settings, key);

    if (pkuserid && strlen(pkuserid)) {
        const char *old_pkuserid;

        old_pkuserid = gtk_entry_get_text (GTK_ENTRY (self->priv->pkuserid_entry));
        if (!old_pkuserid || (old_pkuserid && strcmp (old_pkuserid, pkuserid)))
            gtk_entry_set_text (GTK_ENTRY (self->priv->pkuserid_entry), pkuserid);
    } else {
        gtk_entry_set_text (GTK_ENTRY (self->priv->pkuserid_entry), "");
    }
}


static void
ka_preferences_pkuserid_changed (GtkEntry *entry,
                                 gpointer *userdata)
{
    const char *pkuserid;
    KaPreferences *self = KA_PREFERENCES (userdata);

    pkuserid = gtk_entry_get_text (entry);

    if (pkuserid && strlen(pkuserid))
        g_object_set (self->priv->applet, KA_PROP_NAME_PK_USERID, pkuserid, NULL);
    else
        g_object_set (self->priv->applet, KA_PROP_NAME_PK_USERID, "", NULL);
}


static void
ka_preferences_setup_pkuserid_entry (KaPreferences *self)
{
    char *pkuserid = NULL;

    g_object_get (self->priv->applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pkuserid failed");
    if (pkuserid && strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (self->priv->pkuserid_entry), pkuserid);
    if (pkuserid)
        g_free (pkuserid);

    g_signal_connect (self->priv->pkuserid_entry, "changed",
                      G_CALLBACK (ka_preferences_pkuserid_changed), self);
}

static void
ka_preferences_pkanchors_notify (KaPreferences *self,
                                 gchar *key)
{
    const char *pkanchors;

    pkanchors = g_settings_get_string (self->priv->settings, key);

    if (pkanchors && strlen(pkanchors)) {
        const char *old_pkanchors;

        old_pkanchors = gtk_entry_get_text (GTK_ENTRY (self->priv->pkanchors_entry));
        if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors,
                                                        pkanchors)))
            gtk_entry_set_text (GTK_ENTRY (self->priv->pkanchors_entry), pkanchors);
    } else {
        gtk_entry_set_text (GTK_ENTRY (self->priv->pkanchors_entry), "");
    }
}

static void
ka_preferences_pkanchors_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *pkanchors;
    KaPreferences *self = KA_PREFERENCES (userdata);

    pkanchors = gtk_entry_get_text (entry);

    if (pkanchors && strlen(pkanchors))
        g_object_set (self->priv->applet, KA_PROP_NAME_PK_ANCHORS, pkanchors, NULL);
    else
        g_object_set (self->priv->applet, KA_PROP_NAME_PK_ANCHORS, "", NULL);
}


static void
ka_preferences_setup_pkanchors_entry (KaPreferences *self)
{
    char *pkanchors = NULL;

    g_object_get (self->priv->applet, KA_PROP_NAME_PK_ANCHORS, &pkanchors, NULL);
    if (!pkanchors)
        g_warning ("Getting pkanchors failed");

    if (pkanchors && strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (self->priv->pkanchors_entry), pkanchors);
    if (pkanchors)
        g_free (pkanchors);

    g_signal_connect (self->priv->pkanchors_entry, "changed",
                      G_CALLBACK (ka_preferences_pkanchors_changed), self);
}


static void
ka_preferences_toggle_pkuserid_entry (KaPreferences *self, gboolean state)
{
    gtk_widget_set_sensitive (self->priv->pkuserid_entry, state);
    gtk_widget_set_sensitive (self->priv->pkuserid_button, state);
}


static void
ka_preferences_smartcard_toggled (GtkToggleButton *toggle,
                                  gpointer userdata)
{
    gboolean smartcard = gtk_toggle_button_get_active (toggle);
    static gchar *old_path = NULL;
    KaPreferences *self = KA_PREFERENCES(userdata);

    if (smartcard) {
        const char *path;

        path = gtk_entry_get_text (GTK_ENTRY(self->priv->pkuserid_entry));
        if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
            g_free (old_path);
            old_path = g_strdup (path);
        }

        ka_preferences_toggle_pkuserid_entry (self, FALSE);
        g_object_set (self->priv->applet, KA_PROP_NAME_PK_USERID, PKINIT_SMARTCARD, NULL);
    } else {
        ka_preferences_toggle_pkuserid_entry (self, TRUE);
        if (old_path && strlen(old_path))
            g_object_set (self->priv->applet, KA_PROP_NAME_PK_USERID, old_path, NULL);
        else
            g_object_set (self->priv->applet, KA_PROP_NAME_PK_USERID, "", NULL);
    }
}


static void
ka_preferences_setup_smartcard_toggle (KaPreferences *self)
{
    char *pkuserid = NULL;

    g_object_get(self->priv->applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pk userid failed");

    g_signal_connect (self->priv->smartcard_toggle, "toggled",
                      G_CALLBACK (ka_preferences_smartcard_toggled), self);

    if (!g_strcmp0 (pkuserid, PKINIT_SMARTCARD))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->smartcard_toggle), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->smartcard_toggle), FALSE);

    g_free (pkuserid);
}


static void
ka_preferences_browse_certs (KaPreferences *self, GtkEntry *entry)
{
    GtkWidget *filechooser;
    GtkFileFilter *cert_filter, *all_filter;
    gchar *filename = NULL;
    const gchar *current;
    gint ret;

    filechooser = gtk_file_chooser_dialog_new(_("Choose Certificate"),
                                              GTK_WINDOW (self),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                                              _("_Open"), GTK_RESPONSE_ACCEPT,
                                              NULL);

    current = gtk_entry_get_text (entry);
    if (current && g_str_has_prefix (current, PKINIT_FILE) &&
        strlen(current) > strlen (PKINIT_FILE)) {
        gtk_file_chooser_select_filename (GTK_FILE_CHOOSER(filechooser),
                                          (const gchar*)&current[strlen(PKINIT_FILE)]);
    }

    cert_filter = g_object_ref_sink (gtk_file_filter_new ());
    gtk_file_filter_add_mime_type (cert_filter, "application/x-x509-ca-cert");
    gtk_file_filter_set_name (cert_filter, _("X509 Certificates"));
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filechooser), cert_filter);
    all_filter = g_object_ref_sink (gtk_file_filter_new ());
    gtk_file_filter_add_pattern (all_filter, "*");
    gtk_file_filter_set_name (all_filter, _("all files"));
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filechooser), all_filter);

    ret = gtk_dialog_run (GTK_DIALOG(filechooser));
    if (ret == GTK_RESPONSE_ACCEPT)
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(filechooser));
    gtk_widget_destroy (GTK_WIDGET(filechooser));

    if (filename) {
        gchar *cert = g_strconcat (PKINIT_FILE, filename, NULL);
        gtk_entry_set_text (entry, cert);
        g_free (cert);
    }
    g_object_unref (cert_filter);
    g_object_unref (all_filter);
    g_free (filename);
}

static void
ka_preferences_browse_pkuserids (GtkButton *button G_GNUC_UNUSED,
                                 gpointer userdata)
{
    KaPreferences *self = KA_PREFERENCES (userdata);
    ka_preferences_browse_certs (self, GTK_ENTRY(self->priv->pkuserid_entry));
}

static void
ka_preferences_browse_pkanchors(GtkButton *button G_GNUC_UNUSED,
                                gpointer userdata)
{
    KaPreferences *self = KA_PREFERENCES (userdata);
    ka_preferences_browse_certs (self, GTK_ENTRY(self->priv->pkanchors_entry));
}

static void
ka_preferences_setup_pkuserid_button (KaPreferences *self)
{
    g_signal_connect (self->priv->pkuserid_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkuserids), self);

}

static void
ka_preferences_setup_pkanchors_button (KaPreferences *self)
{
    g_signal_connect (self->priv->pkanchors_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkanchors), self);

}

static void
ka_preferences_setup_forwardable_toggle (KaPreferences *self)
{
    GBinding *binding;

    binding = g_object_bind_property (self->priv->applet,
                                      KA_PROP_NAME_TGT_FORWARDABLE,
                                      self->priv->forwardable_toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL |
                                      G_BINDING_SYNC_CREATE);
    self->priv->bindings[self->priv->n_bindings] = binding;
    self->priv->n_bindings++;
}

static void
ka_preferences_setup_proxiable_toggle (KaPreferences *self)
{
    GBinding *binding;

    binding = g_object_bind_property (self->priv->applet,
                                      KA_PROP_NAME_TGT_PROXIABLE,
                                      self->priv->proxiable_toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL |
                                      G_BINDING_SYNC_CREATE);
    self->priv->bindings[self->priv->n_bindings] = binding;
    self->priv->n_bindings++;
}

static void
ka_preferences_setup_renewable_toggle (KaPreferences *self)
{
    GBinding *binding;

    binding = g_object_bind_property (self->priv->applet,
                                      KA_PROP_NAME_TGT_RENEWABLE,
                                      self->priv->renewable_toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL |
                                      G_BINDING_SYNC_CREATE);
    self->priv->bindings[self->priv->n_bindings] = binding;
    self->priv->n_bindings++;
}


static void
ka_preferences_prompt_mins_changed (GtkSpinButton *button,
                                    gpointer userdata)
{
    gint prompt_mins;
    KaPreferences *self = KA_PREFERENCES (userdata);

    prompt_mins = gtk_spin_button_get_value_as_int (button);
    g_object_set (self->priv->applet, KA_PROP_NAME_PW_PROMPT_MINS, prompt_mins, NULL);
}


static void
ka_preferences_prompt_mins_notify (KaPreferences *self,
                                   gchar *key)
{
    gint prompt_mins;

    prompt_mins = g_settings_get_int (self->priv->settings, key);
    if (prompt_mins != gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON (self->priv->prompt_mins_entry)))
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->priv->prompt_mins_entry),
                                   prompt_mins);
}

static void
ka_preferences_setup_prompt_mins_entry (KaPreferences *self)
{
    gint prompt_mins;

    g_object_get (self->priv->applet, KA_PROP_NAME_PW_PROMPT_MINS, &prompt_mins, NULL);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (self->priv->prompt_mins_entry),
                               prompt_mins);

    g_signal_connect (self->priv->prompt_mins_entry, "value-changed",
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

  if (G_OBJECT_CLASS (ka_preferences_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (ka_preferences_parent_class)->constructed (object);

  g_assert_nonnull (self->priv->applet);
  self->priv->settings = ka_applet_get_settings(self->priv->applet);
  ka_preferences_setup_principal_entry (self);
  ka_preferences_setup_pkuserid_entry (self);
  ka_preferences_setup_pkuserid_button (self);
  ka_preferences_setup_smartcard_toggle (self);
  ka_preferences_setup_pkanchors_entry (self);
  ka_preferences_setup_pkanchors_button (self);

  ka_preferences_setup_forwardable_toggle (self);
  ka_preferences_setup_proxiable_toggle (self);
  ka_preferences_setup_renewable_toggle (self);
  ka_preferences_setup_prompt_mins_entry (self);

  g_signal_connect (ka_applet_get_settings(self->priv->applet),
                    "changed",
                    G_CALLBACK (ka_preferences_settings_changed),
                    self);

  g_assert (self->priv->n_bindings == N_BINDINGS);
}


static void
ka_preferences_init (KaPreferences *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              KA_TYPE_PREFERENCES,
                                              KaPreferencesPrivate);

    gtk_widget_init_template (GTK_WIDGET (self));
}


static void
ka_preferences_class_init (KaPreferencesClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->constructor = ka_preferences_constructor;
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

    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, principal_entry);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, pkuserid_button);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, pkuserid_entry);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, smartcard_toggle);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, pkanchors_entry);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, pkanchors_button);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, prompt_mins_entry);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, forwardable_toggle);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, renewable_toggle);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, proxiable_toggle);
    gtk_widget_class_bind_template_child_private (widget_class, KaPreferences, notebook);
}


KaPreferences*
ka_preferences_new (KaApplet *applet)
{
    KaPreferences *self;
    gboolean use_header;

    g_object_get (gtk_settings_get_default (), "gtk-dialogs-use-header", &use_header, NULL);
    self = g_object_new (KA_TYPE_PREFERENCES,
                         "applet", applet,
                         "use-header-bar", use_header,
                         NULL);
    return self;
}


void
ka_preferences_run (KaPreferences *self)
{
    GtkWindow *parent = ka_applet_last_focused_window (self->priv->applet);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW(self),
                                      GTK_WINDOW(parent));
    gtk_window_present (GTK_WINDOW(self));
    gtk_dialog_run (GTK_DIALOG (self));
    gtk_widget_hide (GTK_WIDGET (self));
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
