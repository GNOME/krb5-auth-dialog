/*
 * Copyright (C) 2009 Guido Guenther <agx@sigxcup.org>
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
 * Author:
 * 	Guido Guenther <agx@sigxcpu.org>
 *
 * based on the vino capplet which is:
 *
 * 	Copyright (C) 2003 Sun Microsystems, Inc.
 * 	Copyright (C) 2006 Jonh Wendell <wendell@bani.com.br> 
 *
 * 	Authors:
 *      	Mark McLoughlin <mark@skynet.ie>
 *      	Jonh Wendell <wendell@bani.com.br>
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "krb5-auth-gconf-tools.h"
#include "krb5-auth-tools.h"

#define PKINIT_SMARTCARD "PKCS11:" SC_PKCS11
#define PKINIT_FILE "FILE:"

#define N_LISTENERS 8

typedef struct {
  GtkBuilder  *xml;
  GConfClient *client;

  GtkWidget *dialog;
  GtkWidget *principal_entry;
  GtkWidget *pkuserid_entry;
  GtkWidget *pkuserid_button;
  GtkWidget *smartcard_toggle;
  GtkWidget *pkanchors_entry;
  GtkWidget *pkanchors_button;
  GtkWidget *forwardable_toggle;
  GtkWidget *proxiable_toggle;
  GtkWidget *renewable_toggle;
  GtkWidget *trayicon_toggle;
  GtkWidget *prompt_mins_entry;

  guint     listeners [N_LISTENERS];
  int       n_listeners;
} KaPreferencesDialog;


static void
ka_preferences_principal_notify (GConfClient *client G_GNUC_UNUSED,
                           guint cnx_id G_GNUC_UNUSED,
                           GConfEntry *entry,
                           KaPreferencesDialog *dialog)
{
  const char *principal;

  if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
    return;

  principal = gconf_value_get_string (entry->value);

  if (!principal || !strlen(principal))
      gtk_entry_set_text (GTK_ENTRY (dialog->principal_entry), "");
  else {
      const char *old_principal;

      old_principal = gtk_entry_get_text (GTK_ENTRY (dialog->principal_entry));
      if (!old_principal || (old_principal && strcmp (old_principal, principal)))
          gtk_entry_set_text (GTK_ENTRY (dialog->principal_entry), principal);
  }
}


static void
ka_preferences_dialog_principal_changed (GtkEntry *entry,
                                   KaPreferencesDialog *dialog)
{
  const char *principal;

  principal = gtk_entry_get_text (entry);

  if (!principal || !strlen(principal))
      gconf_client_unset (dialog->client, KA_GCONF_KEY_PRINCIPAL, NULL);
  else
      gconf_client_set_string (dialog->client, KA_GCONF_KEY_PRINCIPAL, principal, NULL);
}


static void
ka_preferences_dialog_setup_principal_entry (KaPreferencesDialog *dialog)
{
  char     *principal = NULL;

  dialog->principal_entry = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "principal_entry"));
  g_assert (dialog->principal_entry != NULL);

  if (!ka_gconf_get_string (dialog->client, KA_GCONF_KEY_PRINCIPAL, &principal))
      g_warning ("Getting principal failed");

  if (principal && strlen(principal))
      gtk_entry_set_text (GTK_ENTRY (dialog->principal_entry), principal);
  if (principal)
      g_free (principal);

  g_signal_connect (dialog->principal_entry, "changed",
              G_CALLBACK (ka_preferences_dialog_principal_changed), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_PRINCIPAL, NULL)) {
      gtk_widget_set_sensitive (dialog->principal_entry, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                          KA_GCONF_KEY_PRINCIPAL,
                          (GConfClientNotifyFunc) ka_preferences_principal_notify,
                          dialog, NULL, NULL);
  dialog->n_listeners++;
}


static void
ka_preferences_pkuserid_notify (GConfClient *client G_GNUC_UNUSED,
                           guint cnx_id G_GNUC_UNUSED,
                           GConfEntry *entry,
                           KaPreferencesDialog *dialog)
{
  const char *pkuserid;

  if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
      return;

  pkuserid = gconf_value_get_string (entry->value);

  if (!pkuserid || !strlen(pkuserid))
      gtk_entry_set_text (GTK_ENTRY (dialog->pkuserid_entry), "");
  else {
      const char *old_pkuserid;

      old_pkuserid = gtk_entry_get_text (GTK_ENTRY (dialog->pkuserid_entry));
      if (!old_pkuserid || (old_pkuserid && strcmp (old_pkuserid, pkuserid)))
          gtk_entry_set_text (GTK_ENTRY (dialog->pkuserid_entry), pkuserid);
  }
}


static void
ka_preferences_dialog_pkuserid_changed (GtkEntry *entry,
                                   KaPreferencesDialog *dialog)
{
  const char *pkuserid;

  pkuserid = gtk_entry_get_text (entry);

  if (!pkuserid || !strlen(pkuserid))
      gconf_client_unset (dialog->client, KA_GCONF_KEY_PK_USERID, NULL);
  else
      gconf_client_set_string (dialog->client, KA_GCONF_KEY_PK_USERID, pkuserid, NULL);
}


static void
ka_preferences_dialog_setup_pkuserid_entry (KaPreferencesDialog *dialog)
{
  char     *pkuserid = NULL;

  dialog->pkuserid_entry = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "pkuserid_entry"));
  g_assert (dialog->pkuserid_entry != NULL);

  if (!ka_gconf_get_string (dialog->client, KA_GCONF_KEY_PK_USERID, &pkuserid))
      g_warning ("Getting pkuserid failed");

  if (pkuserid && strlen(pkuserid))
      gtk_entry_set_text (GTK_ENTRY (dialog->pkuserid_entry), pkuserid);
  if (pkuserid)
      g_free (pkuserid);

  g_signal_connect (dialog->pkuserid_entry, "changed",
                    G_CALLBACK (ka_preferences_dialog_pkuserid_changed), dialog);
  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_PK_USERID, NULL)) {
      gtk_widget_set_sensitive (dialog->pkuserid_entry, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                          KA_GCONF_KEY_PK_USERID,
                          (GConfClientNotifyFunc) ka_preferences_pkuserid_notify,
                          dialog, NULL, NULL);
  dialog->n_listeners++;
}


static void
ka_preferences_pkanchors_notify (GConfClient *client G_GNUC_UNUSED,
                           guint cnx_id G_GNUC_UNUSED,
                           GConfEntry *entry,
                           KaPreferencesDialog *dialog)
{
  const char *pkanchors;

  if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
      return;

  pkanchors = gconf_value_get_string (entry->value);

  if (!pkanchors || !strlen(pkanchors))
      gtk_entry_set_text (GTK_ENTRY (dialog->pkanchors_entry), "");
  else {
      const char *old_pkanchors;

      old_pkanchors = gtk_entry_get_text (GTK_ENTRY (dialog->pkanchors_entry));
      if (!old_pkanchors || (old_pkanchors && strcmp (old_pkanchors, pkanchors)))
          gtk_entry_set_text (GTK_ENTRY (dialog->pkanchors_entry), pkanchors);
  }
}


static void
ka_preferences_dialog_pkanchors_changed (GtkEntry *entry,
                                   KaPreferencesDialog *dialog)
{
  const char *pkanchors;

  pkanchors = gtk_entry_get_text (entry);

  if (!pkanchors || !strlen(pkanchors))
      gconf_client_unset (dialog->client, KA_GCONF_KEY_PK_ANCHORS, NULL);
  else
      gconf_client_set_string (dialog->client, KA_GCONF_KEY_PK_ANCHORS, pkanchors, NULL);
}


static void
ka_preferences_dialog_setup_pkanchors_entry (KaPreferencesDialog *dialog)
{
  char *pkanchors = NULL;

  dialog->pkanchors_entry = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "pkanchors_entry"));
  g_assert (dialog->pkanchors_entry != NULL);

  if (!ka_gconf_get_string (dialog->client, KA_GCONF_KEY_PK_ANCHORS, &pkanchors))
      g_warning ("Getting pkanchors failed");

  if (pkanchors && strlen(pkanchors))
      gtk_entry_set_text (GTK_ENTRY (dialog->pkanchors_entry), pkanchors);
  if (pkanchors)
      g_free (pkanchors);

  g_signal_connect (dialog->pkanchors_entry, "changed",
      G_CALLBACK (ka_preferences_dialog_pkanchors_changed), dialog);
  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_PK_ANCHORS, NULL)) {
      gtk_widget_set_sensitive (dialog->pkanchors_entry, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                          KA_GCONF_KEY_PK_ANCHORS,
                          (GConfClientNotifyFunc) ka_preferences_pkanchors_notify,
                          dialog, NULL, NULL);
  dialog->n_listeners++;
}


static void
ka_preferences_toggle_pkuserid_entry (gboolean state, KaPreferencesDialog *dialog)
{
  gtk_widget_set_sensitive (dialog->pkuserid_entry, state);
  gtk_widget_set_sensitive (dialog->pkuserid_button, state);
}


static void
ka_preferences_dialog_smartcard_toggled (GtkToggleButton *toggle,
                                         KaPreferencesDialog *dialog)
{
  gboolean smartcard = gtk_toggle_button_get_active (toggle);
  static gchar *old_path = NULL;

  if (smartcard) {
      const char *path;

      path = gtk_entry_get_text (GTK_ENTRY(dialog->pkuserid_entry));
      if (g_strcmp0 (path, PKINIT_SMARTCARD)) {
          g_free (old_path);
          old_path = g_strdup (path);
      }
      ka_preferences_toggle_pkuserid_entry (FALSE, dialog);
      gconf_client_set_string (dialog->client, KA_GCONF_KEY_PK_USERID, PKINIT_SMARTCARD, NULL);
  } else {
      ka_preferences_toggle_pkuserid_entry (TRUE, dialog);
      if (old_path)
          gconf_client_set_string (dialog->client, KA_GCONF_KEY_PK_USERID, old_path, NULL);
      else
          gconf_client_unset (dialog->client, KA_GCONF_KEY_PK_USERID, NULL);
  }
}


static void
ka_preferences_dialog_setup_smartcard_toggle(KaPreferencesDialog *dialog)
{
  char *pkuserid = NULL;

  dialog->smartcard_toggle = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "smartcard_toggle"));
  g_assert (dialog->smartcard_toggle != NULL);

  if (!ka_gconf_get_string (dialog->client, KA_GCONF_KEY_PK_USERID, &pkuserid))
      g_warning ("Getting pkanchors failed");

  g_signal_connect (dialog->smartcard_toggle, "toggled",
              G_CALLBACK (ka_preferences_dialog_smartcard_toggled), dialog);

  if (!g_strcmp0 (pkuserid, PKINIT_SMARTCARD))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->smartcard_toggle), TRUE);
  else
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->smartcard_toggle), FALSE);

  if (pkuserid)
      g_free (pkuserid);
}


static void
ka_preferences_dialog_browse_certs (KaPreferencesDialog *dialog, GtkEntry *entry)
{
  GtkWidget *filechooser;
  GtkFileFilter *cert_filter, *all_filter;
  gchar *filename = NULL;
  const gchar *current;
  gint ret;

  filechooser = gtk_file_chooser_dialog_new(_("Choose Certificate"),
                                            GTK_WINDOW(dialog->dialog),
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
ka_preferences_dialog_browse_pkuserids (GtkButton *button G_GNUC_UNUSED,
                                       KaPreferencesDialog *dialog)
{
  ka_preferences_dialog_browse_certs (dialog,
                                      GTK_ENTRY(dialog->pkuserid_entry));
}

static void
ka_preferences_dialog_browse_pkanchors(GtkButton *button G_GNUC_UNUSED,
                                       KaPreferencesDialog *dialog)
{
  ka_preferences_dialog_browse_certs (dialog,
                                      GTK_ENTRY(dialog->pkanchors_entry));
}

static void
ka_preferences_dialog_setup_pkuserid_button (KaPreferencesDialog *dialog)
{
  dialog->pkuserid_button = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "pkuserid_button"));
  g_assert (dialog->pkuserid_button != NULL);

  g_signal_connect (dialog->pkuserid_button, "clicked",
                    G_CALLBACK (ka_preferences_dialog_browse_pkuserids), dialog);

}

static void
ka_preferences_dialog_setup_pkanchors_button (KaPreferencesDialog *dialog)
{
  dialog->pkanchors_button = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "pkanchors_button"));
  g_assert (dialog->pkanchors_button != NULL);

  g_signal_connect (dialog->pkanchors_button, "clicked",
                    G_CALLBACK (ka_preferences_dialog_browse_pkanchors), dialog);

}


static void
ka_preferences_dialog_forwardable_toggled (GtkToggleButton *toggle,
                                           KaPreferencesDialog *dialog)
{
  gboolean forwardable;

  forwardable = gtk_toggle_button_get_active (toggle);

  gconf_client_set_bool (dialog->client, KA_GCONF_KEY_FORWARDABLE, forwardable, NULL);
}


static void
ka_preferences_dialog_forwardable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    KaPreferencesDialog *dialog)
{
  gboolean forwardable;

  if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

  forwardable = gconf_value_get_bool (entry->value) != FALSE;

  if (forwardable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->forwardable_toggle)))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->forwardable_toggle), forwardable);
}


static gboolean
ka_preferences_dialog_setup_forwardable_toggle (KaPreferencesDialog *dialog)
{
  gboolean forwardable;

  dialog->forwardable_toggle = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "forwardable_toggle"));
  g_assert (dialog->forwardable_toggle != NULL);

  forwardable = gconf_client_get_bool (dialog->client, KA_GCONF_KEY_FORWARDABLE, NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->forwardable_toggle), forwardable);

  g_signal_connect (dialog->forwardable_toggle, "toggled",
              G_CALLBACK (ka_preferences_dialog_forwardable_toggled), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_FORWARDABLE, NULL)) {
      gtk_widget_set_sensitive (dialog->forwardable_toggle, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                       KA_GCONF_KEY_FORWARDABLE,
                       (GConfClientNotifyFunc) ka_preferences_dialog_forwardable_notify,
                       dialog, NULL, NULL);
  dialog->n_listeners++;
  return forwardable;
}


static void
ka_preferences_dialog_proxiable_toggled (GtkToggleButton *toggle,
                                     KaPreferencesDialog *dialog)
{
  gboolean proxiable;

  proxiable = gtk_toggle_button_get_active (toggle);

  gconf_client_set_bool (dialog->client, KA_GCONF_KEY_PROXIABLE, proxiable, NULL);
}


static void
ka_preferences_dialog_proxiable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    KaPreferencesDialog *dialog)
{
  gboolean proxiable;

  if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

  proxiable = gconf_value_get_bool (entry->value) != FALSE;

  if (proxiable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->proxiable_toggle)))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->proxiable_toggle), proxiable);
}


static gboolean
ka_preferences_dialog_setup_proxiable_toggle (KaPreferencesDialog *dialog)
{
  gboolean proxiable;

  dialog->proxiable_toggle = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "proxiable_toggle"));
  g_assert (dialog->proxiable_toggle != NULL);

  proxiable = gconf_client_get_bool (dialog->client, KA_GCONF_KEY_PROXIABLE, NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->proxiable_toggle), proxiable);

  g_signal_connect (dialog->proxiable_toggle, "toggled",
              G_CALLBACK (ka_preferences_dialog_proxiable_toggled), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_PROXIABLE, NULL)) {
      gtk_widget_set_sensitive (dialog->proxiable_toggle, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                       KA_GCONF_KEY_PROXIABLE,
                       (GConfClientNotifyFunc) ka_preferences_dialog_proxiable_notify,
                       dialog, NULL, NULL);
  dialog->n_listeners++;
  return proxiable;
}


static void
ka_preferences_dialog_renewable_toggled (GtkToggleButton *toggle,
                                     KaPreferencesDialog *dialog)
{
  gboolean renewable;

  renewable = gtk_toggle_button_get_active (toggle);

  gconf_client_set_bool (dialog->client, KA_GCONF_KEY_RENEWABLE, renewable, NULL);
}


static void
ka_preferences_dialog_renewable_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    KaPreferencesDialog *dialog)
{
  gboolean renewable;

  if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

  renewable = gconf_value_get_bool (entry->value) != FALSE;

  if (renewable != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->renewable_toggle)))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->renewable_toggle), renewable);
}


static gboolean
ka_preferences_dialog_setup_renewable_toggle (KaPreferencesDialog *dialog)
{
  gboolean renewable;

  dialog->renewable_toggle = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "renewable_toggle"));
  g_assert (dialog->renewable_toggle != NULL);

  renewable = gconf_client_get_bool (dialog->client, KA_GCONF_KEY_RENEWABLE, NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->renewable_toggle), renewable);

  g_signal_connect (dialog->renewable_toggle, "toggled",
              G_CALLBACK (ka_preferences_dialog_renewable_toggled), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_RENEWABLE, NULL)) {
      gtk_widget_set_sensitive (dialog->renewable_toggle, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                       KA_GCONF_KEY_RENEWABLE,
                       (GConfClientNotifyFunc) ka_preferences_dialog_renewable_notify,
                       dialog, NULL, NULL);
  dialog->n_listeners++;
  return renewable;
}

static void
ka_preferences_dialog_trayicon_toggled (GtkToggleButton *toggle,
                                     KaPreferencesDialog *dialog)
{
  gboolean trayicon;

  trayicon = gtk_toggle_button_get_active (toggle);
  gconf_client_set_bool (dialog->client, KA_GCONF_KEY_SHOW_TRAYICON, trayicon, NULL);
}


static void
ka_preferences_dialog_trayicon_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    KaPreferencesDialog *dialog)
{
  gboolean trayicon;

  if (!entry->value || entry->value->type != GCONF_VALUE_BOOL)
      return;

  trayicon = gconf_value_get_bool (entry->value) != FALSE;

  if (trayicon != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->trayicon_toggle)))
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->trayicon_toggle), trayicon);
}


static gboolean
ka_preferences_dialog_setup_trayicon_toggle (KaPreferencesDialog *dialog)
{
  gboolean trayicon;

  dialog->trayicon_toggle = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "trayicon_toggle"));
  g_assert (dialog->trayicon_toggle != NULL);

  trayicon = gconf_client_get_bool (dialog->client, KA_GCONF_KEY_SHOW_TRAYICON, NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->trayicon_toggle), trayicon);

  g_signal_connect (dialog->trayicon_toggle, "toggled",
              G_CALLBACK (ka_preferences_dialog_trayicon_toggled), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_SHOW_TRAYICON, NULL)) {
      gtk_widget_set_sensitive (dialog->trayicon_toggle, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                       KA_GCONF_KEY_SHOW_TRAYICON,
                       (GConfClientNotifyFunc) ka_preferences_dialog_trayicon_notify,
                       dialog, NULL, NULL);
  dialog->n_listeners++;
  return trayicon;
}


static void
ka_preferences_dialog_prompt_mins_changed (GtkSpinButton *button,
                                     KaPreferencesDialog *dialog)
{
  gint prompt_mins;

  prompt_mins = gtk_spin_button_get_value_as_int (button);
  gconf_client_set_int (dialog->client, KA_GCONF_KEY_PROMPT_MINS, prompt_mins, NULL);
}


static void
ka_preferences_dialog_prompt_mins_notify (GConfClient *client G_GNUC_UNUSED,
                                    guint cnx_id G_GNUC_UNUSED,
                                    GConfEntry *entry,
                                    KaPreferencesDialog *dialog)
{
  gint prompt_mins;

  if (!entry->value || entry->value->type != GCONF_VALUE_INT)
      return;

  prompt_mins = gconf_value_get_int (entry->value);

  if (prompt_mins != gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (dialog->prompt_mins_entry)))
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->prompt_mins_entry), prompt_mins);
}


static gint
ka_preferences_dialog_setup_prompt_mins_entry (KaPreferencesDialog *dialog)
{
  gint prompt_mins;

  dialog->prompt_mins_entry = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "prompt_mins_entry"));
  g_assert (dialog->prompt_mins_entry != NULL);

  prompt_mins = gconf_client_get_int (dialog->client, KA_GCONF_KEY_PROMPT_MINS, NULL);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->prompt_mins_entry), prompt_mins);

  g_signal_connect (dialog->prompt_mins_entry, "value-changed",
              G_CALLBACK (ka_preferences_dialog_prompt_mins_changed), dialog);

  if (!gconf_client_key_is_writable (dialog->client, KA_GCONF_KEY_PROMPT_MINS, NULL)) {
      gtk_widget_set_sensitive (dialog->prompt_mins_entry, FALSE);
  }

  dialog->listeners [dialog->n_listeners] = gconf_client_notify_add (dialog->client,
                       KA_GCONF_KEY_PROMPT_MINS,
                       (GConfClientNotifyFunc) ka_preferences_dialog_prompt_mins_notify,
                       dialog, NULL, NULL);
  dialog->n_listeners++;
  return prompt_mins;
}



static void
ka_preferences_dialog_response (GtkWidget *widget,
                          int response,
                          KaPreferencesDialog *dialog)
{
  if (response != GTK_RESPONSE_HELP) {
      gtk_widget_destroy (widget);
      return;
  }

  ka_show_help (gtk_window_get_screen (GTK_WINDOW (dialog->dialog)),
                "#preferences", GTK_WINDOW(dialog->dialog));
}


static void
ka_preferences_dialog_destroyed (GtkWidget *widget G_GNUC_UNUSED,
                           KaPreferencesDialog *dialog)
{
  dialog->dialog = NULL;

  gtk_main_quit ();
}


static gboolean
ka_preferences_dialog_init(KaPreferencesDialog* dialog)
{
  dialog->xml = gtk_builder_new ();

  g_assert(gtk_builder_add_from_file(dialog->xml, KA_DATA_DIR G_DIR_SEPARATOR_S
                                     PACKAGE "-preferences.xml", NULL));

  dialog->dialog = GTK_WIDGET(gtk_builder_get_object (dialog->xml, "krb5_auth_dialog_prefs"));
  g_assert (dialog->dialog);

  g_signal_connect (dialog->dialog, "response",
                    G_CALLBACK (ka_preferences_dialog_response), dialog);
  g_signal_connect (dialog->dialog, "destroy",
                    G_CALLBACK (ka_preferences_dialog_destroyed), dialog);

  dialog->client = gconf_client_get_default ();
  gconf_client_add_dir (dialog->client, KA_GCONF_PATH, GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

  ka_preferences_dialog_setup_principal_entry (dialog);
  ka_preferences_dialog_setup_pkuserid_entry (dialog);
  ka_preferences_dialog_setup_pkuserid_button (dialog);
  ka_preferences_dialog_setup_smartcard_toggle (dialog);
  ka_preferences_dialog_setup_pkanchors_entry(dialog);
  ka_preferences_dialog_setup_pkanchors_button (dialog);
  ka_preferences_dialog_setup_forwardable_toggle (dialog);
  ka_preferences_dialog_setup_proxiable_toggle (dialog);
  ka_preferences_dialog_setup_renewable_toggle (dialog);
  ka_preferences_dialog_setup_trayicon_toggle (dialog);
  ka_preferences_dialog_setup_prompt_mins_entry (dialog);

  g_assert (dialog->n_listeners == N_LISTENERS);

  gtk_widget_show (dialog->dialog);
  return TRUE;
}


static void
ka_preferences_dialog_finalize (KaPreferencesDialog *dialog)
{
  if (dialog->dialog)
      gtk_widget_destroy (dialog->dialog);
  dialog->dialog = NULL;

  if (dialog->client) {
      int i;

      for (i = 0; i < dialog->n_listeners; i++) {
          if (dialog->listeners [i])
	      gconf_client_notify_remove (dialog->client, dialog->listeners [i]);
	  dialog->listeners [i] = 0;
      }
      dialog->n_listeners = 0;

      gconf_client_remove_dir (dialog->client, KA_GCONF_PATH, NULL);

      g_object_unref (dialog->client);
      dialog->client = NULL;
  }

  if (dialog->xml)
      g_object_unref (dialog->xml);
  dialog->xml = NULL;
}

int
main (int argc, char *argv[])
{
  GOptionContext *context;
  GError *error = NULL;
  KaPreferencesDialog dialog = { NULL, };

  const char *help_msg = "Run '" PACKAGE " --help' to see a full list of available command line options";
  const GOptionEntry options [] = {
            { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  context = g_option_context_new ("- Kerberos Authentication Configuration");
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, &error);
  if (error) {
      g_print ("%s\n%s\n",
               error->message,
               help_msg);
     g_error_free (error);
     return 1;
  }
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  bindtextdomain (PACKAGE, LOCALE_DIR);

  ka_preferences_dialog_init(&dialog);
  gtk_main ();
  ka_preferences_dialog_finalize(&dialog);
  return 0;
}
