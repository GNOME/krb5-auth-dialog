/*
 *  Copyright (C) 2004 Red Hat, Inc.
 *  Authored by Christopher Aillon <caillon@redhat.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <time.h>
#include <krb5.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "config.h"

#define CREDENTIAL_CHECK_INTERVAL 30000 /* milliseconds */
#define SECONDS_BEFORE_PROMPTING 1800

static GladeXML *xml = NULL;
static krb5_context kcontext;
static krb5_principal kprincipal;
static char* defname = NULL;

static int renew_credentials ();

static int
whoami (const char **username)
{
  struct passwd *p;
  uid_t uid;

  uid = geteuid();
  if (!(p = getpwuid(uid)))
    {
      return -1;
    }

  *username = p->pw_name;

  return 0;
}

static void
setup_dialog(GladeXML *xml, GtkWidget *dialog, gchar *prompt)
{
  GtkWidget *entry;
  GtkWidget *label;

  /* Clear the password entry field */
  entry = glade_xml_get_widget (xml, "krb5_entry");
  gtk_entry_set_text (GTK_ENTRY (entry), "");

  /* Use the prompt label that krb5 provides us */
  label = glade_xml_get_widget (xml, "krb5_message_label");
  gtk_label_set_text (GTK_LABEL (label), prompt);

  /* Only hide, and not destroy the dialog, so we can re-use it later. */
  g_signal_connect (dialog, "response",
		    G_CALLBACK (gtk_widget_hide),
		    dialog);
}

static krb5_error_code
KRB5_CALLCONV
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
      setup_dialog(xml, dialog, (gchar *) prompts[i].prompt);

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

  return errcode;
}

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

  gboolean retval = FALSE;

  memset (&my_creds, 0, sizeof(my_creds));

  code = krb5_init_context (&kcontext);
  if (code)
    return FALSE;

  if ((code = krb5_cc_default(kcontext, &cache)))
    return FALSE;

  flags = 0;                /* turns off OPENCLOSE mode */
  if ((code = krb5_cc_set_flags(kcontext, cache, flags)))
    {
      if (code == KRB5_FCC_NOFILE) {
#ifdef KRB5_KRB4_COMPAT
        if (name == NULL)
	  do_v4_ccache(0);
#endif
      }
      exit(1);
    }

  if ((code = krb5_cc_get_principal(kcontext, cache, &princ)))
    exit(1);

  if ((code = krb5_unparse_name(kcontext, princ, &defname)))
    exit(1);

  if ((code = krb5_cc_start_seq_get(kcontext, cache, &cur)))
    exit(1);

  while (!(code = krb5_cc_next_cred(kcontext, cache, &cur, &my_creds)))
    {
      if (my_creds.times.endtime - time(0) < SECONDS_BEFORE_PROMPTING)
        retval = TRUE;
      krb5_free_cred_contents(kcontext, &my_creds);
    }
  if (code == KRB5_CC_END)
    {
      if ((code = krb5_cc_end_seq_get(kcontext, cache, &cur)))
	exit(1);
      flags = KRB5_TC_OPENCLOSE;  /* turns on OPENCLOSE mode, from klist.c */
      if ((code = krb5_cc_set_flags(kcontext, cache, flags)))
        exit(1);
#ifdef KRB5_KRB4_COMPAT
      if (name == NULL && !status_only)
        do_v4_ccache(0);
#endif
      if (exit_status)
	exit(exit_status);
    }
  else
    {
      exit(1);
    }

  return retval;
}

static gboolean
credentials_expiring (gpointer *data)
{
  if (credentials_expiring_real())
    renew_credentials();

  return TRUE;
}

static int
renew_credentials (void)
{
  krb5_error_code retval;
  krb5_get_init_creds_opt options;
  krb5_creds my_creds;
  krb5_ccache ccache;
  const char *username;

  memset(&my_creds, 0, sizeof(my_creds));
  krb5_get_init_creds_opt_init(&options);

  retval = krb5_init_context(&kcontext);
  if (retval)
    return retval;

  retval = whoami(&username);
  if (retval)
    return retval;

  retval = krb5_parse_name(kcontext, username, &kprincipal);
  if (retval)
    return retval;

  retval = krb5_get_init_creds_password(kcontext, &my_creds, kprincipal,
                                        NULL, krb5_gtk_prompter, 0,
                                        0,
                                        NULL,
                                        &options);
  if (retval)
    {
      if (retval == KRB5KRB_AP_ERR_BAD_INTEGRITY)
	{
	  /* Invalid password, try again. */
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

int
main (int argc, char *argv[])
{
  GtkWidget *dialog;

  gtk_init (&argc, &argv);

  xml = glade_xml_new (GLADEDIR "krb5-auth-dialog.glade", NULL, NULL);
  dialog = glade_xml_get_widget (xml, "krb5_dialog");

  g_timeout_add (CREDENTIAL_CHECK_INTERVAL, (GSourceFunc)credentials_expiring, NULL);
  credentials_expiring (NULL);
  gtk_main();

  return 0;
}
