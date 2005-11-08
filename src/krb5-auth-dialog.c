/*
 * Copyright (C) 2004,2005 Red Hat, Inc.
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

#include "config.h"

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnome.h>
#include <stdlib.h>
#include <time.h>
#include <krb5.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>


#ifdef ENABLE_NETWORK_MANAGER
#include <libnm_glib.h>
#endif

static GladeXML *xml = NULL;
static krb5_context kcontext;
static krb5_principal kprincipal;
static gboolean invalid_password;
static gboolean always_run;
static gint creds_expiry;

static int renew_credentials ();
static gboolean get_tgt_from_ccache (krb5_context context, krb5_creds *creds);

static gchar* minutes_to_expiry_text (int minutes)
{
	gchar *expiry_text;
	gchar *tmp;

	if (minutes > 0)
		expiry_text = g_strdup_printf (ngettext("Your credentials expire in %d minute",
		                                        "Your credentials expire in %d minutes",
		                                        minutes),
		                               minutes);
	else
	{
		expiry_text = g_strdup (_("Your credentials have expired"));
		tmp = g_strdup_printf ("<span foreground=\"red\">%s</span>", expiry_text);
		g_free (expiry_text);
		expiry_text = tmp;
	}

	return expiry_text;
}

static gboolean
krb5_auth_dialog_wrong_label_update_expiry (gpointer data)
{
	GtkWidget *label = GTK_WIDGET(data);
	int minutes_left;
	gchar *expiry_text;
	gchar *expiry_markup;

	g_return_val_if_fail (label != NULL, FALSE);

	minutes_left = (creds_expiry - time(0)) / 60;

	expiry_text = minutes_to_expiry_text (minutes_left);

	expiry_markup = g_strdup_printf ("<span size=\"smaller\" style=\"italic\">%s</span>", expiry_text);
	gtk_label_set_markup (GTK_LABEL (label), expiry_markup);
	g_free (expiry_text);
	g_free (expiry_markup);

	return TRUE;
}

static void
krb5_auth_dialog_setup (GtkWidget *dialog,
                        const gchar *krb5prompt,
                        gboolean hide_password)
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
			prompt = g_strdup_printf (_("Please enter the password for '%s'"), uid);
		}
		else
			prompt = g_strdup (krb5prompt);
	}

	/* Clear the password entry field */
	entry = glade_xml_get_widget (xml, "krb5_entry");
	gtk_entry_set_text (GTK_ENTRY (entry), "");
	gtk_entry_set_visibility (GTK_ENTRY (entry), !hide_password);

	/* Use the prompt label that krb5 provides us */
	label = glade_xml_get_widget (xml, "krb5_message_label");
	gtk_label_set_text (GTK_LABEL (label), prompt);

	/* Add our extra message hints, if any */
	wrong_label = glade_xml_get_widget (xml, "krb5_wrong_label");
	wrong_text = NULL;

	if (wrong_label)
	{
		if (invalid_password)
			wrong_text = g_strdup (_("The password you entered is invalid"));
		else
		{
			int minutes_left = (creds_expiry - time(0)) / 60;

			wrong_text = minutes_to_expiry_text (minutes_left);
		}
	}

	if (wrong_text)
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
	GtkWidget *wrong_label;
	krb5_error_code errcode;
	int i;

	errcode = KRB5_LIBOS_CANTREADPWD;

	dialog = glade_xml_get_widget (xml, "krb5_dialog");

	for (i = 0; i < num_prompts; i++)
	{
		const gchar *password = NULL;
		int password_len = 0;
		int response;
		guint32 source_id = 0;

		GtkWidget *entry;

		errcode = KRB5_LIBOS_CANTREADPWD;

		entry = glade_xml_get_widget(xml, "krb5_entry");
		krb5_auth_dialog_setup (dialog, (gchar *) prompts[i].prompt, prompts[i].hidden);

		wrong_label = glade_xml_get_widget (xml, "krb5_wrong_label");
		source_id = g_timeout_add (5000, (GSourceFunc)krb5_auth_dialog_wrong_label_update_expiry,
		                           wrong_label);

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

		g_source_remove (source_id);

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
	krb5_creds my_creds;
	gboolean retval = FALSE;

	if (!get_tgt_from_ccache (kcontext, &my_creds)) {
		creds_expiry = 0;
		return TRUE;
	}

	if (krb5_principal_compare (kcontext, my_creds.client, kprincipal)) {
		krb5_free_principal(kcontext, kprincipal);
		krb5_copy_principal(kcontext, my_creds.client, &kprincipal);
	}
	creds_expiry = my_creds.times.endtime;
	if (time(NULL) + MINUTES_BEFORE_PROMPTING * 60 > my_creds.times.endtime)
		retval = TRUE;

	krb5_free_cred_contents(kcontext, &my_creds);

	return retval;
}

static gboolean
credentials_expiring (gpointer *data)
{
	if (credentials_expiring_real () && is_online)
		renew_credentials ();

	return TRUE;
}

#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_FORWARDABLE)
static int
get_cred_forwardable(krb5_creds *creds)
{
	return creds->ticket_flags & TKT_FLG_FORWARDABLE;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_FORWARDABLE)
static int
get_cred_forwardable(krb5_creds *creds)
{
	return creds->flags.b.forwardable;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_FORWARDABLE)
static int
get_cred_forwardable(krb5_creds *creds)
{
	return creds->flags & KDC_OPT_FORWARDABLE;
}
#endif

#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_RENEWABLE)
static int
get_cred_renewable(krb5_creds *creds)
{
	return creds->ticket_flags & TKT_FLG_RENEWABLE;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_RENEWABLE)
static int
get_cred_renewable(krb5_creds *creds)
{
	return creds->flags.b.renewable;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_RENEWABLE)
static int
get_cred_renewable(krb5_creds *creds)
{
	return creds->flags & KDC_OPT_RENEWABLE;
}
#endif

#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_PROXIABLE)
static int
get_cred_proxiable(krb5_creds *creds)
{
	return creds->ticket_flags & TKT_FLG_PROXIABLE;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_PROXIABLE)
static int
get_cred_proxiable(krb5_creds *creds)
{
	return creds->flags.b.proxiable;
}
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_PROXIABLE)
static int
get_cred_proxiable(krb5_creds *creds)
{
	return creds->flags & KDC_OPT_PROXIABLE;
}
#endif

static void
set_options_using_creds(krb5_context context,
			krb5_creds *creds,
			krb5_get_init_creds_opt *opts)
{
	krb5_deltat renew_lifetime;
	int flag;

	flag = get_cred_forwardable(creds) != 0;
	krb5_get_init_creds_opt_set_forwardable(opts, flag);
	flag = get_cred_proxiable(creds) != 0;
	krb5_get_init_creds_opt_set_proxiable(opts, flag);
	flag = get_cred_renewable(creds) != 0;
	if (flag && (creds->times.renew_till > creds->times.starttime)) {
		renew_lifetime = creds->times.renew_till -
				 creds->times.starttime;
		krb5_get_init_creds_opt_set_renew_life(opts,
						       renew_lifetime);
	}
	if (creds->times.endtime >
	    creds->times.starttime + MINUTES_BEFORE_PROMPTING * 60) {
		krb5_get_init_creds_opt_set_tkt_life(opts,
					 	     creds->times.endtime -
						     creds->times.starttime);
	}
	/* This doesn't do a deep copy -- fix it later. */
	/* krb5_get_init_creds_opt_set_address_list(opts, creds->addresses); */
}

static int
renew_credentials (void)
{
	krb5_error_code retval;
	krb5_creds my_creds;
	krb5_ccache ccache;
	krb5_get_init_creds_opt opts;

	if (kprincipal == NULL) {
		retval = krb5_parse_name(kcontext, g_get_user_name (),
					 &kprincipal);
		if (retval) {
			return retval;
		}
	}

	krb5_get_init_creds_opt_init(&opts);
	if (get_tgt_from_ccache (kcontext, &my_creds))
	{
		set_options_using_creds(kcontext, &my_creds, &opts);
		creds_expiry = my_creds.times.endtime;
		krb5_free_cred_contents(kcontext, &my_creds);
	} else {
		creds_expiry = 0;
	}

	retval = krb5_get_init_creds_password(kcontext, &my_creds, kprincipal,
                                              NULL, krb5_gtk_prompter, 0,
                                              0, NULL, &opts);
	if (retval)
	{
		switch (retval) {
			case KRB5KDC_ERR_PREAUTH_FAILED:
			case KRB5KRB_AP_ERR_BAD_INTEGRITY:
				/* Invalid password, try again. */
				invalid_password = TRUE;
				return renew_credentials();
			default:
				break;
		}
		return retval;
	}

	retval = krb5_cc_default(kcontext, &ccache);
	if (retval)
		return retval;

	retval = krb5_cc_initialize(kcontext, ccache, kprincipal);
	if (retval)
		goto out;

	retval = krb5_cc_store_cred(kcontext, ccache, &my_creds);
	if (retval)
		goto out;

	creds_expiry = my_creds.times.endtime;

out:
	krb5_cc_close (kcontext, ccache);

	return retval;
}

#if defined(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING)
static size_t
get_principal_realm_length(krb5_principal p)
{
	return strlen(p->realm);
}
static const char *
get_principal_realm_data(krb5_principal p)
{
	return p->realm;
}
#elif defined(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA)
static size_t
get_principal_realm_length(krb5_principal p)
{
	return p->realm.length;
}
static const char *
get_principal_realm_data(krb5_principal p)
{
	return p->realm.data;
}
#endif

static gboolean
get_tgt_from_ccache (krb5_context context, krb5_creds *creds)
{
	krb5_ccache ccache;
	krb5_creds mcreds;
	krb5_principal principal, tgt_principal;
	gboolean ret;

	memset(&ccache, 0, sizeof(ccache));
	ret = FALSE;
	if (krb5_cc_default(context, &ccache) == 0)
	{
		memset(&principal, 0, sizeof(principal));
		if (krb5_cc_get_principal(context, ccache, &principal) == 0)
		{
			memset(&tgt_principal, 0, sizeof(tgt_principal));
			if (krb5_build_principal_ext(context, &tgt_principal,
			                             get_principal_realm_length(principal),
			                             get_principal_realm_data(principal),
			                             KRB5_TGS_NAME_SIZE,
			                             KRB5_TGS_NAME,
			                             get_principal_realm_length(principal),
			                             get_principal_realm_data(principal),
			                             0) == 0) {
				memset(creds, 0, sizeof(*creds));
				memset(&mcreds, 0, sizeof(mcreds));
				mcreds.client = principal;
				mcreds.server = tgt_principal;
				if (krb5_cc_retrieve_cred(context, ccache,
				                          0,
				                          &mcreds,
				                          creds) == 0)
				{
					ret = TRUE;
				} else {
					memset(creds, 0, sizeof(*creds));
				}
				krb5_free_principal(context, tgt_principal);
			}
			krb5_free_principal(context, principal);
		}
		krb5_cc_close(context, ccache);
	}
	return ret;
}

static gboolean
using_krb5()
{
	krb5_error_code err;
	gboolean have_tgt = FALSE;
	krb5_creds creds;

	err = krb5_init_context(&kcontext);
	if (err)
		return TRUE;

	have_tgt = get_tgt_from_ccache(kcontext, &creds);
	if (have_tgt) {
		krb5_copy_principal(kcontext, creds.client, &kprincipal);
		krb5_free_cred_contents (kcontext, &creds);
	}

	return have_tgt;
}

int
main (int argc, char *argv[])
{
	GtkWidget *dialog;
	GnomeClient *client;
	int run_auto = 0, run_always = 0;
	struct poptOption options[] = {
		{"auto", 'a', 0, &run_auto, 0,
		 "Only run if an initialized ccache is found (default)", NULL},
		{"always", 'A', 0, &run_always, 0,
		 "Always run", NULL},
		{NULL},
	};

#ifdef ENABLE_NETWORK_MANAGER
	libnm_glib_ctx *nm_context;
	guint32 nm_callback_id;	
#endif

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
	                    argc, argv, GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_NONE);

	client = gnome_master_client ();
	gnome_client_set_restart_style (client, GNOME_RESTART_ANYWAY);

	if (run_always && !run_auto)
		always_run++;
	if (using_krb5 () || always_run)
	{
		g_signal_connect (G_OBJECT (client), "die",
		                  G_CALLBACK (gtk_main_quit), NULL);

		g_set_application_name (_("Network Authentication"));

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

				g_warning ("Could not connect to NetworkManager, connection status will not be managed!");
			}
		}
#endif /* ENABLE_NETWORK_MANAGER */

		xml = glade_xml_new (GLADEDIR "krb5-auth-dialog.glade", NULL, NULL);
		dialog = glade_xml_get_widget (xml, "krb5_dialog");

		if (credentials_expiring (NULL))
			g_timeout_add (CREDENTIAL_CHECK_INTERVAL * 1000, (GSourceFunc)credentials_expiring, NULL);

		gtk_main ();
	}

	return 0;
}
