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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "ka-preferences.h"

#include "ka-gconf-tools.h"
#include "ka-tools.h"

#include <glib/gi18n.h>

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

#define N_LISTENERS 7

struct _KaPreferencesPrivate {
    GtkBuilder *builder;

    GConfClient *client;

    GtkWidget *dialog;
    GtkWidget *notebook;
    GtkWidget *principal_entry;
    GtkWidget *pkuserid_entry;
    GtkWidget *pkuserid_button;
    GtkWidget *smartcard_toggle;
    GtkWidget *pkanchors_entry;
    GtkWidget *pkanchors_button;
    GtkWidget *forwardable_toggle;
    GtkWidget *proxiable_toggle;
    GtkWidget *renewable_toggle;
    GtkWidget *prompt_mins_entry;

    guint     listeners [N_LISTENERS];
    int       n_listeners;
} prefs;


static void
ka_preferences_principal_notify (GConfClient *client G_GNUC_UNUSED,
                                 guint cnx_id G_GNUC_UNUSED,
                                 GConfEntry *entry,
                                 gpointer userdata G_GNUC_UNUSED)
{
    const char *principal;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
      return;

    principal = gconf_value_get_string (entry->value);

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
                                  gpointer userdata G_GNUC_UNUSED)
{
    const char *principal;

    principal = gtk_entry_get_text (entry);

    if (!principal || !strlen(principal))
        gconf_client_unset (prefs.client, KA_GCONF_KEY_PRINCIPAL, NULL);
    else
        gconf_client_set_string (prefs.client, KA_GCONF_KEY_PRINCIPAL, principal, NULL);
}


static void
ka_preferences_setup_principal_entry ()
{
    char     *principal = NULL;

    prefs.principal_entry = WID (prefs.builder, "principal_entry");
    g_assert (prefs.principal_entry != NULL);

    if (!ka_gconf_get_string (prefs.client, KA_GCONF_KEY_PRINCIPAL, &principal))
        g_warning ("Getting principal failed");

    if (principal && strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (prefs.principal_entry), principal);
    if (principal)
        g_free (principal);

    g_signal_connect (prefs.principal_entry, "changed",
                      G_CALLBACK (ka_preferences_principal_changed), NULL);

    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_PRINCIPAL, NULL)) {
        gtk_widget_set_sensitive (prefs.principal_entry, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_PRINCIPAL,
                                 (GConfClientNotifyFunc) ka_preferences_principal_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
}


static void
ka_preferences_pkuserid_notify (GConfClient *client G_GNUC_UNUSED,
                                guint cnx_id G_GNUC_UNUSED,
                                GConfEntry *entry,
                                gpointer *userdata G_GNUC_UNUSED)
{
    const char *pkuserid;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
        return;

    pkuserid = gconf_value_get_string (entry->value);

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
                                 gpointer *user_data G_GNUC_UNUSED)
{
    const char *pkuserid;

    pkuserid = gtk_entry_get_text (entry);

    if (!pkuserid || !strlen(pkuserid))
        gconf_client_unset (prefs.client, KA_GCONF_KEY_PK_USERID, NULL);
    else
        gconf_client_set_string (prefs.client, KA_GCONF_KEY_PK_USERID, pkuserid, NULL);
}


static void
ka_preferences_setup_pkuserid_entry ()
{
    char     *pkuserid = NULL;

    prefs.pkuserid_entry = WID(prefs.builder, "pkuserid_entry");
    g_assert (prefs.pkuserid_entry != NULL);

    if (!ka_gconf_get_string (prefs.client, KA_GCONF_KEY_PK_USERID, &pkuserid))
        g_warning ("Getting pkuserid failed");

    if (pkuserid && strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkuserid_entry), pkuserid);
    if (pkuserid)
        g_free (pkuserid);

    g_signal_connect (prefs.pkuserid_entry, "changed",
                      G_CALLBACK (ka_preferences_pkuserid_changed), NULL);
    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_PK_USERID, NULL)) {
        gtk_widget_set_sensitive (prefs.pkuserid_entry, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_PK_USERID,
                                 (GConfClientNotifyFunc) ka_preferences_pkuserid_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
}


static void
ka_preferences_pkanchors_notify (GConfClient *client G_GNUC_UNUSED,
                                 guint cnx_id G_GNUC_UNUSED,
                                 GConfEntry *entry,
                                 gpointer userdata G_GNUC_UNUSED)
{
    const char *pkanchors;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
        return;

    pkanchors = gconf_value_get_string (entry->value);

    if (!pkanchors || !strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), "");
    else {
        const char *old_pkanchors;

        old_pkanchors = gtk_entry_get_text (GTK_ENTRY (prefs.pkanchors_entry));
        if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors, pkanchors)))
            gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), pkanchors);
    }
}


static void
ka_preferences_pkanchors_changed (GtkEntry *entry,
                                  gpointer userdata G_GNUC_UNUSED)
{
    const char *pkanchors;

    pkanchors = gtk_entry_get_text (entry);

    if (!pkanchors || !strlen(pkanchors))
        gconf_client_unset (prefs.client, KA_GCONF_KEY_PK_ANCHORS, NULL);
    else
        gconf_client_set_string (prefs.client, KA_GCONF_KEY_PK_ANCHORS,
                                 pkanchors, NULL);
}


static void
ka_preferences_setup_pkanchors_entry ()
{
    char *pkanchors = NULL;

    prefs.pkanchors_entry = WID(prefs.builder, "pkanchors_entry");
    g_assert (prefs.pkanchors_entry != NULL);

    if (!ka_gconf_get_string (prefs.client, KA_GCONF_KEY_PK_ANCHORS, &pkanchors))
        g_warning ("Getting pkanchors failed");

    if (pkanchors && strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (prefs.pkanchors_entry), pkanchors);
    if (pkanchors)
        g_free (pkanchors);

    g_signal_connect (prefs.pkanchors_entry, "changed",
                      G_CALLBACK (ka_preferences_pkanchors_changed), NULL);
    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_PK_ANCHORS, NULL)) {
        gtk_widget_set_sensitive (prefs.pkanchors_entry, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_PK_ANCHORS,
                                 (GConfClientNotifyFunc) ka_preferences_pkanchors_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
}


static void
ka_preferences_toggle_pkuserid_entry (gboolean state, gpointer userdata G_GNUC_UNUSED)
{
    gtk_widget_set_sensitive (prefs.pkuserid_entry, state);
    gtk_widget_set_sensitive (prefs.pkuserid_button, state);
}


static void
ka_preferences_smartcard_toggled (GtkToggleButton *toggle,
                                  gpointer userdata G_GNUC_UNUSED)
{
    gboolean smartcard = gtk_toggle_button_get_active (toggle);
    static gchar *old_path = NULL;

    if (smartcard) {
        const char *path;

        path = gtk_entry_get_text (GTK_ENTRY(prefs.pkuserid_entry));
        if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
            g_free (old_path);
            old_path = g_strdup (path);
        }
        ka_preferences_toggle_pkuserid_entry (FALSE, NULL);
        gconf_client_set_string (prefs.client, KA_GCONF_KEY_PK_USERID, PKINIT_SMARTCARD, NULL);
    } else {
        ka_preferences_toggle_pkuserid_entry (TRUE, NULL);
        if (old_path)
            gconf_client_set_string (prefs.client, KA_GCONF_KEY_PK_USERID, old_path, NULL);
        else
            gconf_client_unset (prefs.client, KA_GCONF_KEY_PK_USERID, NULL);
    }
}


static void
ka_preferences_setup_smartcard_toggle ()
{
    char *pkuserid = NULL;

    prefs.smartcard_toggle = WID (prefs.builder, "smartcard_toggle");
    g_assert (prefs.smartcard_toggle != NULL);

    if (!ka_gconf_get_string (prefs.client, KA_GCONF_KEY_PK_USERID, &pkuserid))
        g_warning ("Getting pkanchors failed");

    g_signal_connect (prefs.smartcard_toggle, "toggled",
                      G_CALLBACK (ka_preferences_smartcard_toggled), NULL);

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
ka_preferences_forwardable_toggled (GtkToggleButton *toggle,
                                    gpointer userdata G_GNUC_UNUSED)
{
    gboolean forwardable;

    forwardable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (prefs.client, KA_GCONF_KEY_FORWARDABLE, forwardable, NULL);
}


static void
ka_preferences_forwardable_notify (GConfClient *client G_GNUC_UNUSED,
                                   guint cnx_id G_GNUC_UNUSED,
                                   GConfEntry *entry,
                                   gpointer userdata G_GNUC_UNUSED)
{
    gboolean forwardable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

    forwardable = gconf_value_get_bool (entry->value) != FALSE;

    if (forwardable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs.forwardable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.forwardable_toggle), forwardable);
}


static gboolean
ka_preferences_setup_forwardable_toggle ()
{
    gboolean forwardable;

    prefs.forwardable_toggle = WID (prefs.builder, "forwardable_toggle");
    g_assert (prefs.forwardable_toggle != NULL);

    forwardable = gconf_client_get_bool (prefs.client, KA_GCONF_KEY_FORWARDABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.forwardable_toggle), forwardable);

    g_signal_connect (prefs.forwardable_toggle, "toggled",
                      G_CALLBACK (ka_preferences_forwardable_toggled), NULL);

    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_FORWARDABLE, NULL)) {
        gtk_widget_set_sensitive (prefs.forwardable_toggle, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_FORWARDABLE,
                                 (GConfClientNotifyFunc) ka_preferences_forwardable_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
    return forwardable;
}


static void
ka_preferences_proxiable_toggled (GtkToggleButton *toggle,
                                  gpointer userdata G_GNUC_UNUSED)
{
    gboolean proxiable;

    proxiable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (prefs.client, KA_GCONF_KEY_PROXIABLE, proxiable, NULL);
}


static void
ka_preferences_proxiable_notify (GConfClient *client G_GNUC_UNUSED,
                                 guint cnx_id G_GNUC_UNUSED,
                                 GConfEntry *entry,
                                 gpointer userdata G_GNUC_UNUSED)
{
    gboolean proxiable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
        return;

    proxiable = gconf_value_get_bool (entry->value) != FALSE;

    if (proxiable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs.proxiable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.proxiable_toggle), proxiable);
}


static gboolean
ka_preferences_setup_proxiable_toggle ()
{
    gboolean proxiable;

    prefs.proxiable_toggle = WID (prefs.builder, "proxiable_toggle");
    g_assert (prefs.proxiable_toggle != NULL);

    proxiable = gconf_client_get_bool (prefs.client, KA_GCONF_KEY_PROXIABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.proxiable_toggle), proxiable);

    g_signal_connect (prefs.proxiable_toggle, "toggled",
                      G_CALLBACK (ka_preferences_proxiable_toggled), NULL);

    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_PROXIABLE, NULL)) {
        gtk_widget_set_sensitive (prefs.proxiable_toggle, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_PROXIABLE,
                                 (GConfClientNotifyFunc) ka_preferences_proxiable_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
    return proxiable;
}


static void
ka_preferences_renewable_toggled (GtkToggleButton *toggle,
                                  gpointer userdata G_GNUC_UNUSED)
{
    gboolean renewable;

    renewable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (prefs.client, KA_GCONF_KEY_RENEWABLE, renewable, NULL);
}


static void
ka_preferences_renewable_notify (GConfClient *client G_GNUC_UNUSED,
                                 guint cnx_id G_GNUC_UNUSED,
                                 GConfEntry *entry,
                                 gpointer userdata G_GNUC_UNUSED)
{
    gboolean renewable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
        return;

    renewable = gconf_value_get_bool (entry->value) != FALSE;

    if (renewable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prefs.renewable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.renewable_toggle), renewable);
}


static gboolean
ka_preferences_setup_renewable_toggle ()
{
    gboolean renewable;

    prefs.renewable_toggle = WID (prefs.builder, "renewable_toggle");
    g_assert (prefs.renewable_toggle != NULL);

    renewable = gconf_client_get_bool (prefs.client, KA_GCONF_KEY_RENEWABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prefs.renewable_toggle), renewable);

    g_signal_connect (prefs.renewable_toggle, "toggled",
                      G_CALLBACK (ka_preferences_renewable_toggled), NULL);

    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_RENEWABLE, NULL)) {
        gtk_widget_set_sensitive (prefs.renewable_toggle, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_RENEWABLE,
                                 (GConfClientNotifyFunc) ka_preferences_renewable_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
    return renewable;
}


static void
ka_preferences_prompt_mins_changed (GtkSpinButton *button,
                                    gpointer userdata G_GNUC_UNUSED)
{
    gint prompt_mins;

    prompt_mins = gtk_spin_button_get_value_as_int (button);
    gconf_client_set_int (prefs.client, KA_GCONF_KEY_PROMPT_MINS, prompt_mins, NULL);
}


static void
ka_preferences_prompt_mins_notify (GConfClient *client G_GNUC_UNUSED,
                                   guint cnx_id G_GNUC_UNUSED,
                                   GConfEntry *entry,
                                   gpointer userdata G_GNUC_UNUSED)
{
    gint prompt_mins;

    if (!entry->value || entry->value->type != GCONF_VALUE_INT)
        return;

    prompt_mins = gconf_value_get_int (entry->value);

    if (prompt_mins != gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (prefs.prompt_mins_entry)))
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs.prompt_mins_entry), prompt_mins);
}


static gint
ka_preferences_setup_prompt_mins_entry ()
{
    gint prompt_mins;

    prefs.prompt_mins_entry = WID (prefs.builder, "prompt_mins_entry");
    g_assert (prefs.prompt_mins_entry != NULL);

    prompt_mins = gconf_client_get_int (prefs.client, KA_GCONF_KEY_PROMPT_MINS, NULL);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (prefs.prompt_mins_entry), prompt_mins);

    g_signal_connect (prefs.prompt_mins_entry, "value-changed",
                      G_CALLBACK (ka_preferences_prompt_mins_changed), NULL);

    if (!gconf_client_key_is_writable (prefs.client, KA_GCONF_KEY_PROMPT_MINS, NULL)) {
        gtk_widget_set_sensitive (prefs.prompt_mins_entry, FALSE);
    }

    prefs.listeners [prefs.n_listeners] =
        gconf_client_notify_add (prefs.client,
                                 KA_GCONF_KEY_PROMPT_MINS,
                                 (GConfClientNotifyFunc) ka_preferences_prompt_mins_notify,
                                 NULL, NULL, NULL);
    prefs.n_listeners++;
    return prompt_mins;
}


void
ka_preferences_window_create (KaApplet *applet G_GNUC_UNUSED,
                              GtkBuilder *xml)
{
    prefs.client = gconf_client_get_default ();
    gconf_client_add_dir (prefs.client, KA_GCONF_PATH,
                          GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    prefs.builder = xml;
    ka_preferences_setup_principal_entry (prefs);
    ka_preferences_setup_pkuserid_entry (prefs);
    ka_preferences_setup_pkuserid_button (prefs);
    ka_preferences_setup_smartcard_toggle (prefs);
    ka_preferences_setup_pkanchors_entry(prefs);
    ka_preferences_setup_pkanchors_button (prefs);
    ka_preferences_setup_forwardable_toggle (prefs);
    ka_preferences_setup_proxiable_toggle (prefs);
    ka_preferences_setup_renewable_toggle (prefs);
    ka_preferences_setup_prompt_mins_entry (prefs);

    g_assert (prefs.n_listeners == N_LISTENERS);

    prefs.notebook = WID (xml, "ka_notebook");
    prefs.dialog = WID (xml, "krb5_preferences_dialog");

}

void
ka_preferences_window_show (GtkWindow *main_window)
{
    if (main_window)
        gtk_window_set_transient_for (GTK_WINDOW(prefs.dialog), main_window);
    gtk_window_present (GTK_WINDOW(prefs.dialog));
    gtk_dialog_run (GTK_DIALOG (prefs.dialog));
    gtk_widget_hide (prefs.dialog);
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
