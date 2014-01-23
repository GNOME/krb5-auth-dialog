/*
 * Copyright (C) 2011 Guido Guenther <agx@sigxcpu.org>
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

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

#define N_BINDINGS 3

struct _KaPreferencesPrivate {
    GtkBuilder *builder;

    GSettings *settings;

    GtkWidget *dialog;
    GtkWidget *notebook;
    GtkWidget *principal_entry;
    GtkWidget *pkuserid_entry;
    GtkWidget *pkuserid_button;
    GtkWidget *smartcard_toggle;
    GtkWidget *pkanchors_entry;
    GtkWidget *pkanchors_button;
    GtkWidget *prompt_mins_entry;

    GBinding* bindings[N_BINDINGS];
    int       n_bindings;
} prefs;


static void
ka_preferences_principal_notify (GSettings *settings,
                                 gchar *key,
                                 gpointer userdata G_GNUC_UNUSED)
{
    const char *principal;

    principal = g_settings_get_string (settings, key);

    if (!principal || !strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (prefs.principal_entry), "");
    else {
        const char *old_principal;

        old_principal = gtk_entry_get_text (GTK_ENTRY (prefs.principal_entry));
        if (!old_principal || (old_principal && strcmp (old_principal, principal)))
            gtk_entry_set_text (GTK_ENTRY (prefs.principal_entry), principal);
    }
}

static void
ka_preferences_principal_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *principal;
    KaApplet *applet = KA_APPLET(userdata);

    principal = gtk_entry_get_text (entry);

    if (principal || strlen(principal))
        g_object_set (applet, KA_PROP_NAME_PRINCIPAL, principal, NULL);
    else
        g_object_set (applet, KA_PROP_NAME_PRINCIPAL, "", NULL);
}


static void
ka_preferences_setup_principal_entry (KaApplet *applet)
{
    char *principal = NULL;

    prefs.principal_entry = WID (prefs.builder, "principal_entry");
    g_assert (prefs.principal_entry != NULL);

    g_object_get (applet, KA_PROP_NAME_PRINCIPAL, &principal, NULL);
    if (!principal)
        g_warning ("Getting principal failed");
    if (principal && strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (prefs.principal_entry), principal);
    g_free (principal);

    g_signal_connect (prefs.principal_entry, "changed",
                      G_CALLBACK (ka_preferences_principal_changed), applet);
}


static void
ka_preferences_pkuserid_notify (GSettings *settings,
                                gchar *key,
                                gpointer userdata G_GNUC_UNUSED)
{
    const char *pkuserid;

    pkuserid = g_settings_get_string (settings, key);

    if (!pkuserid || !strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkuserid_entry), "");
    else {
        const char *old_pkuserid;

        old_pkuserid = gtk_entry_get_text (GTK_ENTRY (prefs.pkuserid_entry));
        if (!old_pkuserid || (old_pkuserid && strcmp (old_pkuserid, pkuserid)))
            gtk_entry_set_text (GTK_ENTRY (prefs.pkuserid_entry), pkuserid);
    }
}


static void
ka_preferences_pkuserid_changed (GtkEntry *entry,
                                 gpointer *userdata)
{
    const char *pkuserid;
    KaApplet *applet = KA_APPLET(userdata);

    pkuserid = gtk_entry_get_text (entry);

    if (!pkuserid || !strlen(pkuserid))
        g_object_set (applet, KA_PROP_NAME_PK_USERID, "", NULL);
    else
        g_object_set (applet, KA_PROP_NAME_PK_USERID, pkuserid, NULL);
}

static void
ka_preferences_setup_pkuserid_entry (KaApplet *applet)
{
    char     *pkuserid = NULL;

    prefs.pkuserid_entry = WID(prefs.builder, "pkuserid_entry");
    g_assert (prefs.pkuserid_entry != NULL);

    g_object_get (applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pkuserid failed");
    if (pkuserid && strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkuserid_entry), pkuserid);
    if (pkuserid)
        g_free (pkuserid);

    g_signal_connect (prefs.pkuserid_entry, "changed",
                      G_CALLBACK (ka_preferences_pkuserid_changed), applet);
}

static void
ka_preferences_pkanchors_notify (GSettings *settings,
                                 gchar *key,
                                 gpointer userdata G_GNUC_UNUSED)
{
    const char *pkanchors;

    pkanchors = g_settings_get_string (settings, key);

    if (!pkanchors || !strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), "");
    else {
        const char *old_pkanchors;

        old_pkanchors = gtk_entry_get_text (GTK_ENTRY (prefs.pkanchors_entry));
        if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors,
                                                        pkanchors)))
            gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), pkanchors);
    }
}

static void
ka_preferences_pkanchors_changed (GtkEntry *entry,
                                  gpointer userdata)
{
    const char *pkanchors;
    KaApplet *applet = KA_APPLET(userdata);

    pkanchors = gtk_entry_get_text (entry);

    if (!pkanchors || !strlen(pkanchors))
        g_object_set (applet, KA_PROP_NAME_PK_ANCHORS, "", NULL);
    else
        g_object_set (applet, KA_PROP_NAME_PK_ANCHORS, pkanchors, NULL);
}


static void
ka_preferences_setup_pkanchors_entry (KaApplet *applet)
{
    char *pkanchors = NULL;

    prefs.pkanchors_entry = WID(prefs.builder, "pkanchors_entry");
    g_assert (prefs.pkanchors_entry != NULL);

    g_object_get (applet, KA_PROP_NAME_PK_ANCHORS, &pkanchors, NULL);
    if (!pkanchors)
        g_warning ("Getting pkanchors failed");

    if (pkanchors && strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), pkanchors);
    if (pkanchors)
        g_free (pkanchors);

    g_signal_connect (prefs.pkanchors_entry, "changed",
                      G_CALLBACK (ka_preferences_pkanchors_changed), applet);
}


static void
ka_preferences_toggle_pkuserid_entry (gboolean state, gpointer userdata G_GNUC_UNUSED)
{
    gtk_widget_set_sensitive (prefs.pkuserid_entry, state);
    gtk_widget_set_sensitive (prefs.pkuserid_button, state);
}


static void
ka_preferences_smartcard_toggled (GtkToggleButton *toggle,
                                  gpointer userdata)
{
    gboolean smartcard = gtk_toggle_button_get_active (toggle);
    static gchar *old_path = NULL;
    KaApplet *applet = KA_APPLET(userdata);

    if (smartcard) {
        const char *path;

        path = gtk_entry_get_text (GTK_ENTRY(prefs.pkuserid_entry));
        if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
            g_free (old_path);
            old_path = g_strdup (path);
        }

        ka_preferences_toggle_pkuserid_entry (FALSE, NULL);
        g_object_set (applet, KA_SETTING_KEY_PK_USERID, PKINIT_SMARTCARD, NULL);
    } else {
        ka_preferences_toggle_pkuserid_entry (TRUE, NULL);
        if (old_path)
            g_object_set (applet, KA_SETTING_KEY_PK_USERID, old_path, NULL);
        else
            g_object_set (applet, KA_SETTING_KEY_PK_USERID, old_path, "", NULL);
    }
}


static void
ka_preferences_setup_smartcard_toggle (KaApplet *applet)
{
    char *pkuserid = NULL;

    prefs.smartcard_toggle = WID (prefs.builder, "smartcard_toggle");
    g_assert (prefs.smartcard_toggle != NULL);

    g_object_get(applet, KA_PROP_NAME_PK_USERID, &pkuserid, NULL);
    if (!pkuserid)
        g_warning ("Getting pk userid failed");

    g_signal_connect (prefs.smartcard_toggle, "toggled",
                      G_CALLBACK (ka_preferences_smartcard_toggled), applet);

    if (!g_strcmp0 (pkuserid, PKINIT_SMARTCARD))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.smartcard_toggle), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.smartcard_toggle), FALSE);

    if (pkuserid)
        g_free (pkuserid);
}


static void
ka_preferences_browse_certs (GtkEntry *entry)
{
    GtkWidget *filechooser;
    GtkFileFilter *cert_filter, *all_filter;
    gchar *filename = NULL;
    const gchar *current;
    gint ret;

    filechooser = gtk_file_chooser_dialog_new(_("Choose Certificate"),
                                              GTK_WINDOW (prefs.dialog),
                                              GTK_FILE_CHOOSER_ACTION_OPEN,
                                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                              GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
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
        gchar *cert = g_strconcat( PKINIT_FILE, filename, NULL);
        gtk_entry_set_text (entry, cert);
        g_free (filename);
        g_free (cert);
  }
    g_object_unref (cert_filter);
    g_object_unref (all_filter);
}

static void
ka_preferences_browse_pkuserids (GtkButton *button G_GNUC_UNUSED,
                                 gpointer userdata G_GNUC_UNUSED)
{
    ka_preferences_browse_certs (GTK_ENTRY(prefs.pkuserid_entry));
}

static void
ka_preferences_browse_pkanchors(GtkButton *button G_GNUC_UNUSED,
                                gpointer userdata G_GNUC_UNUSED)
{
    ka_preferences_browse_certs (GTK_ENTRY(prefs.pkanchors_entry));
}

static void
ka_preferences_setup_pkuserid_button ()
{
    prefs.pkuserid_button = WID (prefs.builder, "pkuserid_button");
    g_assert (prefs.pkuserid_button != NULL);

    g_signal_connect (prefs.pkuserid_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkuserids), NULL);

}

static void
ka_preferences_setup_pkanchors_button ()
{
    prefs.pkanchors_button = WID (prefs.builder, "pkanchors_button");
    g_assert (prefs.pkanchors_button != NULL);

    g_signal_connect (prefs.pkanchors_button, "clicked",
                      G_CALLBACK (ka_preferences_browse_pkanchors), NULL);

}

static void
ka_preferences_setup_forwardable_toggle (KaApplet *applet)
{
    GBinding *binding;
    GtkWidget *toggle;

    toggle = WID (prefs.builder, "forwardable_toggle");
    g_assert (toggle != NULL);

    binding = g_object_bind_property (applet,
                                      KA_PROP_NAME_TGT_FORWARDABLE,
                                      toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL);
    prefs.bindings[prefs.n_bindings] = binding;
    prefs.n_bindings++;
}

static void
ka_preferences_setup_proxiable_toggle (KaApplet *applet)
{
    GBinding *binding;
    GtkWidget *toggle;

    toggle = WID (prefs.builder, "proxiable_toggle");
    g_assert (toggle != NULL);

    binding = g_object_bind_property (applet,
                                      KA_PROP_NAME_TGT_PROXIABLE,
                                      toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL);
    prefs.bindings[prefs.n_bindings] = binding;
    prefs.n_bindings++;
}

static void
ka_preferences_setup_renewable_toggle (KaApplet *applet)
{
    GBinding *binding;
    GtkWidget *toggle;

    toggle = WID (prefs.builder, "renewable_toggle");
    g_assert (toggle != NULL);

    binding = g_object_bind_property (applet,
                                      KA_PROP_NAME_TGT_RENEWABLE,
                                      toggle,
                                      "active",
                                      G_BINDING_BIDIRECTIONAL);
    prefs.bindings[prefs.n_bindings] = binding;
    prefs.n_bindings++;
}


static void
ka_preferences_prompt_mins_changed (GtkSpinButton *button,
                                    gpointer userdata)
{
    gint prompt_mins;
    KaApplet *applet = KA_APPLET(userdata);

    prompt_mins = gtk_spin_button_get_value_as_int (button);
    g_object_set (applet, KA_PROP_NAME_PW_PROMPT_MINS, prompt_mins, NULL);
}


static void
ka_preferences_prompt_mins_notify (GSettings *settings,
                                   gchar *key,
                                   gpointer userdata G_GNUC_UNUSED)
{
    gint prompt_mins;

    prompt_mins = g_settings_get_int (settings, key);
    if (prompt_mins != gtk_spin_button_get_value_as_int (
            GTK_SPIN_BUTTON (prefs.prompt_mins_entry)))
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs.prompt_mins_entry),
                                   prompt_mins);
}

static void
ka_preferences_setup_prompt_mins_entry (KaApplet *applet)
{
    gint prompt_mins;

    prefs.prompt_mins_entry = WID (prefs.builder, "prompt_mins_entry");
    g_assert (prefs.prompt_mins_entry != NULL);

    g_object_get (applet, KA_PROP_NAME_PW_PROMPT_MINS, &prompt_mins, NULL);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs.prompt_mins_entry),
                               prompt_mins);

    g_signal_connect (prefs.prompt_mins_entry, "value-changed",
                      G_CALLBACK (ka_preferences_prompt_mins_changed), applet);
}


static void
ka_preferences_settings_changed (GSettings *settings,
                                 gchar *key,
                                 gpointer userdata)
{
    KaApplet *applet = KA_APPLET (userdata);

    if (!g_strcmp0 (key, KA_SETTING_KEY_PRINCIPAL))
        ka_preferences_principal_notify (settings, key, applet);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PK_USERID))
        ka_preferences_pkuserid_notify (settings, key, applet);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PK_ANCHORS))
        ka_preferences_pkanchors_notify (settings, key, applet);
    else if (!g_strcmp0(key, KA_SETTING_KEY_PW_PROMPT_MINS))
        ka_preferences_prompt_mins_notify (settings, key, applet);
}

void
ka_preferences_window_create (KaApplet *applet,
                              GtkBuilder *xml)
{
    prefs.builder = xml;
    ka_preferences_setup_principal_entry (applet);
    ka_preferences_setup_pkuserid_entry (applet);
    ka_preferences_setup_pkuserid_button (applet);
    ka_preferences_setup_smartcard_toggle (applet);
    ka_preferences_setup_pkanchors_entry (applet);
    ka_preferences_setup_pkanchors_button (applet);

    ka_preferences_setup_forwardable_toggle (applet);
    ka_preferences_setup_proxiable_toggle (applet);
    ka_preferences_setup_renewable_toggle (applet);
    ka_preferences_setup_prompt_mins_entry (applet);

    g_signal_connect (ka_applet_get_settings(applet),
                      "changed",
                      G_CALLBACK (ka_preferences_settings_changed),
                      applet);

    g_assert (prefs.n_bindings == N_BINDINGS);

    prefs.notebook = WID (xml, "ka_notebook");
    prefs.dialog = WID (xml, "krb5_preferences_dialog");
}

void
ka_preferences_window_show (KaApplet *applet)
{
    GtkWindow *parent = ka_applet_last_focused_window (applet);

    if (parent)
        gtk_window_set_transient_for (GTK_WINDOW(prefs.dialog),
                                      GTK_WINDOW(parent));
    gtk_window_present (GTK_WINDOW(prefs.dialog));
    gtk_dialog_run (GTK_DIALOG (prefs.dialog));
    gtk_widget_hide (prefs.dialog);
}

void
ka_preferences_window_destroy ()
{
    int i;

    for (i = 0; i < prefs.n_bindings; i++)
        g_object_unref (prefs.bindings[i]);
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
