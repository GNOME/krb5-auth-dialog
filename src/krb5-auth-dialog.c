/*
 * Copyright (C) 2004,2005,2006 Red Hat, Inc.
 * Authored by Christopher Aillon <caillon@redhat.com>
 *
 * Copyright (C) 2008,2009 Guido Guenther <agx@sigxcup.org>
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

#include <stdlib.h>
#include <time.h>
#include <krb5.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glade/glade.h>

#include "gtksecentry.h"
#include "secmem-util.h"
#include "memory.h"

#include "krb5-auth-dialog.h"
#include "krb5-auth-applet.h"
#include "krb5-auth-gconf.h"
#include "krb5-auth-dbus.h"

#ifdef ENABLE_NETWORK_MANAGER
#include <libnm_glib.h>
#endif

#ifdef HAVE_HX509_ERR_H
# include <hx509_err.h>
#endif

static krb5_context kcontext;
static krb5_principal kprincipal;
static krb5_timestamp creds_expiry;
static krb5_timestamp canceled_creds_expiry;
static gboolean canceled;
static gboolean invalid_auth;
static gboolean always_run;

static int grab_credentials (KaApplet* applet);
static int ka_renew_credentials (KaApplet* applet);
static gboolean ka_get_tgt_from_ccache (krb5_context context, krb5_creds *creds);

/* YAY for different Kerberos implementations */
static int
get_cred_forwardable(krb5_creds *creds)
{
#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_FORWARDABLE)
	return creds->ticket_flags & TKT_FLG_FORWARDABLE;
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_FORWARDABLE)
	return creds->flags.b.forwardable;
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_FORWARDABLE)
	return creds->flags & KDC_OPT_FORWARDABLE;
#endif
}

static int
get_cred_renewable(krb5_creds *creds)
{
#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_RENEWABLE)
	return creds->ticket_flags & TKT_FLG_RENEWABLE;
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_RENEWABLE)
	return creds->flags.b.renewable;
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_RENEWABLE)
	return creds->flags & KDC_OPT_RENEWABLE;
#endif
}

static krb5_error_code
get_renewed_creds(krb5_context context,
                  krb5_creds *creds,
                  krb5_principal client,
                  krb5_ccache ccache,
                  char *in_tkt_service)
{
#ifdef HAVE_KRB5_GET_RENEWED_CREDS
	return krb5_get_renewed_creds (context, creds, client, ccache, in_tkt_service);
#else
	return 1; /* XXX is there something better to return? */
#endif
}

static int
get_cred_proxiable(krb5_creds *creds)
{
#if defined(HAVE_KRB5_CREDS_TICKET_FLAGS) && defined(TKT_FLG_PROXIABLE)
	return creds->ticket_flags & TKT_FLG_PROXIABLE;
#elif defined(HAVE_KRB5_CREDS_FLAGS_B_PROXIABLE)
	return creds->flags.b.proxiable;
#elif defined(HAVE_KRB5_CREDS_FLAGS) && defined(KDC_OPT_PROXIABLE)
	return creds->flags & KDC_OPT_PROXIABLE;
#endif
}

static size_t
get_principal_realm_length(krb5_principal p)
{
#if defined(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING)
	return strlen(p->realm);
#elif defined(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA)
	return p->realm.length;
#endif
}

static const char *
get_principal_realm_data(krb5_principal p)
{
#if defined(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING)
	return p->realm;
#elif defined(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA)
	return p->realm.data;
#endif
}

static const char*
get_error_message(krb5_context context, krb5_error_code err)
{
	const char *msg = NULL;

#if defined(HAVE_KRB5_GET_ERROR_MESSAGE)
	msg = krb5_get_error_message(context, err);
#else
	msg = error_message(err);
#endif
	if (msg == NULL)
		return "unknown error";
	else
		return msg;
}

static void
ka_krb5_cc_clear_mcred(krb5_creds* mcred)
{
#if defined HAVE_KRB5_CC_CLEAR_MCRED
	krb5_cc_clear_mcred(mcred);
#else
	memset(mcred, 0, sizeof(krb5_creds));
#endif
}

/* ***************************************************************** */
/* ***************************************************************** */

static gboolean
credentials_expiring_real (KaApplet* applet)
{
	krb5_creds my_creds;
	krb5_timestamp now;
	gboolean retval = FALSE;

	ka_applet_set_tgt_renewable(applet, FALSE);
	if (!ka_get_tgt_from_ccache (kcontext, &my_creds)) {
		creds_expiry = 0;
		retval = TRUE;
		goto out;
	}

	if (krb5_principal_compare (kcontext, my_creds.client, kprincipal)) {
		krb5_free_principal(kcontext, kprincipal);
		krb5_copy_principal(kcontext, my_creds.client, &kprincipal);
	}
	creds_expiry = my_creds.times.endtime;
	if ((krb5_timeofday(kcontext, &now) == 0) &&
	    (now + ka_applet_get_pw_prompt_secs(applet) > my_creds.times.endtime))
		retval = TRUE;

	/* If our creds are expiring, determine whether they are renewable */
	if (retval && get_cred_renewable(&my_creds) && my_creds.times.renew_till > now) {
		ka_applet_set_tgt_renewable(applet, TRUE);
	}

	krb5_free_cred_contents (kcontext, &my_creds);

out:
	ka_applet_update_status(applet, creds_expiry);
	return retval;
}


static gchar* minutes_to_expiry_text (int minutes)
{
	gchar *expiry_text;
	gchar *tmp;

	if (minutes > 0) {
		expiry_text = g_strdup_printf (ngettext("Your credentials expire in %d minute",
		                                        "Your credentials expire in %d minutes",
		                                        minutes),
		                               minutes);
	} else {
		expiry_text = g_strdup (_("Your credentials have expired"));
		tmp = g_strdup_printf ("<span foreground=\"red\">%s</span>", expiry_text);
		g_free (expiry_text);
		expiry_text = tmp;
	}

	return expiry_text;
}


static gboolean
krb5_auth_dialog_wrong_label_update_expiry (GtkWidget* label)
{
	int minutes_left;
	krb5_timestamp now;
	gchar *expiry_text;
	gchar *expiry_markup;

	g_return_val_if_fail (label!= NULL, FALSE);

	if (krb5_timeofday(kcontext, &now) != 0) {
		return TRUE;
	}

	minutes_left = (creds_expiry - now) / 60;
	expiry_text = minutes_to_expiry_text (minutes_left);

	expiry_markup = g_strdup_printf ("<span size=\"smaller\" style=\"italic\">%s</span>", expiry_text);
	gtk_label_set_markup (GTK_LABEL (label), expiry_markup);
	g_free (expiry_text);
	g_free (expiry_markup);

	return TRUE;
}


/* Check for things we have to do while the password dialog is open */
static gboolean
krb5_auth_dialog_do_updates (gpointer data)
{
	KaApplet* applet = KA_APPLET(data);

	g_return_val_if_fail (applet != NULL, FALSE);
	/* Update creds_expiry and close the applet if we got the creds by other means (e.g. kinit) */
	if (!credentials_expiring_real(applet))
		ka_applet_hide_pw_dialog(applet, FALSE);

	/* Update the expiry information in the dialog */
	krb5_auth_dialog_wrong_label_update_expiry (ka_applet_get_pw_label(applet));
	return TRUE;
}


static void
krb5_auth_dialog_setup (KaApplet *applet,
                        const gchar *krb5prompt,
                        gboolean hide_password)
{
	GtkWidget *entry;
	GtkWidget *label;
	gchar *wrong_text;
	gchar *wrong_markup;
	gchar *prompt;
	int pw4len;

	if (krb5prompt == NULL) {
		prompt = g_strdup (_("Please enter your Kerberos password."));
	} else {
		/* Kerberos's prompts are a mess, and basically impossible to
		 * translate.  There's basically no way short of doing a lot of
		 * string parsing to translate them.  The most common prompt is
		 * "Password for $uid:".  We special case that one at least.  We
		 * cannot do any of the fancier strings (like challenges),
		 * though. */
		pw4len = strlen ("Password for ");
		if (strncmp (krb5prompt, "Password for ", pw4len) == 0) {
			gchar *uid = (gchar *) (krb5prompt + pw4len);
			prompt = g_strdup_printf (_("Please enter the password for '%s'"), uid);
		} else {
			prompt = g_strdup (krb5prompt);
		}
	}

	/* Clear the password entry field */
	entry = glade_xml_get_widget (ka_applet_get_pwdialog_xml(applet),
                                      "krb5_entry");
	gtk_secure_entry_set_text (GTK_SECURE_ENTRY (entry), "");

	/* Use the prompt label that krb5 provides us */
	label = glade_xml_get_widget (ka_applet_get_pwdialog_xml(applet),
                                      "krb5_message_label");
	gtk_label_set_text (GTK_LABEL (label), prompt);

	/* Add our extra message hints, if any */
	wrong_text = NULL;

	if (ka_applet_get_pw_label(applet)) {
		if (invalid_auth) {
			wrong_text = g_strdup (_("The password you entered is invalid"));
		} else {
			krb5_timestamp now;
			int minutes_left;

			if (krb5_timeofday(kcontext, &now) == 0)
				minutes_left = (creds_expiry - now) / 60;
			else
				minutes_left = 0;
			wrong_text = minutes_to_expiry_text (minutes_left);
		}
	}

	if (wrong_text) {
		wrong_markup = g_strdup_printf ("<span size=\"smaller\" style=\"italic\">%s</span>", wrong_text);
		gtk_label_set_markup (GTK_LABEL (ka_applet_get_pw_label(applet)), wrong_markup);
		g_free(wrong_text);
		g_free(wrong_markup);
	} else {
		gtk_label_set_text (GTK_LABEL (ka_applet_get_pw_label(applet)), "");
	}
	g_free (prompt);
}


static krb5_error_code
auth_dialog_prompter (krb5_context ctx,
                      void *data,
                      const char *name,
                      const char *banner,
                      int num_prompts,
                      krb5_prompt prompts[])
{
	KaApplet* applet = KA_APPLET(data);
	krb5_error_code errcode;
	int i;

	errcode = KRB5KRB_ERR_GENERIC;
	canceled = FALSE;
	canceled_creds_expiry = 0;

	for (i = 0; i < num_prompts; i++) {
		const gchar *password = NULL;
		int password_len = 0;
		int response;
		guint32 source_id;

		GtkWidget *entry;

		errcode = KRB5_LIBOS_CANTREADPWD;

		krb5_auth_dialog_setup (applet, (gchar *) prompts[i].prompt, prompts[i].hidden);
		entry = glade_xml_get_widget (ka_applet_get_pwdialog_xml(applet), "krb5_entry");
		gtk_widget_grab_focus (entry);

		source_id = g_timeout_add_seconds (5, (GSourceFunc)krb5_auth_dialog_do_updates, applet);
		response = ka_applet_run_pw_dialog (applet);
		switch (response)
		{
			case GTK_RESPONSE_OK:
				password = gtk_secure_entry_get_text (GTK_SECURE_ENTRY (entry));
				password_len = strlen (password);
				break;
			case GTK_RESPONSE_CANCEL:
				canceled = TRUE;
				break;
			case GTK_RESPONSE_NONE:
			case GTK_RESPONSE_DELETE_EVENT:
				break;
			default:
				g_warning ("Unknown Response: %d", response);
				g_assert_not_reached ();
		}
		g_source_remove (source_id);

		if (!password)
			goto cleanup;
		if (password_len+1 > prompts[i].reply->length) {
			g_warning("Password too long %d/%d", password_len+1, prompts[i].reply->length);
			goto cleanup;
		}

		memcpy(prompts[i].reply->data, (char *) password, password_len + 1);
		prompts[i].reply->length = password_len;
		errcode = 0;
	}
cleanup:
	ka_applet_hide_pw_dialog (applet, TRUE);
	/* Reset this, so we know the next time we get a TRUE value, it is accurate. */
	invalid_auth = FALSE;

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

/* credentials expiring timer */
static gboolean
credentials_expiring (gpointer *data)
{
	int retval;
	gboolean give_up;
	KaApplet* applet = KA_APPLET(data);

	KA_DEBUG("Checking expiry <%ds", ka_applet_get_pw_prompt_secs(applet));
	if (credentials_expiring_real (applet) && is_online) {
		KA_DEBUG("Expiry @ %ld", creds_expiry);

		if (!ka_renew_credentials (applet)) {
			KA_DEBUG("Credentials renewed");
			goto out;
		}

		/* no popup when using a trayicon */
		if (ka_applet_get_show_trayicon(applet))
			goto out;

		give_up = canceled && (creds_expiry == canceled_creds_expiry);
		if (!give_up) {
			do {
				retval = grab_credentials (applet);
				give_up = canceled &&
					  (creds_expiry == canceled_creds_expiry);
			} while ((retval != 0) && 
			         (retval != KRB5_REALM_CANT_RESOLVE) &&
			         (retval != KRB5_KDC_UNREACH) &&
				 invalid_auth &&
			         !give_up);
		}
	}
out:
	ka_applet_update_status(applet, creds_expiry);
	return TRUE;
}


static void
set_options_from_creds(const KaApplet* applet,
		       krb5_context context,
		       krb5_creds *in,
		       krb5_get_init_creds_opt *out)
{
	krb5_deltat renew_lifetime;
	int flag;

#ifdef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_DEFAULT_FLAGS
	krb5_get_init_creds_opt_set_default_flags(kcontext, PACKAGE,
		krb5_principal_get_realm(kcontext, kprincipal), out);
#endif

	flag = get_cred_forwardable(in) != 0;
	krb5_get_init_creds_opt_set_forwardable(out, flag);
	flag = get_cred_proxiable(in) != 0;
	krb5_get_init_creds_opt_set_proxiable(out, flag);
	flag = get_cred_renewable(in) != 0;
	if (flag && (in->times.renew_till > in->times.starttime)) {
		renew_lifetime = in->times.renew_till -
				 in->times.starttime;
		krb5_get_init_creds_opt_set_renew_life(out,
						       renew_lifetime);
	}
	if (in->times.endtime >
	    in->times.starttime + ka_applet_get_pw_prompt_secs(applet)) {
		krb5_get_init_creds_opt_set_tkt_life(out,
						     in->times.endtime -
						     in->times.starttime);
	}
	/* This doesn't do a deep copy -- fix it later. */
	/* krb5_get_init_creds_opt_set_address_list(out, creds->addresses); */
}


static krb5_error_code
ka_auth_pkinit(KaApplet* applet, krb5_creds* creds, const char* pk_userid)
{
#ifdef ENABLE_PKINIT
	krb5_get_init_creds_opt *opts = NULL;
	krb5_error_code retval;

	KA_DEBUG("pkinit with '%s'", pk_userid);

	retval = krb5_get_init_creds_opt_alloc (kcontext, &opts);
	if (retval)
		goto out;
	set_options_from_creds (applet, kcontext, creds, opts);

	retval = krb5_get_init_creds_opt_set_pkinit(kcontext, opts,
						    kprincipal,
						    pk_userid,
						    NULL, /* x509 anchors */
						    NULL,
						    NULL,
						    0,	  /* pk_use_enc_key */
						    auth_dialog_prompter,
						    applet, /* data */
						    NULL);  /* passwd */
	KA_DEBUG("pkinit returned with %d", retval);
	if (retval)
		goto out;

	retval = krb5_get_init_creds_password(kcontext, creds, kprincipal,
	                                      NULL, auth_dialog_prompter, applet,
		                              0, NULL, opts);
out:
	krb5_get_init_creds_opt_free(kcontext, opts);
	return retval;
#else  /* ENABLE_PKINIT */
	return 0;
#endif /* ! ENABLE_PKINIT */
}


krb5_error_code
ka_parse_name(KaApplet* applet, krb5_context kcontext, krb5_principal* kprinc)
{
	krb5_error_code ret;
	gchar *principal = NULL;

	g_object_get(applet, "principal", &principal,
			     NULL);

	ret = krb5_parse_name(kcontext, principal,
			      kprinc);

	g_free(principal);
	return ret;
}


/* grab credentials interactively */
static int
grab_credentials (KaApplet* applet)
{
	krb5_error_code retval;
	krb5_creds my_creds;
	krb5_ccache ccache;
	krb5_get_init_creds_opt *opt = NULL;
	gchar *pk_userid = NULL;

	g_object_get(applet, "pk-userid", &pk_userid,
			     NULL);

	memset(&my_creds, 0, sizeof(my_creds));

	if (kprincipal == NULL) {
		retval = ka_parse_name(applet, kcontext, &kprincipal);
		if (retval)
			goto out2;
	}

	retval = krb5_cc_default (kcontext, &ccache);
	if (retval)
		goto out2;

#if ENABLE_PKINIT
	if (pk_userid && strlen(pk_userid)) { /* try pkinit */
#else
	if (0) {
#endif
		retval = ka_auth_pkinit(applet, &my_creds, pk_userid);
	} else {
		retval = krb5_get_init_creds_opt_alloc (kcontext, &opt);
		if (retval)
			goto out;
		set_options_from_creds (applet, kcontext, &my_creds, opt);
		retval = krb5_get_init_creds_password(kcontext, &my_creds, kprincipal,
						      NULL, auth_dialog_prompter, applet,
						      0, NULL, opt);
	}
	creds_expiry = my_creds.times.endtime;
	if (canceled)
		canceled_creds_expiry = creds_expiry;
	if (retval) {
		switch (retval) {
			case KRB5KDC_ERR_PREAUTH_FAILED:
			case KRB5KRB_AP_ERR_BAD_INTEGRITY:
#ifdef HAVE_HX509_ERR_H
			case HX509_PKCS11_LOGIN:
#endif
				/* Invalid password/pin, try again. */
				invalid_auth = TRUE;
				goto out;
			default:
				KA_DEBUG("Auth failed with %d: %s", retval,
				         get_error_message(kcontext, retval));
				break;
		}
		goto out;
	}
	retval = krb5_cc_initialize(kcontext, ccache, kprincipal);
	if (retval)
		goto out;

	retval = krb5_cc_store_cred(kcontext, ccache, &my_creds);
	if (retval)
		goto out;

out:
	if (opt)
		krb5_get_init_creds_opt_free(kcontext, opt);
	krb5_free_cred_contents (kcontext, &my_creds);
	krb5_cc_close (kcontext, ccache);
out2:
	g_free(pk_userid);

	return retval;
}

/* try to renew the credentials noninteractively */
static int
ka_renew_credentials (KaApplet* applet)
{
	krb5_error_code retval;
	krb5_creds my_creds;
	krb5_ccache ccache;
	krb5_get_init_creds_opt opts;

	if (kprincipal == NULL) {
		retval = ka_parse_name(applet, kcontext, &kprincipal);
		if (retval)
			return retval;
	}

	retval = krb5_cc_default (kcontext, &ccache);
	if (retval)
		return retval;

	retval = ka_get_tgt_from_ccache (kcontext, &my_creds);
	if (!retval) {
		krb5_cc_close (kcontext, ccache);
		return -1;
	}

	krb5_get_init_creds_opt_init (&opts);
	set_options_from_creds (applet, kcontext, &my_creds, &opts);

	if (ka_applet_get_tgt_renewable(applet)) {
		retval = get_renewed_creds (kcontext, &my_creds, kprincipal, ccache, NULL);
		if (retval)
			goto out;

		retval = krb5_cc_initialize(kcontext, ccache, kprincipal);
		if(retval) {
			g_warning("krb5_cc_initialize: %s", get_error_message(kcontext, retval));
			goto out;
		}
		retval = krb5_cc_store_cred(kcontext, ccache, &my_creds);
		if (retval) {
			g_warning("krb5_cc_store_cred: %s", get_error_message(kcontext, retval));
			goto out;
		}
	}
out:
	creds_expiry = my_creds.times.endtime;
	krb5_free_cred_contents (kcontext, &my_creds);
	krb5_cc_close (kcontext, ccache);
	return retval;
}


/* get principal associated with the default credentials cache - if found store
 * it in *creds, return FALSE otwerwise */
static gboolean
ka_get_tgt_from_ccache (krb5_context context, krb5_creds *creds)
{
	krb5_ccache ccache;
	krb5_creds pattern;
	krb5_principal principal;
	gboolean ret = FALSE;

	ka_krb5_cc_clear_mcred(&pattern);

	if (krb5_cc_default(context, &ccache))
		return FALSE;

	if (krb5_cc_get_principal(context, ccache, &principal))
		goto out;

	if (krb5_build_principal_ext(context, &pattern.server,
				     get_principal_realm_length(principal),
			             get_principal_realm_data(principal),
				     KRB5_TGS_NAME_SIZE,
				     KRB5_TGS_NAME,
				     get_principal_realm_length(principal),
				     get_principal_realm_data(principal), 0)) {
		goto out_free_princ;
	}

	pattern.client = principal;
	if (!krb5_cc_retrieve_cred(context, ccache, 0, &pattern, creds))
		ret = TRUE;
	krb5_free_principal(context, pattern.server);

out_free_princ:
	krb5_free_principal(context, principal);
out:
	krb5_cc_close(context, ccache);
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
		return FALSE;

	have_tgt = ka_get_tgt_from_ccache(kcontext, &creds);
	if (have_tgt) {
		krb5_copy_principal(kcontext, creds.client, &kprincipal);
		krb5_free_cred_contents (kcontext, &creds);
	}

	return have_tgt;
}


void
ka_destroy_cache (GtkMenuItem  *menuitem, gpointer data)
{
	KaApplet *applet = KA_APPLET(data);
	krb5_ccache  ccache;
	const char* cache;
	krb5_error_code ret;

	cache = krb5_cc_default_name(kcontext);
	ret =  krb5_cc_resolve(kcontext, cache, &ccache);
	ret = krb5_cc_destroy (kcontext, ccache);

	credentials_expiring_real(applet);
}


static void
ka_error_dialog(int err)
{
	const char* msg = get_error_message(kcontext, err);
	GtkWidget *dialog = gtk_message_dialog_new (NULL,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				_("Couldn't acquire kerberos ticket: '%s'"), msg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}


/*
 * check if we have valid credentials for the requested principal - if not, grab them
 * principal: requested principal - if empty use default
 */
gboolean
ka_check_credentials (KaApplet *applet, const char* newprincipal)
{
	gboolean renewable;
	gboolean success = FALSE;
	int retval;
	char* principal;

	g_object_get(applet, "principal", &principal,
			     NULL);

	if (strlen(newprincipal)) {
		krb5_principal knewprinc;

		/* no ticket cache: is requested princ the one from our config? */
		if (!kprincipal && g_strcmp0(principal, newprincipal)) {
			KA_DEBUG("Requested principal %s not %s", principal, newprincipal);
			goto out;
		}

		/* ticket cache: check if the requested principal is the one we have */
		retval = krb5_parse_name(kcontext, newprincipal, &knewprinc);
		if (retval) {
			g_warning ("Cannot parse principal '%s'", newprincipal);
			goto out;
		}
		if (kprincipal && !krb5_principal_compare (kcontext, kprincipal, knewprinc)) {
			KA_DEBUG("Current Principal '%s' not '%s'", principal, newprincipal);
		        krb5_free_principal(kcontext, knewprinc);
			goto out;
		}
		krb5_free_principal(kcontext, knewprinc);
	}

	if (credentials_expiring_real (applet)) {
		if (!is_online)
			success = FALSE;
		else
			success = ka_grab_credentials (applet);
	} else
		success = TRUE;
out:
	g_free (principal);
	return success;
}


/* initiate grabbing of credentials (e.g. on leftclick of tray icon) */
gboolean
ka_grab_credentials (KaApplet* applet)
{
	int retval;
	gboolean retry;
	int success = FALSE;

	ka_applet_set_pw_dialog_persist(applet, TRUE);
	do {
		retry = TRUE;
		retval = grab_credentials (applet);
		if (invalid_auth)
			continue;
		switch (retval) {
		    case 0: /* success */
			    success = TRUE;
		    case KRB5_LIBOS_PWDINTR:     /* canceled (heimdal) */
		    case KRB5_LIBOS_CANTREADPWD: /* canceled (mit) */
			    retry = FALSE;
			    break;
		    case KRB5KDC_ERR_C_PRINCIPAL_UNKNOWN:
		    default:
			    ka_error_dialog(retval);
			    retry = FALSE;
			    break;
		}
	} while(retry);

	ka_applet_set_pw_dialog_persist(applet, FALSE);
	credentials_expiring_real(applet);

	return success;
}


static GtkWidget*
ka_create_gtk_secure_entry (GladeXML *xml, gchar *func_name, gchar *name,
				gchar *s1, gchar *s2, gint i1, gint i2,
				gpointer user_data)
{
	GtkWidget* entry = NULL;

	if (!strcmp(name, "krb5_entry")) {
		entry = gtk_secure_entry_new ();
		gtk_secure_entry_set_activates_default(GTK_SECURE_ENTRY(entry), TRUE);
		gtk_widget_show (entry);
	} else {
		g_warning("Don't know anything about widget %s", name);
	}
	return entry;
}


static void
ka_secmem_init ()
{
	/* Initialize secure memory.  1 is too small, so the default size
	will be used.  */
	secmem_init (1);
	secmem_set_flags (SECMEM_WARN);
	drop_privs ();

	if (atexit (secmem_term))
		g_error("Couln't register atexit handler");
}


int
main (int argc, char *argv[])
{
	KaApplet *applet;
	GOptionContext *context;
	GError *error = NULL;

	guint status = 0;
	gboolean run_auto = FALSE, run_always = FALSE;

	const char *help_msg = "Run '" PACKAGE " --help' to see a full list of available command line options";
	const GOptionEntry options [] = {
		{"auto", 'a', 0, G_OPTION_ARG_NONE, &run_auto,
		 	"Only run if an initialized ccache is found (default)", NULL},
		{"always", 'A', 0, G_OPTION_ARG_NONE, &run_always,
		 	"Always run", NULL},
  		{ NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
	};

#ifdef ENABLE_NETWORK_MANAGER
	libnm_glib_ctx *nm_context;
	guint32 nm_callback_id;	
#endif
	context = g_option_context_new ("- Kerberos 5 credential checking");
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
	ka_secmem_init();

	if (!ka_dbus_connect (&status))
		exit(status);

	if (run_always && !run_auto) {
		always_run = TRUE;
	}
	if (using_krb5 () || always_run) {
		g_set_application_name (_("Network Authentication"));
		glade_set_custom_handler (&ka_create_gtk_secure_entry, NULL);

		applet = ka_applet_create ();
		if (!applet)
			return 1;
		if (!ka_gconf_init (applet, argc, argv))
			return 1;

#ifdef ENABLE_NETWORK_MANAGER
		nm_context = libnm_glib_init ();
		if (!nm_context) {
			g_warning ("Could not initialize libnm_glib");
		} else {
			nm_callback_id = libnm_glib_register_callback (nm_context, network_state_cb, &is_online, NULL);
			if (nm_callback_id == 0) {
				libnm_glib_shutdown (nm_context);
				nm_context = NULL;

				g_warning ("Could not connect to NetworkManager, connection status will not be managed!");
			}
		}
#endif /* ENABLE_NETWORK_MANAGER */

		if (credentials_expiring ((gpointer)applet)) {
			g_timeout_add_seconds (CREDENTIAL_CHECK_INTERVAL, (GSourceFunc)credentials_expiring, applet);
		}
		ka_dbus_service(applet);
		gtk_main ();
	}

	return 0;
}
