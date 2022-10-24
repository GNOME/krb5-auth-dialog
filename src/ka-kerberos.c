/*
 * Copyright (C) 2004,2005,2006 Red Hat, Inc.
 * Copyright (C) 2008,2009,2010,2011 Guido Guenther
 *
 * Author(s): Christopher Aillon <caillon@redhat.com>
 *            Guido Günther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

#include "ka-kerberos.h"
#include "ka-applet-priv.h"
#include "ka-pwdialog.h"
#include "ka-tools.h"
#include "ka-main-window.h"

#ifdef HAVE_HX509_ERR_H
#include <hx509_err.h>
#endif

static krb5_context kcontext;
static krb5_principal kprincipal;
static krb5_timestamp creds_expiry;
static gboolean canceled;
static gboolean invalid_auth;
static gboolean is_online = TRUE;
static gboolean kcontext_valid;
GFileMonitor *ccache_monitor;

static int ka_renew_credentials (KaApplet *applet);
static gboolean ka_get_tgt_from_ccache (krb5_context context,
                                        krb5_creds *creds);

/* YAY for different Kerberos implementations */
static int
get_cred_forwardable (krb5_creds *creds)
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
get_cred_renewable (krb5_creds *creds)
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
get_renewed_creds (krb5_context context,
                   krb5_creds *creds,
                   krb5_principal client,
                   krb5_ccache ccache, char *in_tkt_service)
{
#ifdef HAVE_KRB5_GET_RENEWED_CREDS
    return krb5_get_renewed_creds (context, creds, client, ccache,
                                   in_tkt_service);
#else
    return 1;                   /* XXX is there something better to return? */
#endif
}

static int
get_cred_proxiable (krb5_creds *creds)
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
get_principal_realm_length (krb5_principal p)
{
#if defined(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING)
    return strlen (p->realm);
#elif defined(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA)
    return p->realm.length;
#endif
}

static const char *
get_principal_realm_data (krb5_principal p)
{
#if defined(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING)
    return p->realm;
#elif defined(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA)
    return p->realm.data;
#endif
}

static void
ka_krb5_free_error_message (krb5_context context, const char *msg)
{
#if defined(HAVE_KRB5_FREE_ERROR_MESSAGE)
    krb5_free_error_message (context, msg);
#elif defined(HAVE_KRB5_FREE_ERROR_STRING)
    krb5_free_error_string (context, (char *) msg);
#else
#	error No way to free error string.
#endif
}

/*
 * Returns a descriptive error message or kerberos related error
 * returned pointer must be freed using g_free().
 */
static char *
ka_get_error_message (krb5_context context, krb5_error_code err)
{
    char *msg = NULL;

#if defined(HAVE_KRB5_GET_ERROR_MESSAGE)
    const char *krberr;

    krberr = krb5_get_error_message (context, err);
    msg = g_strdup (krberr);
    ka_krb5_free_error_message (context, krberr);
#else
#	error No detailed error message information
#endif
    if (msg == NULL)
        msg = g_strdup (_("unknown error"));
    return msg;
}


static void
ka_krb5_cc_clear_mcred (krb5_creds *mcred)
{
#if defined HAVE_KRB5_CC_CLEAR_MCRED
    krb5_cc_clear_mcred (mcred);
#else
    memset (mcred, 0, sizeof (krb5_creds));
#endif
}


/* ***************************************************************** */
/* ***************************************************************** */

/* log a kerberos error messge at the given log level */
static void
ka_log_error_message_at_level (GLogLevelFlags level,
                               const char *prefix,
                               krb5_context context,
                               krb5_error_code err)
{
    char *errmsg = ka_get_error_message (context, err);

    g_log (G_LOG_DOMAIN, level, "%s: %s", prefix, errmsg);
    g_free (errmsg);
}


/* log a kerberos error messge */
static void
ka_log_error_message (const char *prefix, krb5_context context,
                      krb5_error_code err)
{
    ka_log_error_message_at_level (G_LOG_LEVEL_ERROR, prefix, context, err);
}


static gboolean
credentials_expiring_real (KaApplet *applet)
{
    krb5_creds my_creds;
    krb5_timestamp now;
    gboolean retval = FALSE;

    if (!kcontext_valid)
        return retval;

    memset (&my_creds, 0, sizeof (my_creds));
    ka_applet_set_tgt_renewable (applet, FALSE);
    if (!ka_get_tgt_from_ccache (kcontext, &my_creds)) {
        creds_expiry = 0;
        retval = TRUE;
        goto out;
    }

    /* copy principal from cache if any */
    if (kprincipal == NULL ||
        krb5_principal_compare (kcontext, my_creds.client, kprincipal)) {
        if (kprincipal)
            krb5_free_principal (kcontext, kprincipal);
        krb5_copy_principal (kcontext, my_creds.client, &kprincipal);
    }
    creds_expiry = my_creds.times.endtime;
    if ((krb5_timeofday (kcontext, &now) == 0) &&
        (now + ka_applet_get_pw_prompt_secs (applet) >
         my_creds.times.endtime))
        retval = TRUE;

    /* If our creds are expiring, determine whether they are renewable.
     * If the expiry is already at the renew_till time, don't consider
     * credentials renewable */
    if (retval && get_cred_renewable (&my_creds)
        && my_creds.times.renew_till > now
        && my_creds.times.renew_till > creds_expiry) {
        ka_applet_set_tgt_renewable (applet, TRUE);
    }

  out:
    krb5_free_cred_contents (kcontext, &my_creds);
    ka_applet_update_status (applet, creds_expiry);
    return retval;
}

/**
 * ka_tgt_valid_seconds:
 *
 * Returns: The time in seconds the tgt will be still valid
 */
int
ka_tgt_valid_seconds (void)
{
    krb5_timestamp now;

    if (krb5_timeofday (kcontext, &now))
        return 0;

    return (creds_expiry - now);
}


/* return credential cache filename, strip "FILE:" prefix if necessary */
static const char *
ka_ccache_filename (void)
{
    const gchar *name;

    name = krb5_cc_default_name (kcontext);
    if (g_str_has_prefix (name, "FILE:"))
        return strchr (name, ':') + 1;
    else if (g_str_has_prefix (name, "SCC:"))
        g_warning ("Cannot monitor sqlite based cache '%s'", name);
    else
        g_warning ("Unsupported cache type for '%s'", name);
    return NULL;
}


static void
ka_format_time (time_t t, gchar *ts, size_t len)
{
    g_strlcpy (ts, ctime (&t) + 4, len);
    ts[15] = 0;
}


/**
 * ka_get_service_tickets:
 * @tickets: The tickets list store
 * @hide_conf_tickets: Whether to hide configuration principals
 *
 * Fill in service tickets data
 *
 * Returns: %TRUE on success
 */
gboolean
ka_get_service_tickets (GtkListStore * tickets, gboolean hide_conf_tickets)
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

    gtk_list_store_clear (tickets);

    krb5_timeofday (kcontext, &sec);
    ret = krb5_cc_default (kcontext, &ccache);
    g_return_val_if_fail (!ret, FALSE);

    ret = krb5_cc_start_seq_get (kcontext, ccache, &cursor);
    if (ret == KRB5_FCC_NOFILE || ret == ENOENT) {
        ka_log_error_message_at_level (G_LOG_LEVEL_INFO, "krb5_cc_start_seq_get", kcontext, ret);
        retval = TRUE;
        goto out;
    } else if (ret) {
        ka_log_error_message_at_level (G_LOG_LEVEL_DEBUG, "krb5_cc_start_seq_get", kcontext, ret);
        goto out;
    }

    while ((ret = krb5_cc_next_cred (kcontext, ccache, &cursor, &creds)) == 0) {
        gboolean renewable, proxiable, forwardable;

        if (hide_conf_tickets && krb5_is_config_principal (kcontext, creds.server)) {
            krb5_free_cred_contents (kcontext, &creds);
            continue;
        }

        if (creds.times.starttime)
            ka_format_time (creds.times.starttime, start_time,
                            sizeof (start_time));
        else
            ka_format_time (creds.times.authtime, start_time,
                            sizeof (start_time));

        ka_format_time (creds.times.endtime, end_time, sizeof (end_time));
        if (creds.times.endtime > sec)
            strcpy (end_time_markup, end_time);
        else
            g_snprintf (end_time_markup, sizeof (end_time_markup),
                        "%s <span foreground=\"red\" style=\"italic\">(%s)</span>",
                        end_time, _("Expired"));

        forwardable = get_cred_forwardable (&creds);
        renewable = get_cred_renewable (&creds);
        proxiable = get_cred_proxiable (&creds);

        ret = krb5_unparse_name (kcontext, creds.server, &name);
        if (!ret) {
            gtk_list_store_append (tickets, &iter);
            gtk_list_store_set (tickets, &iter,
                                PRINCIPAL_COLUMN, name,
                                START_TIME_COLUMN, start_time,
                                END_TIME_COLUMN, end_time_markup,
                                FORWARDABLE_COLUMN, forwardable,
                                RENEWABLE_COLUMN, renewable,
                                PROXIABLE_COLUMN, proxiable, -1);
            free (name);
        } else
            ka_log_error_message ("krb5_unparse_name", kcontext, ret);
        krb5_free_cred_contents (kcontext, &creds);
    }
    if (ret != KRB5_CC_END)
        ka_log_error_message ("krb5_cc_get_next", kcontext, ret);

    ret = krb5_cc_end_seq_get (kcontext, ccache, &cursor);
    if (ret)
        ka_log_error_message ("krb5_cc_end_seq_get", kcontext, ret);

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
    KaApplet *applet = KA_APPLET (data);
    KaPwDialog *pwdialog = ka_applet_get_pwdialog (applet);

    g_return_val_if_fail (pwdialog != NULL, FALSE);
    /* Update creds_expiry and close the applet if we got the creds by other means (e.g. kinit) */
    if (!credentials_expiring_real (applet))
        ka_pwdialog_hide (pwdialog, FALSE);

    /* Update the expiry information in the dialog */
    ka_pwdialog_status_update (pwdialog);
    return TRUE;
}


static krb5_error_code
auth_dialog_prompter (krb5_context ctx G_GNUC_UNUSED,
                      void *data,
                      const char *name G_GNUC_UNUSED,
                      const char *banner G_GNUC_UNUSED,
                      int num_prompts, krb5_prompt prompts[])
{
    KaApplet *applet = KA_APPLET (data);
    KaPwDialog *pwdialog = ka_applet_get_pwdialog (applet);
    krb5_error_code errcode;
    guint source_id;
    int i;

    errcode = KRB5KRB_ERR_GENERIC;
    canceled = FALSE;

    if (banner && !num_prompts)
        ka_applet_set_msg (applet, banner);

    KA_DEBUG ("Num prompts: %d", num_prompts);
    for (i = 0; i < num_prompts; i++) {
        const gchar *password = NULL;
        int password_len = 0;
        int response;

#ifdef HAVE_KRB5_PROMPT_TYPE
        KA_DEBUG ("prompt: %s, type: %d", prompts[i].prompt, prompts[i].type);
#else
        KA_DEBUG ("prompt: %s", prompts[i].prompt);
#endif
        errcode = KRB5_LIBOS_CANTREADPWD;

        source_id =
            g_timeout_add_seconds (5,
                                   (GSourceFunc) krb5_auth_dialog_do_updates,
                                   applet);
        ka_pwdialog_setup (pwdialog, prompts[i].prompt,
                           invalid_auth);
        response = ka_pwdialog_run (pwdialog);
        switch (response) {
        case GTK_RESPONSE_OK:
            password = ka_pwdialog_get_password (pwdialog);
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
        g_clear_handle_id (&source_id, g_source_remove);

        if (!password)
            goto cleanup;
        if (password_len + 1 > prompts[i].reply->length) {
            g_warning ("Password too long %d/%zd", password_len + 1,
                       (size_t)prompts[i].reply->length);
            goto cleanup;
        }

        memcpy (prompts[i].reply->data, (char *) password, password_len + 1);
        prompts[i].reply->length = password_len;
        errcode = 0;
    }
  cleanup:
    g_clear_handle_id (&source_id, g_source_remove);
    ka_pwdialog_hide (pwdialog, TRUE);
    /* Reset this, so we know the next time we get a TRUE value, it is accurate. */
    invalid_auth = FALSE;

    return errcode;
}


static void
ka_network_available_changed_cb (GNetworkMonitor *mon,
                                 GParamSpec *pspec G_GNUC_UNUSED,
                                 gpointer data)
{
    gboolean *online = (gboolean *) data;

    /* TODO: better bind to a property */
    *online = g_network_monitor_get_network_available (mon);
    KA_DEBUG ("Network state: %sline", *online ? "on" : "off");
}


/* credentials expiring timer */
static gboolean
credentials_expiring (gpointer data)
{
    KaApplet *applet = KA_APPLET (data);

    g_assert (KA_IS_APPLET (applet));
    KA_DEBUG ("Checking expiry <%ds", ka_applet_get_pw_prompt_secs (applet));
    if (credentials_expiring_real (applet) && is_online) {
        KA_DEBUG ("Expiry @ %ld", (long int)creds_expiry);

        if (!ka_renew_credentials (applet))
            KA_DEBUG ("Credentials renewed");
    }
    ka_applet_update_status (applet, creds_expiry);

    return G_SOURCE_CONTINUE;
}


/* run once, then terminate the timer */
static gboolean
credentials_expiring_once (gpointer data)
{
    credentials_expiring (data);
    return G_SOURCE_REMOVE;
}


/*
 * set ticket options by looking at krb5.conf and gsettings
 */
static void
ka_set_ticket_options (KaApplet *applet, krb5_context context,
                       krb5_get_init_creds_opt * out,
                       const char *pk_userid G_GNUC_UNUSED,
                       const char *pk_anchors G_GNUC_UNUSED)
{
    gboolean flag;

#ifdef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_DEFAULT_FLAGS
    krb5_get_init_creds_opt_set_default_flags (context, PACKAGE,
                                               krb5_principal_get_realm
                                               (context, kprincipal), out);
#endif
    g_object_get (applet, KA_PROP_NAME_TGT_FORWARDABLE, &flag, NULL);
    if (flag)
        krb5_get_init_creds_opt_set_forwardable (out, flag);
    g_object_get (applet, KA_PROP_NAME_TGT_PROXIABLE, &flag, NULL);
    if (flag)
        krb5_get_init_creds_opt_set_proxiable (out, flag);
    g_object_get (applet, KA_PROP_NAME_TGT_RENEWABLE, &flag, NULL);
    if (flag) {
        krb5_deltat r = 3600 * 24 * 30; /* 1 month */

        krb5_get_init_creds_opt_set_renew_life (out, r);
    }
#if ENABLE_PKINIT && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA
    /* pkinit options for MIT Kerberos */
    if (pk_userid && strlen (pk_userid)) {
        KA_DEBUG ("pkinit with '%s'", pk_userid);
        krb5_get_init_creds_opt_set_pa (context, out,
                                        "X509_user_identity", pk_userid);
        if (pk_anchors && strlen (pk_anchors)) {
            KA_DEBUG ("pkinit anchors '%s'", pk_anchors);
            krb5_get_init_creds_opt_set_pa (context, out,
                                            "X509_anchors", pk_anchors);
        }
    }
#endif /* HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA */
}


#if ENABLE_PKINIT && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT
static krb5_error_code
ka_auth_heimdal_pkinit (KaApplet *applet, krb5_creds *creds,
                        const char *pk_userid, const char *pk_anchors)
{
    krb5_get_init_creds_opt *opts = NULL;
    krb5_error_code retval;
    const char *pkinit_anchors = NULL;
    krb5_init_creds_context ctx = NULL;

    KA_DEBUG ("pkinit with '%s'", pk_userid);
    if (pk_anchors && strlen (pk_anchors)) {
        pkinit_anchors = pk_anchors;
        KA_DEBUG ("pkinit anchors '%s'", pkinit_anchors);
    }

    if ((retval = krb5_get_init_creds_opt_alloc (kcontext, &opts)))
        goto out;

    ka_set_ticket_options (applet, kcontext, opts, NULL, NULL);
    retval = krb5_get_init_creds_opt_set_pkinit (kcontext, opts, kprincipal, pk_userid, pkinit_anchors, NULL, NULL, 0,  /* pk_use_enc_key */
                                                 auth_dialog_prompter, applet,  /* data */
                                                 NULL); /* passwd */
    KA_DEBUG ("pkinit returned with %d", retval);
    if (retval)
        goto out;

    retval = krb5_init_creds_init(kcontext, kprincipal, auth_dialog_prompter, NULL, 0, opts, &ctx);
    if (retval) {
        g_autofree char *msg = ka_get_error_message (kcontext, retval);

        g_warning ("krb5_init_creds_init failed: %s", msg);
        goto out;
    }

    retval = krb5_init_creds_get (kcontext, ctx);
    if (retval) {
        g_autofree char *msg = ka_get_error_message (kcontext, retval);

        g_warning ("krb5_init_creds_init failed: %s", msg);
        goto out;
    }

    retval = krb5_init_creds_get_creds (kcontext, ctx, creds);
    if (retval) {
        g_autofree char *msg = ka_get_error_message (kcontext, retval);

        g_warning ("krb5_init_creds_init failed: %s", msg);
        goto out;
    }

  out:
    if (ctx)
        krb5_init_creds_free (kcontext, ctx);
    if (opts)
        krb5_get_init_creds_opt_free (kcontext, opts);
    return retval;
}
#endif /* ! ENABLE_PKINIT */

static krb5_error_code
ka_auth_password (KaApplet *applet, krb5_creds *creds,
                  const char *pk_userid, const char *pk_anchors)
{
    krb5_error_code retval;
    krb5_get_init_creds_opt *opts = NULL;

    if ((retval = krb5_get_init_creds_opt_alloc (kcontext, &opts)))
        goto out;
    ka_set_ticket_options (applet, kcontext, opts, pk_userid, pk_anchors);

    retval = krb5_get_init_creds_password (kcontext, creds, kprincipal,
                                           NULL, auth_dialog_prompter, applet,
                                           0, NULL, opts);
  out:
    if (opts)
        krb5_get_init_creds_opt_free (kcontext, opts);
    return retval;
}

static krb5_error_code
ka_parse_name (KaApplet *applet, krb5_context krbcontext,
               krb5_principal * kprinc)
{
    krb5_error_code ret;
    const gchar *principal;

    if (*kprinc != NULL)
        krb5_free_principal (krbcontext, *kprinc);

    principal = ka_applet_get_principal (applet);
    if (principal[0] == '\0') {
        principal = g_get_user_name();
    }
    ret = krb5_parse_name (krbcontext, principal, kprinc);

    return ret;
}


/**
 * ka_unparse_name:
 *
 * Returns: The current principal in text form. The caller needs to free the
 *     returned result using g_free();
 */
char *
ka_unparse_name (void)
{
    char *princ, *gprinc = NULL;
    krb5_error_code err;

    if (!kprincipal)
        goto out;

    if ((err = krb5_unparse_name (kcontext, kprincipal, &princ))) {
        ka_log_error_message (__func__, kcontext, err);
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
                   GFileMonitorEvent event_type, gpointer data)
{
    KaApplet *applet = KA_APPLET (data);
    gchar *ccache_name = g_file_get_path (file);

    switch (event_type) {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_MOVED:
    case G_FILE_MONITOR_EVENT_RENAMED:
        KA_DEBUG ("%s changed", ccache_name);
        credentials_expiring ((gpointer) applet);
        g_signal_emit_by_name(applet, "krb-ccache-changed");
        break;
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_MOVED_IN:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
    default:
        KA_DEBUG ("%s unhandled event: %d", ccache_name, event_type);
    }
    g_free (ccache_name);
}


static GFileMonitor *
monitor_ccache (KaApplet *applet)
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
            credentials_expiring ((gpointer) applet);
        else
            g_warning ("Failed to monitor %s: %s", ccache_name, err->message);
    } else {
        /* g_file_monitor_set_rate_limit(monitor, 10*1000); */
        g_signal_connect (monitor, "changed", G_CALLBACK (ccache_changed_cb),
                          applet);
        KA_DEBUG ("Monitoring %s", ccache_name);
    }
    g_object_unref (ccache);
    g_clear_error (&err);
    return monitor;
}


/* grab credentials interactively */
static int
grab_credentials (KaApplet *applet)
{
    krb5_error_code retval = KRB5_KDC_UNREACH;
    krb5_creds my_creds;
    krb5_ccache ccache;
    gchar *pk_userid = NULL;
    gchar *pk_anchors = NULL;
    gchar *errmsg = NULL;
    gboolean pw_auth = TRUE;

    memset (&my_creds, 0, sizeof (my_creds));

    retval = ka_parse_name (applet, kcontext, &kprincipal);
    if (retval)
        goto out2;

    retval = krb5_cc_default (kcontext, &ccache);
    if (retval)
        goto out2;

    g_object_get (applet, KA_PROP_NAME_PK_USERID, &pk_userid,
                  "pk-anchors", &pk_anchors, NULL);
#if ENABLE_PKINIT && defined(HAVE_HX509_ERR_H) && HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT
    /* pk_userid set: try pkinit */
    if (pk_userid && strlen (pk_userid)) {
        retval = ka_auth_heimdal_pkinit (applet, &my_creds,
                                         pk_userid, pk_anchors);
        /* other error than: "no token found" - no need to try password auth: */
        if (retval != HX509_PKCS11_NO_TOKEN && retval != HX509_PKCS11_NO_SLOT)
            pw_auth = FALSE;
    }
#endif /* ENABLE_PKINIT */
    if (pw_auth)
        retval = ka_auth_password (applet, &my_creds, pk_userid, pk_anchors);

    creds_expiry = my_creds.times.endtime;
    if (retval) {
        switch (retval) {
        case KRB5KDC_ERR_PREAUTH_FAILED:
        case KRB5KRB_AP_ERR_BAD_INTEGRITY:
        case KRB5KRB_AP_ERR_MODIFIED:
        case KRB5_GET_IN_TKT_LOOP:
#ifdef HAVE_HX509_ERR_H
        case HX509_PKCS11_LOGIN:
#endif /* Invalid password/pin, try again. */
            invalid_auth = TRUE;
            break;
        default:
            errmsg = ka_get_error_message (kcontext, retval);
            KA_DEBUG ("Auth failed with %d: %s", retval, errmsg);
            g_free (errmsg);
            break;
        }
        goto out;
    }
    retval = krb5_cc_initialize (kcontext, ccache, kprincipal);
    if (retval)
        goto out;

    retval = krb5_cc_store_cred (kcontext, ccache, &my_creds);
    if (retval)
        goto out;
  out:
    krb5_free_cred_contents (kcontext, &my_creds);
    krb5_cc_close (kcontext, ccache);
  out2:
    g_free (pk_userid);
    return retval;
}

/* try to renew the credentials noninteractively */
static int
ka_renew_credentials (KaApplet *applet)
{
    krb5_error_code retval;
    krb5_creds my_creds;
    krb5_ccache ccache;

    memset (&my_creds, 0, sizeof (my_creds));
    if (kprincipal == NULL) {
        retval = ka_parse_name (applet, kcontext, &kprincipal);
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

    if (ka_applet_get_tgt_renewable (applet)) {
        krb5_free_cred_contents (kcontext, &my_creds);
        retval =
            get_renewed_creds (kcontext, &my_creds, kprincipal, ccache, NULL);
        if (retval)
            goto out;

        retval = krb5_cc_initialize (kcontext, ccache, kprincipal);
        if (retval) {
            ka_log_error_message ("krb5_cc_initialize", kcontext, retval);
            goto out;
        }
        retval = krb5_cc_store_cred (kcontext, ccache, &my_creds);
        if (retval) {
            ka_log_error_message ("krb5_cc_store_cred", kcontext, retval);
            goto out;
        }
        ka_applet_emit_renewed (applet, my_creds.times.endtime);
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

    ka_krb5_cc_clear_mcred (&pattern);

    if (krb5_cc_default (context, &ccache))
        return FALSE;

    if (krb5_cc_get_principal (context, ccache, &principal))
        goto out;

    if (krb5_build_principal_ext (context, &pattern.server,
                                  get_principal_realm_length (principal),
                                  get_principal_realm_data (principal),
                                  KRB5_TGS_NAME_SIZE,
                                  KRB5_TGS_NAME,
                                  get_principal_realm_length (principal),
                                  get_principal_realm_data (principal), 0)) {
        goto out_free_princ;
    }
    pattern.client = principal;
    if (!krb5_cc_retrieve_cred (context, ccache, 0, &pattern, creds))
        ret = TRUE;

    krb5_free_principal (context, pattern.server);
  out_free_princ:
    krb5_free_principal (context, principal);
  out:
    krb5_cc_close (context, ccache);
    return ret;
}

static gboolean
ka_krb5_context_init (void)
{
    krb5_error_code err;
    gboolean have_tgt = FALSE;
    krb5_creds creds;

    err = krb5_init_context (&kcontext);
    if (err)
        return FALSE;
    else
        kcontext_valid = TRUE;

    have_tgt = ka_get_tgt_from_ccache (kcontext, &creds);
    if (have_tgt) {
        krb5_copy_principal (kcontext, creds.client, &kprincipal);
        krb5_free_cred_contents (kcontext, &creds);
    }
    return have_tgt;
}


static void
ka_krb5_context_free (void)
{
    if (kcontext_valid == FALSE)
        return;

    kcontext_valid = FALSE;
    krb5_free_context (kcontext);
}


gboolean
ka_destroy_ccache (KaApplet *applet)
{
    krb5_ccache ccache;
    const char *cache;
    krb5_error_code ret;

    cache = krb5_cc_default_name (kcontext);
    ret = krb5_cc_resolve (kcontext, cache, &ccache);
    ret = krb5_cc_destroy (kcontext, ccache);

    credentials_expiring_real (applet);

    if (ret)
        return FALSE;
    else
        return TRUE;
}


/**
 * ka_check_credentials:
 * @applet: The applet
 * newprincipal: The requested principal - if empty string, use the default
 *
 * Check if we have valid credentials for the requested principal - if not, grab them.
 *
 * Returns: %TRUE if credentials were grabbed
 */
gboolean
ka_check_credentials (KaApplet *applet, const char *newprincipal)
{
    gboolean success = FALSE;
    int retval;
    const char *principal;

    principal = ka_applet_get_principal (applet);

    if (strlen (newprincipal)) {
        krb5_principal knewprinc;

        /* no ticket cache: is requested princ the one from our config? */
        if (!kprincipal && g_strcmp0 (principal, newprincipal)) {
            KA_DEBUG ("Requested principal %s not %s", principal,
                      newprincipal);
            return FALSE;
        }

        /* ticket cache: check if the requested principal is the one we have */
        retval = krb5_parse_name (kcontext, newprincipal, &knewprinc);
        if (retval) {
            g_warning ("Cannot parse principal '%s'", newprincipal);
            return FALSE;
        }
        if (kprincipal
            && !krb5_principal_compare (kcontext, kprincipal, knewprinc)) {
            KA_DEBUG ("Current Principal '%s' not '%s'", principal,
                      newprincipal);
            krb5_free_principal (kcontext, knewprinc);
            return FALSE;
        }
        krb5_free_principal (kcontext, knewprinc);
    }

    if (credentials_expiring_real (applet)) {
        if (is_online)
            success = ka_grab_credentials (applet);
        else
            success = FALSE;
    } else
        success = TRUE;
    return success;
}


/**
 * ka_grab_credentials:
 * @applet: The applet
 *
 * Initiate grabbing of credentials (e.g. xon "Get Ticket" button click)
 *
 * Returns: %TRUE if credentials were grabbed
 */
gboolean
ka_grab_credentials (KaApplet *applet)
{
    int retval;
    int success = FALSE;
    KaPwDialog *pwdialog = ka_applet_get_pwdialog (applet);

    ka_pwdialog_set_persist (pwdialog, TRUE);
    do {
        retval = grab_credentials (applet);
        if (invalid_auth)
            continue;
        if (canceled)
            break;
        if (retval) {
            g_autofree char *errmsg = g_strdup_printf("%s%s",
                                                      ka_get_error_message (kcontext, retval),
                                                      is_online ? "" : _(" (No network connection)"));
            ka_pwdialog_error (pwdialog, errmsg);
            break;
        } else {
            success = TRUE;
            break;
        }
    } while (TRUE);

    ka_pwdialog_set_persist (pwdialog, FALSE);
    credentials_expiring_real (applet);

    return success;
}


static void
ka_nm_init (void)
{
    GNetworkMonitor *mon = g_network_monitor_get_default ();

    g_signal_connect (mon, "notify::network-available",
                      G_CALLBACK (ka_network_available_changed_cb),
                      &is_online);
    /* Set initial state */
    ka_network_available_changed_cb (mon, NULL, &is_online);
}


gboolean
ka_kerberos_init (KaApplet *applet)
{
    gboolean ret;

    ret = ka_krb5_context_init ();
    ka_nm_init ();
    g_timeout_add_seconds (CREDENTIAL_CHECK_INTERVAL, credentials_expiring, applet);
    g_idle_add (credentials_expiring_once, applet);
    ccache_monitor = monitor_ccache (applet);
    return ret;
}


gboolean
ka_kerberos_destroy (void)
{
    g_clear_object (&ccache_monitor);

    ka_krb5_context_free ();
    return TRUE;
}

/*
 * vim:ts=4:sts=4:sw=4:et:
 */
