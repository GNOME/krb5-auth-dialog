/*
 * Copyright (C) 2004 Red Hat, Inc.
 * Authored by Christopher Aillon <caillon@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <stdlib.h>
#include <time.h>
#include <krb5.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include "config.h"

#ifdef ENABLE_NETWORK_MANAGER
#include <libnm_glib.h>
#endif

#define CREDENTIAL_CHECK_INTERVAL 30000 /* milliseconds */
#define SECONDS_BEFORE_PROMPTING 1800

static GladeXML *xml = NULL;
static krb5_context kcontext;
static krb5_principal kprincipal;
static char* defname = NULL;
static gboolean invalid_password;
static gint creds_expiry;

static int renew_credentials ();


static void
krb5_auth_dialog_setup (GtkWidget *dialog,
                        const gchar *krb5prompt)
{
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *wrong_label;
	gchar *wrong_text;
	gchar *wrong_markup;
	gchar *prompt;
	int pw4len;

	if (krb5prompt == NULL)
		prompt = g_strdup (_("Please enter your Kerberos password."));
	else
	{
		/* Kerberos's prompts are a mess, and basically impossible to
		 * translate.  There's basically no way short of doing a lot of
		 * string parsing to translate them.  The most common prompt is
		 * "Password for $uid:".  We special case that one at least.  We
		 * cannot do any of the fancier strings (like challenges),
		 * though. */
		pw4len = strlen ("Password for ");
		if (strncmp (krb5prompt, "Password for ", pw4len) == 0)
		{
			gchar *uid = (gchar *) (krb5prompt + pw4len);
			prompt = g_strdup_printf (_("Please enter the Kerberos password for '%s'"), uid);
		}
		else
			prompt = g_strdup (krb5prompt);
	}

	/* Clear the password entry field */
	entry = glade_xml_get_widget (xml, "krb5_entry");
	gtk_entry_set_text (GTK_ENTRY (entry), "");

	/* Use the prompt label that krb5 provides us */
	label = glade_xml_get_widget (xml, "krb5_message_label");
	gtk_label_set_text (GTK_LABEL (label), prompt);

	/* Add our extra message hints, if any */
	if (invalid_password)
		wrong_text = g_strdup (_("The password you entered is invalid"));
	else
	{
		int minutes_left = (creds_expiry - time(0)) / 60;
		if (minutes_left > 0)
			wrong_text = g_strdup_printf (_("Your credentials expire in %d minutes"), minutes_left);
		else
			wrong_text = g_strdup (_("Your credentials have expired"));
	}

	wrong_label = glade_xml_get_widget (xml, "krb5_wrong_label");

	if (wrong_label && wrong_text != NULL)
	{
		wrong_markup = g_strdup_printf ("<span size=\"smaller\" style=\"italic\">%s</span>", wrong_text);
		gtk_label_set_markup (GTK_LABEL (wrong_label), wrong_markup);
		g_free(wrong_text);
		g_free(wrong_markup);
	}
	else
		gtk_label_set_text (GTK_LABEL (wrong_label), "");

	g_free (prompt);
}

static krb5_error_code
krb5_gtk_prompter (krb5_context ctx,
                   void *data,
                   const char *name,
                   const char *banner,
                   int num_prompts,
                   krb5_prompt prompts[])
{
	GtkWidget *dialog;
	krb5_error_code errcode;
	int i;

	errcode = KRB5_LIBOS_CANTREADPWD;

	dialog = glade_xml_get_widget (xml, "krb5_dialog");

	for (i = 0; i < num_prompts; i++)
	{
		const gchar *password = NULL;
		int password_len = 0;
		int response;

		GtkWidget *entry;

		errcode = KRB5_LIBOS_CANTREADPWD;

		entry = glade_xml_get_widget(xml, "krb5_entry");
		krb5_auth_dialog_setup (dialog, (gchar *) prompts[i].prompt);

		response = gtk_dialog_run (GTK_DIALOG (dialog));
		switch (response)
		{
			case GTK_RESPONSE_OK:
				password = gtk_entry_get_text (GTK_ENTRY (entry));
				password_len = strlen (password);
				errcode = 0;
				break;
			case GTK_RESPONSE_CANCEL:
			case GTK_RESPONSE_DELETE_EVENT:
				break;
			default:
				g_assert_not_reached ();
		}

		prompts[i].reply->data = (char *) password;
		prompts[i].reply->length = password_len;
	}

	/* Reset this, so we know the next time we get a TRUE value, it is accurate. */
	gtk_widget_hide (dialog);
	invalid_password = FALSE;

	return errcode;
}

static gboolean is_online = TRUE;

#ifdef ENABLE_NETWORK_MANAGER
static void
network_state_cb (libnm_glib_ctx *context,
                  gpointer data)
{
	gboolean *online = (gboolean*) data;

	libnm_glib_state state;

	state = libnm_glib_get_network_state (context);

	switch (state)
	{
		case LIBNM_NO_DBUS:
		case LIBNM_NO_NETWORKMANAGER:
		case LIBNM_INVALID_CONTEXT:
			/* do nothing */
			break;
		case LIBNM_NO_NETWORK_CONNECTION:
			*online = FALSE;
			break;
		case LIBNM_ACTIVE_NETWORK_CONNECTION:
			*online = TRUE;
			break;
	}
}
#endif

static gboolean
credentials_expiring_real (void)
{
	krb5_ccache cache = NULL;
	krb5_cc_cursor cur;
	krb5_creds my_creds;
	krb5_principal princ;
	krb5_flags flags;
	krb5_error_code code;
	int exit_status = 0;
	int expiry;

	gboolean retval = FALSE;

	memset (&my_creds, 0, sizeof(my_creds));

	code = krb5_init_context (&kcontext);
	if (code)
		return FALSE;

	if ((code = krb5_cc_default(kcontext, &cache)))
		return FALSE;

	flags = 0;								/* turns off OPENCLOSE mode */
	if ((code = krb5_cc_set_flags(kcontext, cache, flags)))
	{
		if (code == KRB5_FCC_NOFILE)
		{
#ifdef KRB5_KRB4_COMPAT
			if (name == NULL)
				do_v4_ccache(0);
#endif
		}
		gtk_exit(1);
	}

	if ((code = krb5_cc_get_principal(kcontext, cache, &princ)))
		gtk_exit(1);

	if ((code = krb5_unparse_name(kcontext, princ, &defname)))
		gtk_exit(1);

	if ((code = krb5_cc_start_seq_get(kcontext, cache, &cur)))
		gtk_exit(1);

	while (!(code = krb5_cc_next_cred(kcontext, cache, &cur, &my_creds)))
	{
		if (my_creds.times.endtime - time(0) < SECONDS_BEFORE_PROMPTING)
		{
			retval = TRUE;
			creds_expiry = my_creds.times.endtime;
		}

		krb5_free_cred_contents(kcontext, &my_creds);
	}

	if (code == KRB5_CC_END)
	{
		if ((code = krb5_cc_end_seq_get(kcontext, cache, &cur)))
			exit(1);
#ifndef HEIMDAL
		flags = KRB5_TC_OPENCLOSE;	/* turns on OPENCLOSE mode, from klist.c */
#endif
		if ((code = krb5_cc_set_flags(kcontext, cache, flags)))
			gtk_exit(1);
#ifdef KRB5_KRB4_COMPAT
		if (name == NULL && !status_only)
			do_v4_ccache(0);
#endif
		if (exit_status)
		gtk_exit(exit_status);
	}
	else
	{
		gtk_exit(1);
	}

	return retval;
}

static gboolean
credentials_expiring (gpointer *data)
{
	if (credentials_expiring_real () && is_online)
		renew_credentials ();

	return TRUE;
}

static int
renew_credentials (void)
{
	krb5_error_code retval;
	krb5_get_init_creds_opt options;
	krb5_creds my_creds;
	krb5_ccache ccache;

	memset(&my_creds, 0, sizeof(my_creds));
	krb5_get_init_creds_opt_init(&options);

	retval = krb5_init_context(&kcontext);
	if (retval)
		return retval;

	retval = krb5_parse_name(kcontext, g_get_user_name (), &kprincipal);
	if (retval)
		return retval;

	retval = krb5_get_init_creds_password(kcontext, &my_creds, kprincipal,
                                              NULL, krb5_gtk_prompter, 0,
                                              0, NULL, &options);
	if (retval)
	{
		if (retval == KRB5KRB_AP_ERR_BAD_INTEGRITY)
		{
			/* Invalid password, try again. */
			invalid_password = TRUE;
			return renew_credentials();
		}
		return retval;
	}

	retval = krb5_cc_default(kcontext, &ccache);
	if (retval)
		return retval;

	retval = krb5_cc_initialize(kcontext, ccache, kprincipal);
	if (retval)
		return retval;

	retval = krb5_cc_store_cred(kcontext, ccache, &my_creds);
	if (retval)
		return retval;

	return 0;
}

gboolean
using_krb5()
{
	const gchar *krb5ccname;

	gboolean success;
	int exit_status;
	GError *error;

	/* See if we have a credential cache specified. */
	krb5ccname = g_getenv("KRB5CCNAME");
	if (krb5ccname != NULL)
		return TRUE;

	/* Nope, let's see if we have any prior tickets. */
	success = g_spawn_command_line_sync("klist -s",
                                            NULL, NULL,
                                            &exit_status,
                                            &error);

	if (success == TRUE && error == NULL &&
	    WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0)
		return TRUE;

	return FALSE;
}

int
main (int argc, char *argv[])
{
	GtkWidget *dialog;
	GnomeClient *client;

#ifdef ENABLE_NETWORK_MANAGER
	libnm_glib_ctx *nm_context;
	guint32 nm_callback_id;	
#endif

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
	                    argc, argv, GNOME_PARAM_NONE);

	client = gnome_master_client ();
	gnome_client_set_restart_style (client, GNOME_RESTART_ANYWAY);

	if (using_krb5 ())
	{
		g_signal_connect (G_OBJECT (client), "die",
		                  G_CALLBACK (gtk_main_quit), NULL);

#ifdef ENABLE_NETWORK_MANAGER
		nm_context = libnm_glib_init ();
		if (!nm_context)
			g_warning ("Could not initialize libnm_glib");
		else
		{
			nm_callback_id = libnm_glib_register_callback (nm_context, network_state_cb, &is_online, NULL);
			if (nm_callback_id == 0)
			{
				libnm_glib_shutdown (nm_context);
				nm_context = NULL;

				g_warning ("Could not connect to NetworkManager, connection status will not be managed!\n");
			}
		}
#endif /* ENABLE_NETWORK_MANAGER */

		xml = glade_xml_new (GLADEDIR "krb5-auth-dialog.glade", NULL, NULL);
		dialog = glade_xml_get_widget (xml, "krb5_dialog");

		g_timeout_add (CREDENTIAL_CHECK_INTERVAL, (GSourceFunc)credentials_expiring, NULL);
		credentials_expiring (NULL);

		gtk_main ();
	}

	return 0;
}
