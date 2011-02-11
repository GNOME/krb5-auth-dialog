/*
 * Copyright (C) 2004,2005,2006 Red Hat, Inc.
 * Authored by Christopher Aillon <caillon@redhat.com>
 *
 * Copyright (C) 2008,2009,2010 Guido Guenther <agx@sigxcpu.org>
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
#include <gio/gio.h>

#include "secmem-util.h"
#include "memory.h"

#include "ka-dialog.h"
#include "ka-applet-priv.h"
#include "ka-pwdialog.h"
#include "ka-dbus.h"
#include "ka-tools.h"
#include "ka-tickets.h"

#ifdef ENABLE_NETWORK_MANAGER
#include <nm-client.h>
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
static gboolean is_online = TRUE;

static int grab_credentials (KaApplet* applet);
static int ka_renew_credentials (KaApplet* applet);
static gboolean ka_get_tgt_from_ccache (krb5_context context, krb5_creds *creds);

#ifdef ENABLE_NETWORK_MANAGER
NMClient *nm_client;
#endif

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

static void
ka_krb5_free_error_message(krb5_context context, const char* msg)
{
#if defined(HAVE_KRB5_FREE_ERROR_MESSAGE)
	krb5_free_error_message(context, msg);
#elif defined(HAVE_KRB5_FREE_ERROR_STRING)
	krb5_free_error_string(context, (char *) msg);
#else
#	error No way to free error string.
#endif
}

/*
 * Returns a descriptive error message or kerberos related error
 * returned pointer must be freed using g_free().
 */
static char*
ka_get_error_message(krb5_context context, krb5_error_code err)
{
	char *msg = NULL;
#if defined(HAVE_KRB5_GET_ERROR_MESSAGE)
	const char *krberr;

	krberr = krb5_get_error_message(context, err);
	msg = g_strdup(krberr);
	ka_krb5_free_error_message(context, krberr);
#else
#	error No detailed error message information
#endif
	if (msg == NULL)
		msg = g_strdup(_("unknown error"));
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

/* log a kerberos error messge */
static void
ka_log_error_message(const char* prefix, krb5_context context, krb5_error_code err)
{
	char *errmsg = ka_get_error_message(context, err);

	g_warning("%s: %s", prefix, errmsg);
	g_free (errmsg);
}


static gboolean
credentials_expiring_real (KaApplet* applet)
{
	krb5_creds my_creds;
	krb5_timestamp now;
	gboolean retval = FALSE;

	memset(&my_creds, 0, sizeof(my_creds));
	ka_applet_set_tgt_renewable(applet, FALSE);
	if (!ka_get_tgt_from_ccache (kcontext, &my_creds)) {
		creds_expiry = 0;
		retval = TRUE;
		goto out;
	}

	/* copy principal from cache if any */
	if (krb5_principal_compare (kcontext, my_creds.client, kprincipal)) {
		krb5_free_principal(kcontext, kprincipal);
		krb5_copy_principal(kcontext, my_creds.client, &kprincipal);
	}
	creds_expiry = my_creds.times.endtime;
	if ((krb5_timeofday(kcontext, &now) == 0) &&
	    (now + ka_applet_get_pw_prompt_secs(applet) > my_creds.times.endtime))
		retval = TRUE;

	/* If our creds are expiring, determine whether they are renewable.
	 * If the expiry is already at the renew_till time, don't consider
	 * credentials renewable */
	if (retval && get_cred_renewable(&my_creds)
	    && my_creds.times.renew_till > now
	    && my_creds.times.renew_till > creds_expiry) {
		ka_applet_set_tgt_renewable(applet, TRUE);
	}

out:
	krb5_free_cred_contents (kcontext, &my_creds);
	ka_applet_update_status(applet, creds_expiry);
	return retval;
}


/* time in seconds the tgt will be still valid */
int
ka_tgt_valid_seconds()
{
	krb5_timestamp now;

	if (krb5_timeofday(kcontext, &now))
		return 0;

	return (creds_expiry - now);
}


/* return credential cache filename, strip "FILE:" prefix if necessary */
static const char*
ka_ccache_filename (void)
{
	const gchar *name;

	name = krb5_cc_default_name (kcontext);
	if (g_str_has_prefix (name, "FILE:"))
		return strchr(name,':')+1;
	else if (g_str_has_prefix (name, "SCC:"))
		g_warning ("Cannot monitor sqlite based cache '%s'", name);
	else
		g_warning ("Unsupported cache type for '%s'", name);
	return NULL;
}


static void
ka_format_time (time_t t, gchar *ts, size_t len)
{
	g_strlcpy(ts, ctime(&t)+ 4, len);
	ts[15] = 0;
}


/* fill in service tickets data */
gboolean
ka_get_service_tickets (GtkListStore *tickets)
{
	krb5_cc_cursor cursor;
	krb5_creds creds;
	krb5_error_code ret;
	GtkTreeIter iter;
	krb5_ccache ccache;
	char *name;
	krb5_timestamp sec;
	gchar start_time[128], end_time[128], end_time_markup[256];
	gboolean retval = FALSE;

	gtk_list_store_clear(tickets);

	krb5_timeofday (kcontext, &sec);
	ret = krb5_cc_default (kcontext, &ccache);
	g_return_val_if_fail (!ret, FALSE);

	ret = krb5_cc_start_seq_get (kcontext, ccache, &cursor);
	if (ret) {
		ka_log_error_message("krb5_cc_start_seq_get", kcontext, ret);

		/* if the file doesn't exist, it's not an error if we can't
		 * parse it */
		if (!g_file_test(ka_ccache_filename (),
				 G_FILE_TEST_EXISTS))
			retval = TRUE;
		goto out;
	}

	while ((ret = krb5_cc_next_cred (kcontext,
					 ccache,
					 &cursor,
					 &creds)) == 0) {
		gboolean renewable, proxiable, forwardable;

		if (creds.times.starttime)
			ka_format_time(creds.times.starttime, start_time,
				       sizeof(start_time));
		else
			ka_format_time(creds.times.authtime, start_time,
				       sizeof(start_time));

		ka_format_time(creds.times.endtime, end_time,
			       sizeof(end_time));
		if (creds.times.endtime > sec)
			strcpy(end_time_markup, end_time);
		else
			g_snprintf(end_time_markup, sizeof(end_time_markup),
				  "%s <span foreground=\"red\" style=\"italic\">(%s)</span>",
				  end_time, _("Expired"));

		forwardable = get_cred_forwardable(&creds);
		renewable = get_cred_renewable(&creds);
		proxiable = get_cred_proxiable(&creds);

		ret = krb5_unparse_name (kcontext, creds.server, &name);
		if (!ret) {
			gtk_list_store_append(tickets, &iter);
			gtk_list_store_set(tickets, &iter,
					   PRINCIPAL_COLUMN, name,
					   START_TIME_COLUMN, start_time,
					   END_TIME_COLUMN, end_time_markup,
					   FORWARDABLE_COLUMN, forwardable,
					   RENEWABLE_COLUMN, renewable,
					   PROXIABLE_COLUMN, proxiable,
					   -1);
			free(name);
		} else
			ka_log_error_message("krb5_unparse_name", kcontext, ret);
		krb5_free_cred_contents (kcontext, &creds);
	}
	if(ret != KRB5_CC_END)
		ka_log_error_message("krb5_cc_get_next", kcontext, ret);

	ret = krb5_cc_end_seq_get (kcontext, ccache, &cursor);
	if (ret)
		ka_log_error_message("krb5_cc_end_seq_get", kcontext, ret);

	retval = TRUE;
out:
	ret = krb5_cc_close (kcontext, ccache);
	g_return_val_if_fail (!ret, FALSE);

	return retval;
}


/* Check for things we have to do while the password dialog is open */
static gboolean
krb5_auth_dialog_do_updates (gpointer data)
{
	KaApplet* applet = KA_APPLET(data);
	KaPwDialog* pwdialog = ka_applet_get_pwdialog(applet);

	g_return_val_if_fail (pwdialog != NULL, FALSE);
	/* Update creds_expiry and close the applet if we got the creds by other means (e.g. kinit) */
	if (!credentials_expiring_real(applet))
		ka_pwdialog_hide(pwdialog, FALSE);

	/* Update the expiry information in the dialog */
	ka_pwdialog_status_update (pwdialog);
	return TRUE;
}


static krb5_error_code
auth_dialog_prompter (krb5_context ctx G_GNUC_UNUSED,
                      void *data,
                      const char *name G_GNUC_UNUSED,
                      const char *banner G_GNUC_UNUSED,
                      int num_prompts,
                      krb5_prompt prompts[])
{
	KaApplet *applet = KA_APPLET(data);
	KaPwDialog *pwdialog = ka_applet_get_pwdialog(applet);
	krb5_error_code errcode;
	int i;

	errcode = KRB5KRB_ERR_GENERIC;
	canceled = FALSE;
	canceled_creds_expiry = 0;

	if (banner && !num_prompts)
            ka_applet_set_msg (applet, banner);

	for (i = 0; i < num_prompts; i++) {
		const gchar *password = NULL;
		int password_len = 0;
		int response;
		guint32 source_id;

		errcode = KRB5_LIBOS_CANTREADPWD;

		source_id = g_timeout_add_seconds (5, (GSourceFunc)krb5_auth_dialog_do_updates, applet);
		ka_pwdialog_setup (pwdialog, (gchar *) prompts[i].prompt, invalid_auth);
		response = ka_pwdialog_run (pwdialog);
		switch (response)
		{
			case GTK_RESPONSE_OK:
				password = ka_pwdialog_get_password(pwdialog);
				password_len = strlen (password);
				break;
			case GTK_RESPONSE_DELETE_EVENT:
			case GTK_RESPONSE_CANCEL:
				canceled = TRUE;
				break;
			case GTK_RESPONSE_NONE:
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
	ka_pwdialog_hide (pwdialog, TRUE);
	/* Reset this, so we know the next time we get a TRUE value, it is accurate. */
	invalid_auth = FALSE;

	return errcode;
}


#ifdef ENABLE_NETWORK_MANAGER
static void
ka_nm_client_state_changed_cb (NMClient *client,
                               GParamSpec *pspec G_GNUC_UNUSED,
                               gpointer data)
{
    NMState  state;
    gboolean *online = (gboolean*) data;

    state = nm_client_get_state(client);
    switch (state) {
        case NM_STATE_UNKNOWN:
        case NM_STATE_ASLEEP:
        case NM_STATE_CONNECTING:
            KA_DEBUG("Network state: %d", state);
	    /* do nothing */
            break;
        case NM_STATE_DISCONNECTED:
            KA_DEBUG("Network disconnected");
            *online = FALSE;
            break;
        case NM_STATE_CONNECTED:
            KA_DEBUG("Network connected");
            *online = TRUE;
            break;
    }
}
#endif

/* credentials expiring timer */
static gboolean
credentials_expiring (gpointer *data)
{
	KaApplet* applet = KA_APPLET(data);

	KA_DEBUG("Checking expiry <%ds", ka_applet_get_pw_prompt_secs(applet));
	if (credentials_expiring_real (applet) && is_online) {
		KA_DEBUG("Expiry @ %ld", creds_expiry);

		if (!ka_renew_credentials (applet))
			KA_DEBUG("Credentials renewed");
	}
	ka_applet_update_status(applet, creds_expiry);

	return TRUE;
}


/*
 * set ticket options by looking at krb5.conf and gconf
 */
static void
ka_set_ticket_options(KaApplet* applet, krb5_context context,
		      krb5_get_init_creds_opt *out,
		      const char* pk_userid G_GNUC_UNUSED,
		      const char* pk_anchors G_GNUC_UNUSED)
{
	gboolean flag;
#ifdef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_DEFAULT_FLAGS
	krb5_get_init_creds_opt_set_default_flags(context, PACKAGE,
		krb5_principal_get_realm(context, kprincipal), out);
#endif
	g_object_get(applet, "tgt-forwardable", &flag, NULL);
	if (flag)
		krb5_get_init_creds_opt_set_forwardable(out, flag);
	g_object_get(applet, "tgt-proxiable", &flag, NULL);
	if (flag)
		krb5_get_init_creds_opt_set_proxiable(out, flag);
	g_object_get(applet, "tgt-renewable", &flag, NULL);
	if (flag) {
		krb5_deltat r = 3600*24*30; /* 1 month */
		krb5_get_init_creds_opt_set_renew_life (out, r);
	}

#if ENABLE_PKINIT && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA
	/* pkinit optins for MIT Kerberos */
	if (pk_userid && strlen(pk_userid)) {
		KA_DEBUG("pkinit with '%s'", pk_userid);
		krb5_get_init_creds_opt_set_pa(context, out,
			"X509_user_identity", pk_userid);
		if (pk_anchors && strlen(pk_anchors)) {
			KA_DEBUG("pkinit anchors '%s'", pk_anchors);
			krb5_get_init_creds_opt_set_pa(context, out,
				"X509_anchors", pk_anchors);
		}
	}
#endif /* HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA */
}


#if ENABLE_PKINIT && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT
static krb5_error_code
ka_auth_heimdal_pkinit(KaApplet* applet, krb5_creds* creds,
                       const char* pk_userid, const char* pk_anchors)
{
	krb5_get_init_creds_opt *opts = NULL;
	krb5_error_code retval;
	const char* pkinit_anchors = NULL;

	KA_DEBUG("pkinit with '%s'", pk_userid);
	if (pk_anchors && strlen (pk_anchors)) {
		pkinit_anchors = pk_anchors;
		KA_DEBUG("pkinit anchors '%s'", pkinit_anchors);
	}

	if ((retval = krb5_get_init_creds_opt_alloc (kcontext, &opts)))
		goto out;

	ka_set_ticket_options (applet, kcontext, opts, NULL, NULL);
	retval = krb5_get_init_creds_opt_set_pkinit(kcontext, opts,
						    kprincipal,
						    pk_userid,
						    pkinit_anchors,
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
	if (opts)
		krb5_get_init_creds_opt_free(kcontext, opts);
	return retval;
}
#endif /* ! ENABLE_PKINIT */

static krb5_error_code
ka_auth_password(KaApplet* applet, krb5_creds* creds,
                 const char* pk_userid, const char* pk_anchors)
{
	krb5_error_code retval;
	krb5_get_init_creds_opt *opts = NULL;

	if ((retval = krb5_get_init_creds_opt_alloc (kcontext, &opts)))
		goto out;
	ka_set_ticket_options (applet, kcontext, opts,
	                       pk_userid, pk_anchors);

	retval = krb5_get_init_creds_password(kcontext, creds, kprincipal,
					      NULL, auth_dialog_prompter, applet,
					      0, NULL, opts);
out:
	if (opts)
		krb5_get_init_creds_opt_free(kcontext, opts);
	return retval;
}

static krb5_error_code
ka_parse_name(KaApplet* applet, krb5_context krbcontext, krb5_principal* kprinc)
{
	krb5_error_code ret;
	gchar *principal = NULL;

	if (*kprinc != NULL)
		krb5_free_principal(krbcontext, *kprinc);

	g_object_get(applet, "principal", &principal, NULL);
	ret = krb5_parse_name(krbcontext, principal, kprinc);

	g_free(principal);
	return ret;
}


/*
 * return current principal in text form
 *
 * caller needs to free the returned result using g_free();
 */
char*
ka_unparse_name ()
{
	char *princ, *gprinc = NULL;
	krb5_error_code err;

	if (!kprincipal)
		goto out;

	if ((err = krb5_unparse_name (kcontext, kprincipal, &princ))) {
		ka_log_error_message(__func__, kcontext, err);
		goto out;
	}

	gprinc = g_strdup (princ);
	free (princ);
out:
	return gprinc;
}


static void
ccache_changed_cb (GFileMonitor *monitor G_GNUC_UNUSED,
                   GFile *file,
                   GFile *other_file G_GNUC_UNUSED,
                   GFileMonitorEvent event_type,
                   gpointer data)
{
	KaApplet *applet = KA_APPLET(data);
	gchar *ccache_name = g_file_get_path(file);

	switch (event_type) {
		case G_FILE_MONITOR_EVENT_DELETED:
		case G_FILE_MONITOR_EVENT_CREATED:
		case G_FILE_MONITOR_EVENT_CHANGED:
			KA_DEBUG ("%s changed", ccache_name);
			credentials_expiring ((gpointer)applet);
			break;
		default:
			KA_DEBUG ("%s unhandled event: %d", ccache_name, event_type);
	}
	g_free (ccache_name);
}


static GFileMonitor*
monitor_ccache(KaApplet *applet)
{
	const gchar *ccache_name;
	GFile *ccache;
	GFileMonitor *monitor = NULL;
	GError *err = NULL;

	ccache_name = ka_ccache_filename ();
	g_return_val_if_fail (ccache_name != NULL, FALSE);

	ccache = g_file_new_for_path (ccache_name);
	monitor = g_file_monitor_file (ccache, G_FILE_MONITOR_NONE, NULL, &err);
	g_assert ((!monitor && err) || (monitor && !err));
	if (!monitor) {
		/* cache disappeared? */
		if (err->code == G_FILE_ERROR_NOENT)
			credentials_expiring ((gpointer)applet);
		else
			g_warning ("Failed to monitor %s: %s", ccache_name, err->message);
	} else {
		/* g_file_monitor_set_rate_limit(monitor, 10*1000); */
		g_signal_connect (monitor, "changed", G_CALLBACK (ccache_changed_cb), applet);
		KA_DEBUG ("Monitoring %s", ccache_name);
	}
	g_object_unref (ccache);
	g_clear_error (&err);
	return monitor;
}


/* grab credentials interactively */
static int
grab_credentials (KaApplet* applet)
{
	krb5_error_code retval = KRB5_KDC_UNREACH;
	krb5_creds my_creds;
	krb5_ccache ccache;
	gchar *pk_userid = NULL;
	gchar *pk_anchors = NULL;
	gchar *errmsg = NULL;
	gboolean pw_auth = TRUE;

	memset(&my_creds, 0, sizeof(my_creds));

	retval = ka_parse_name(applet, kcontext, &kprincipal);
	if (retval)
		goto out2;

	retval = krb5_cc_default (kcontext, &ccache);
	if (retval)
		goto out2;

	g_object_get(applet, "pk-userid", &pk_userid,
	                     "pk-anchors", &pk_anchors,
	                     NULL);
#if ENABLE_PKINIT && HAVE_HX509_ERR_H && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT
	/* pk_userid set: try pkinit */
	if (pk_userid && strlen(pk_userid)) {
		retval = ka_auth_heimdal_pkinit(applet, &my_creds,
		                                pk_userid, pk_anchors);
		/* other error than: "no token found" - no need to try password auth: */
		if (retval != HX509_PKCS11_NO_TOKEN && retval != HX509_PKCS11_NO_SLOT)
			pw_auth = FALSE;
	}
#endif /* ENABLE_PKINIT */
	if (pw_auth)
		retval = ka_auth_password(applet, &my_creds,
		                          pk_userid, pk_anchors);

	creds_expiry = my_creds.times.endtime;
	if (canceled)
		canceled_creds_expiry = creds_expiry;
	if (retval) {
		switch (retval) {
			case KRB5KDC_ERR_PREAUTH_FAILED:
			case KRB5KRB_AP_ERR_BAD_INTEGRITY:
#ifdef HAVE_HX509_ERR_H
			case HX509_PKCS11_LOGIN:
#endif 				/* Invalid password/pin, try again. */
				invalid_auth = TRUE;
				break;
			default:
				errmsg = ka_get_error_message(kcontext, retval);
				KA_DEBUG("Auth failed with %d: %s", retval,
					 errmsg);
				g_free(errmsg);
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

	memset(&my_creds, 0, sizeof(my_creds));
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
		krb5_free_cred_contents (kcontext, &my_creds);
		krb5_cc_close (kcontext, ccache);
		return -1;
	}

	if (ka_applet_get_tgt_renewable(applet)) {
		krb5_free_cred_contents (kcontext, &my_creds);
		retval = get_renewed_creds (kcontext, &my_creds, kprincipal, ccache, NULL);
		if (retval)
			goto out;

		retval = krb5_cc_initialize(kcontext, ccache, kprincipal);
		if(retval) {
			ka_log_error_message("krb5_cc_initialize", kcontext, retval);
			goto out;
		}
		retval = krb5_cc_store_cred(kcontext, ccache, &my_creds);
		if (retval) {
			ka_log_error_message("krb5_cc_store_cred", kcontext, retval);
			goto out;
		}
		ka_applet_signal_emit (applet, KA_SIGNAL_RENEWED_TGT,
				       my_creds.times.endtime);
	}
out:
	if (!retval)
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
using_krb5(void)
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


gboolean
ka_destroy_ccache (KaApplet *applet)
{
	krb5_ccache  ccache;
	const char* cache;
	krb5_error_code ret;

	cache = krb5_cc_default_name(kcontext);
	ret =  krb5_cc_resolve(kcontext, cache, &ccache);
	ret = krb5_cc_destroy (kcontext, ccache);

	credentials_expiring_real(applet);

	if (ret)
		return FALSE;
	else
		return TRUE;
}


/*
 * check if we have valid credentials for the requested principal - if not, grab them
 * principal: requested principal - if empty use default
 */
gboolean
ka_check_credentials (KaApplet *applet, const char* newprincipal)
{
	gboolean success = FALSE;
	int retval;
	char* principal;

	g_object_get(applet, "principal", &principal, NULL);

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
	int success = FALSE;
	KaPwDialog *pwdialog = ka_applet_get_pwdialog(applet);

	ka_pwdialog_set_persist(pwdialog, TRUE);
	do {
		retval = grab_credentials (applet);
		if (invalid_auth)
			continue;
		if (canceled)
			break;
		if (retval) {
			gchar *errmsg;

			errmsg = ka_get_error_message(kcontext, retval);
			ka_pwdialog_error(pwdialog, errmsg);
			g_free (errmsg);
			break;
		} else {
			success = TRUE;
			break;
		}
	} while(TRUE);

	ka_pwdialog_set_persist(pwdialog, FALSE);
	credentials_expiring_real(applet);

	return success;
}


static void
ka_secmem_init (void)
{
	/* Initialize secure memory.  1 is too small, so the default size
	will be used.  */
	secmem_init (1);
	secmem_set_flags (SECMEM_WARN);
	drop_privs ();

	if (atexit (secmem_term))
		g_error("Couln't register atexit handler");
}


static void
ka_nm_shutdown(void)
{
#ifdef ENABLE_NETWORK_MANAGER
    if (nm_client) {
        g_object_unref (nm_client);
        nm_client = NULL;
    }
#endif
}


static gboolean
ka_nm_init(void)
{
#ifdef ENABLE_NETWORK_MANAGER
    nm_client = nm_client_new();
    if (!nm_client) {
        g_warning ("Could not initialize nm-client");
    } else {
        g_signal_connect(nm_client, "notify::state",
                         G_CALLBACK(ka_nm_client_state_changed_cb),
                         &is_online);
	/* Set initial state */
	ka_nm_client_state_changed_cb(nm_client, NULL, &is_online);
    }
#endif /* ENABLE_NETWORK_MANAGER */
    return TRUE;
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
	GFileMonitor *monitor = NULL;

	context = g_option_context_new ("- Kerberos 5 credential checking");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, &error);

	if (error) {
		g_print ("%s\n%s\n",
			 error->message,
			 help_msg);
		g_clear_error (&error);
		return 1;
	}
	g_option_context_free (context);

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
		g_set_application_name (KA_NAME);

		applet = ka_applet_create ();
		if (!applet)
			return 1;
		ka_nm_init();

		g_timeout_add_seconds (CREDENTIAL_CHECK_INTERVAL, (GSourceFunc)credentials_expiring, applet);
		monitor = monitor_ccache (applet);

		ka_dbus_service(applet);
		gtk_main ();
	}
	ka_nm_shutdown();
	if (monitor)
		g_object_unref (monitor);
	return 0;
}
