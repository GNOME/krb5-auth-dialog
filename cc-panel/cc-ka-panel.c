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
#include "cc-ka-panel.h"

#include "ka-gconf-tools.h"
#include "ka-tools.h"

#include <glib/gi18n.h>

G_DEFINE_DYNAMIC_TYPE (CcKaPanel, cc_ka_panel, CC_TYPE_PANEL)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_KA_PANEL, CcKaPanelPrivate))

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

#define N_LISTENERS 7

typedef struct _CcKaPanelPrivate CcKaPanelPrivate;

struct _CcKaPanelPrivate {
    GtkBuilder *builder;

    GConfClient *client;

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
};

static void
cc_ka_panel_principal_notify (GConfClient *client G_GNUC_UNUSED,
                              guint cnx_id G_GNUC_UNUSED,
                              GConfEntry *entry,
                              CcKaPanelPrivate *priv)
{
    const char *principal;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
      return;

    principal = gconf_value_get_string (entry->value);

    if (!principal || !strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (priv->principal_entry), "");
    else {
        const char *old_principal;

        old_principal = gtk_entry_get_text (GTK_ENTRY (priv->principal_entry));
        if (!old_principal || (old_principal && strcmp (old_principal, principal)))
            gtk_entry_set_text (GTK_ENTRY (priv->principal_entry), principal);
    }
}

static void
cc_ka_panel_principal_changed (GtkEntry *entry,
                               CcKaPanelPrivate *priv)
{
    const char *principal;

    principal = gtk_entry_get_text (entry);

    if (!principal || !strlen(principal))
        gconf_client_unset (priv->client, KA_GCONF_KEY_PRINCIPAL, NULL);
    else
        gconf_client_set_string (priv->client, KA_GCONF_KEY_PRINCIPAL, principal, NULL);
}


static void
cc_ka_panel_setup_principal_entry (CcKaPanelPrivate *priv)
{
    char     *principal = NULL;

    priv->principal_entry = WID (priv->builder, "principal_entry");
    g_assert (priv->principal_entry != NULL);

    if (!ka_gconf_get_string (priv->client, KA_GCONF_KEY_PRINCIPAL, &principal))
        g_warning ("Getting principal failed");

    if (principal && strlen(principal))
        gtk_entry_set_text (GTK_ENTRY (priv->principal_entry), principal);
    if (principal)
        g_free (principal);

    g_signal_connect (priv->principal_entry, "changed",
                      G_CALLBACK (cc_ka_panel_principal_changed), priv);

    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_PRINCIPAL, NULL)) {
        gtk_widget_set_sensitive (priv->principal_entry, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_PRINCIPAL,
                                 (GConfClientNotifyFunc) cc_ka_panel_principal_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
}


static void
cc_ka_panel_pkuserid_notify (GConfClient *client G_GNUC_UNUSED,
                             guint cnx_id G_GNUC_UNUSED,
                             GConfEntry *entry,
                             CcKaPanelPrivate *priv)
{
    const char *pkuserid;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
        return;

    pkuserid = gconf_value_get_string (entry->value);

    if (!pkuserid || !strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (priv->pkuserid_entry), "");
    else {
        const char *old_pkuserid;

        old_pkuserid = gtk_entry_get_text (GTK_ENTRY (priv->pkuserid_entry));
        if (!old_pkuserid || (old_pkuserid && strcmp (old_pkuserid, pkuserid)))
            gtk_entry_set_text (GTK_ENTRY (priv->pkuserid_entry), pkuserid);
    }
}


static void
cc_ka_panel_pkuserid_changed (GtkEntry *entry,
                              CcKaPanelPrivate *priv)
{
    const char *pkuserid;

    pkuserid = gtk_entry_get_text (entry);

    if (!pkuserid || !strlen(pkuserid))
        gconf_client_unset (priv->client, KA_GCONF_KEY_PK_USERID, NULL);
    else
        gconf_client_set_string (priv->client, KA_GCONF_KEY_PK_USERID, pkuserid, NULL);
}


static void
cc_ka_panel_setup_pkuserid_entry (CcKaPanelPrivate *priv)
{
    char     *pkuserid = NULL;

    priv->pkuserid_entry = WID(priv->builder, "pkuserid_entry");
    g_assert (priv->pkuserid_entry != NULL);

    if (!ka_gconf_get_string (priv->client, KA_GCONF_KEY_PK_USERID, &pkuserid))
        g_warning ("Getting pkuserid failed");

    if (pkuserid && strlen(pkuserid))
        gtk_entry_set_text (GTK_ENTRY (priv->pkuserid_entry), pkuserid);
    if (pkuserid)
        g_free (pkuserid);

    g_signal_connect (priv->pkuserid_entry, "changed",
                      G_CALLBACK (cc_ka_panel_pkuserid_changed), priv);
    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_PK_USERID, NULL)) {
        gtk_widget_set_sensitive (priv->pkuserid_entry, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_PK_USERID,
                                 (GConfClientNotifyFunc) cc_ka_panel_pkuserid_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
}


static void
ka_preferences_pkanchors_notify (GConfClient *client G_GNUC_UNUSED,
                                 guint cnx_id G_GNUC_UNUSED,
                                 GConfEntry *entry,
                                 CcKaPanelPrivate *priv)
{
    const char *pkanchors;

    if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
        return;

    pkanchors = gconf_value_get_string (entry->value);

    if (!pkanchors || !strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (priv->pkanchors_entry), "");
    else {
        const char *old_pkanchors;

        old_pkanchors = gtk_entry_get_text (GTK_ENTRY (priv->pkanchors_entry));
        if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors, pkanchors)))
            gtk_entry_set_text (GTK_ENTRY (priv->pkanchors_entry), pkanchors);
    }
}


static void
cc_ka_panel_pkanchors_changed (GtkEntry *entry,
                               CcKaPanelPrivate *priv)
{
    const char *pkanchors;

    pkanchors = gtk_entry_get_text (entry);

    if (!pkanchors || !strlen(pkanchors))
        gconf_client_unset (priv->client, KA_GCONF_KEY_PK_ANCHORS, NULL);
    else
        gconf_client_set_string (priv->client, KA_GCONF_KEY_PK_ANCHORS,
                                 pkanchors, NULL);
}


static void
cc_ka_panel_setup_pkanchors_entry (CcKaPanelPrivate *priv)
{
    char *pkanchors = NULL;

    priv->pkanchors_entry = WID(priv->builder, "pkanchors_entry");
    g_assert (priv->pkanchors_entry != NULL);

    if (!ka_gconf_get_string (priv->client, KA_GCONF_KEY_PK_ANCHORS, &pkanchors))
        g_warning ("Getting pkanchors failed");

    if (pkanchors && strlen(pkanchors))
        gtk_entry_set_text (GTK_ENTRY (priv->pkanchors_entry), pkanchors);
    if (pkanchors)
        g_free (pkanchors);

    g_signal_connect (priv->pkanchors_entry, "changed",
                      G_CALLBACK (cc_ka_panel_pkanchors_changed), priv);
    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_PK_ANCHORS, NULL)) {
        gtk_widget_set_sensitive (priv->pkanchors_entry, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_PK_ANCHORS,
                                 (GConfClientNotifyFunc) ka_preferences_pkanchors_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
}


static void
ka_preferences_toggle_pkuserid_entry (gboolean state, CcKaPanelPrivate *priv)
{
    gtk_widget_set_sensitive (priv->pkuserid_entry, state);
    gtk_widget_set_sensitive (priv->pkuserid_button, state);
}


static void
cc_ka_panel_smartcard_toggled (GtkToggleButton *toggle,
                               CcKaPanelPrivate *priv)
{
    gboolean smartcard = gtk_toggle_button_get_active (toggle);
    static gchar *old_path = NULL;

    if (smartcard) {
        const char *path;

        path = gtk_entry_get_text (GTK_ENTRY(priv->pkuserid_entry));
        if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
            g_free (old_path);
            old_path = g_strdup (path);
        }
        ka_preferences_toggle_pkuserid_entry (FALSE, priv);
        gconf_client_set_string (priv->client, KA_GCONF_KEY_PK_USERID, PKINIT_SMARTCARD, NULL);
    } else {
        ka_preferences_toggle_pkuserid_entry (TRUE, priv);
        if (old_path)
            gconf_client_set_string (priv->client, KA_GCONF_KEY_PK_USERID, old_path, NULL);
        else
            gconf_client_unset (priv->client, KA_GCONF_KEY_PK_USERID, NULL);
    }
}


static void
cc_ka_panel_setup_smartcard_toggle(CcKaPanelPrivate *priv)
{
    char *pkuserid = NULL;

    priv->smartcard_toggle = WID (priv->builder, "smartcard_toggle");
    g_assert (priv->smartcard_toggle != NULL);

    if (!ka_gconf_get_string (priv->client, KA_GCONF_KEY_PK_USERID, &pkuserid))
        g_warning ("Getting pkanchors failed");

    g_signal_connect (priv->smartcard_toggle, "toggled",
                      G_CALLBACK (cc_ka_panel_smartcard_toggled), priv);

    if (!g_strcmp0 (pkuserid, PKINIT_SMARTCARD))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->smartcard_toggle), TRUE);
    else
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->smartcard_toggle), FALSE);

    if (pkuserid)
        g_free (pkuserid);
}


static void
cc_ka_panel_browse_certs (CcKaPanelPrivate *priv, GtkEntry *entry)
{
    GtkWidget *filechooser;
    GtkFileFilter *cert_filter, *all_filter;
    gchar *filename = NULL;
    const gchar *current;
    gint ret;

    filechooser = gtk_file_chooser_dialog_new(_("Choose Certificate"),
                                              GTK_WINDOW (gtk_widget_get_toplevel (priv->notebook)),
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
cc_ka_panel_browse_pkuserids (GtkButton *button G_GNUC_UNUSED,
                                       CcKaPanelPrivate *priv)
{
    cc_ka_panel_browse_certs (priv,
                              GTK_ENTRY(priv->pkuserid_entry));
}

static void
cc_ka_panel_browse_pkanchors(GtkButton *button G_GNUC_UNUSED,
                                       CcKaPanelPrivate *priv)
{
    cc_ka_panel_browse_certs (priv,
                              GTK_ENTRY(priv->pkanchors_entry));
}

static void
cc_ka_panel_setup_pkuserid_button (CcKaPanelPrivate *priv)
{
    priv->pkuserid_button = WID (priv->builder, "pkuserid_button");
    g_assert (priv->pkuserid_button != NULL);

    g_signal_connect (priv->pkuserid_button, "clicked",
                      G_CALLBACK (cc_ka_panel_browse_pkuserids), priv);

}

static void
cc_ka_panel_setup_pkanchors_button (CcKaPanelPrivate *priv)
{
    priv->pkanchors_button = WID (priv->builder, "pkanchors_button");
    g_assert (priv->pkanchors_button != NULL);

    g_signal_connect (priv->pkanchors_button, "clicked",
                      G_CALLBACK (cc_ka_panel_browse_pkanchors), priv);

}


static void
cc_ka_panel_forwardable_toggled (GtkToggleButton *toggle,
                                           CcKaPanelPrivate *priv)
{
    gboolean forwardable;

    forwardable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (priv->client, KA_GCONF_KEY_FORWARDABLE, forwardable, NULL);
}


static void
cc_ka_panel_forwardable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    CcKaPanelPrivate *priv)
{
    gboolean forwardable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

    forwardable = gconf_value_get_bool (entry->value) != FALSE;

    if (forwardable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->forwardable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->forwardable_toggle), forwardable);
}


static gboolean
cc_ka_panel_setup_forwardable_toggle (CcKaPanelPrivate *priv)
{
    gboolean forwardable;

    priv->forwardable_toggle = WID (priv->builder, "forwardable_toggle");
    g_assert (priv->forwardable_toggle != NULL);

    forwardable = gconf_client_get_bool (priv->client, KA_GCONF_KEY_FORWARDABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->forwardable_toggle), forwardable);

    g_signal_connect (priv->forwardable_toggle, "toggled",
                      G_CALLBACK (cc_ka_panel_forwardable_toggled), priv);

    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_FORWARDABLE, NULL)) {
        gtk_widget_set_sensitive (priv->forwardable_toggle, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_FORWARDABLE,
                                 (GConfClientNotifyFunc) cc_ka_panel_forwardable_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
    return forwardable;
}


static void
cc_ka_panel_proxiable_toggled (GtkToggleButton *toggle,
                                     CcKaPanelPrivate *priv)
{
    gboolean proxiable;

    proxiable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (priv->client, KA_GCONF_KEY_PROXIABLE, proxiable, NULL);
}


static void
cc_ka_panel_proxiable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    CcKaPanelPrivate *priv)
{
    gboolean proxiable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
        return;

    proxiable = gconf_value_get_bool (entry->value) != FALSE;

    if (proxiable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->proxiable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxiable_toggle), proxiable);
}


static gboolean
cc_ka_panel_setup_proxiable_toggle (CcKaPanelPrivate *priv)
{
    gboolean proxiable;

    priv->proxiable_toggle = WID (priv->builder, "proxiable_toggle");
    g_assert (priv->proxiable_toggle != NULL);

    proxiable = gconf_client_get_bool (priv->client, KA_GCONF_KEY_PROXIABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->proxiable_toggle), proxiable);

    g_signal_connect (priv->proxiable_toggle, "toggled",
                      G_CALLBACK (cc_ka_panel_proxiable_toggled), priv);

    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_PROXIABLE, NULL)) {
        gtk_widget_set_sensitive (priv->proxiable_toggle, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_PROXIABLE,
                                 (GConfClientNotifyFunc) cc_ka_panel_proxiable_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
    return proxiable;
}


static void
cc_ka_panel_renewable_toggled (GtkToggleButton *toggle,
                               CcKaPanelPrivate *priv)
{
    gboolean renewable;

    renewable = gtk_toggle_button_get_active (toggle);

    gconf_client_set_bool (priv->client, KA_GCONF_KEY_RENEWABLE, renewable, NULL);
}


static void
cc_ka_panel_renewable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    CcKaPanelPrivate *priv)
{
    gboolean renewable;

    if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
        return;

    renewable = gconf_value_get_bool (entry->value) != FALSE;

    if (renewable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->renewable_toggle)))
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->renewable_toggle), renewable);
}


static gboolean
cc_ka_panel_setup_renewable_toggle (CcKaPanelPrivate *priv)
{
    gboolean renewable;

    priv->renewable_toggle = WID (priv->builder, "renewable_toggle");
    g_assert (priv->renewable_toggle != NULL);

    renewable = gconf_client_get_bool (priv->client, KA_GCONF_KEY_RENEWABLE, NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->renewable_toggle), renewable);

    g_signal_connect (priv->renewable_toggle, "toggled",
                      G_CALLBACK (cc_ka_panel_renewable_toggled), priv);

    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_RENEWABLE, NULL)) {
        gtk_widget_set_sensitive (priv->renewable_toggle, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_RENEWABLE,
                                 (GConfClientNotifyFunc) cc_ka_panel_renewable_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
    return renewable;
}


static void
cc_ka_panel_prompt_mins_changed (GtkSpinButton *button,
                                     CcKaPanelPrivate *priv)
{
    gint prompt_mins;

    prompt_mins = gtk_spin_button_get_value_as_int (button);
    gconf_client_set_int (priv->client, KA_GCONF_KEY_PROMPT_MINS, prompt_mins, NULL);
}


static void
cc_ka_panel_prompt_mins_notify (GConfClient *client G_GNUC_UNUSED,
                                guint cnx_id G_GNUC_UNUSED,
                                GConfEntry *entry,
                                CcKaPanelPrivate *priv)
{
    gint prompt_mins;

    if (!entry->value || entry->value->type != GCONF_VALUE_INT)
        return;

    prompt_mins = gconf_value_get_int (entry->value);

    if (prompt_mins != gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->prompt_mins_entry)))
        gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->prompt_mins_entry), prompt_mins);
}


static gint
cc_ka_panel_setup_prompt_mins_entry (CcKaPanelPrivate *priv)
{
    gint prompt_mins;

    priv->prompt_mins_entry = WID (priv->builder, "prompt_mins_entry");
    g_assert (priv->prompt_mins_entry != NULL);

    prompt_mins = gconf_client_get_int (priv->client, KA_GCONF_KEY_PROMPT_MINS, NULL);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->prompt_mins_entry), prompt_mins);

    g_signal_connect (priv->prompt_mins_entry, "value-changed",
                      G_CALLBACK (cc_ka_panel_prompt_mins_changed), priv);

    if (!gconf_client_key_is_writable (priv->client, KA_GCONF_KEY_PROMPT_MINS, NULL)) {
        gtk_widget_set_sensitive (priv->prompt_mins_entry, FALSE);
    }

    priv->listeners [priv->n_listeners] =
        gconf_client_notify_add (priv->client,
                                 KA_GCONF_KEY_PROMPT_MINS,
                                 (GConfClientNotifyFunc) cc_ka_panel_prompt_mins_notify,
                                 priv, NULL, NULL);
    priv->n_listeners++;
    return prompt_mins;
}

static void
cc_ka_panel_get_property (GObject    *self,
                          guint       property_id,
                          GValue     *value G_GNUC_UNUSED,
                          GParamSpec *pspec)
{
    switch (property_id) {
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    }
}

static void
cc_ka_panel_set_property (GObject      *self,
                          guint         property_id,
                          const GValue *value G_GNUC_UNUSED,
                          GParamSpec   *pspec)
{
    switch (property_id) {
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    }
}

static void
cc_ka_panel_dispose (GObject *self)
{
    CcKaPanelPrivate *priv = GET_PRIVATE (self);

    if (priv->builder != NULL) {
        g_object_unref (priv->builder);
        priv->builder = NULL;
    }
    G_OBJECT_CLASS (cc_ka_panel_parent_class)->dispose (self);
}

static void
cc_ka_panel_finalize (GObject *self)
{
    CcKaPanelPrivate *priv = GET_PRIVATE (self);

    if (priv->client) {
        int i;

        for (i = 0; i < priv->n_listeners; i++) {
            if (priv->listeners [i])
                gconf_client_notify_remove (priv->client, priv->listeners [i]);
            priv->listeners [i] = 0;
        }
        priv->n_listeners = 0;

      gconf_client_remove_dir (priv->client, KA_GCONF_PATH, NULL);

      g_object_unref (priv->client);
      priv->client = NULL;
  }

    G_OBJECT_CLASS (cc_ka_panel_parent_class)->finalize (self);
}

static void
cc_ka_panel_class_init (CcKaPanelClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (CcKaPanelPrivate));

    object_class->get_property = cc_ka_panel_get_property;
    object_class->set_property = cc_ka_panel_set_property;
    object_class->dispose = cc_ka_panel_dispose;
    object_class->finalize = cc_ka_panel_finalize;
}

static void
cc_ka_panel_class_finalize (CcKaPanelClass *klass G_GNUC_UNUSED)
{
}

static void
cc_ka_panel_init (CcKaPanel *self)
{
    GError *error = NULL;

    CcKaPanelPrivate *priv = GET_PRIVATE (self);

    priv->builder = gtk_builder_new ();
    gtk_builder_add_from_file(priv->builder, KA_DATA_DIR G_DIR_SEPARATOR_S
                              "ka-panel.ui", &error);
    if (error != NULL) {
        g_warning ("Could not load interface file: %s", error->message);
        g_error_free (error);
        return;

    }
    priv->client = gconf_client_get_default ();
    gconf_client_add_dir (priv->client, KA_GCONF_PATH,
                          GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    cc_ka_panel_setup_principal_entry (priv);
    cc_ka_panel_setup_pkuserid_entry (priv);
    cc_ka_panel_setup_pkuserid_button (priv);
    cc_ka_panel_setup_smartcard_toggle (priv);
    cc_ka_panel_setup_pkanchors_entry(priv);
    cc_ka_panel_setup_pkanchors_button (priv);
    cc_ka_panel_setup_forwardable_toggle (priv);
    cc_ka_panel_setup_proxiable_toggle (priv);
    cc_ka_panel_setup_renewable_toggle (priv);
    cc_ka_panel_setup_prompt_mins_entry (priv);

    g_assert (priv->n_listeners == N_LISTENERS);

    priv->notebook = WID (priv->builder, "ka_notebook");
    gtk_widget_reparent (priv->notebook, (GtkWidget *) self);

    gtk_widget_show (priv->notebook);
}

void
cc_ka_panel_register (GIOModule *module)
{
    cc_ka_panel_register_type (G_TYPE_MODULE (module));
    g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                    CC_TYPE_KA_PANEL,
                                    "ka-panel", 0);
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
