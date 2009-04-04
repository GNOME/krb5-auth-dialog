/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008,2009 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include "config.h"

#include <gconf/gconf-client.h>

#include "krb5-auth-applet.h"
#include "krb5-auth-gconf-tools.h"
#include "krb5-auth-gconf.h"

static gboolean
ka_gconf_set_principal (GConfClient* client, KaApplet* applet)
{
	gchar* principal = NULL;

	if(!ka_gconf_get_string (client, KA_GCONF_KEY_PRINCIPAL, &principal)) {
		principal = g_strdup (g_get_user_name());
	}
	g_object_set(applet, "principal", principal, NULL);
	g_free (principal);
	return TRUE;
}


static gboolean
ka_gconf_set_pk_userid (GConfClient* client, KaApplet* applet)
{
	gchar*  pk_userid = NULL;

	if(!ka_gconf_get_string (client, KA_GCONF_KEY_PK_USERID, &pk_userid)) {
		pk_userid = g_strdup ("");
	}
	g_object_set(applet, "pk_userid", pk_userid, NULL);
	g_free (pk_userid);
	return TRUE;
}


static gboolean
ka_gconf_set_prompt_mins (GConfClient* client, KaApplet* applet)
{
	gint prompt_mins = 0;

	if(!ka_gconf_get_int (client, KA_GCONF_KEY_PROMPT_MINS, &prompt_mins)) {
		prompt_mins = MINUTES_BEFORE_PROMPTING;
	}
	g_object_set(applet, "pw-prompt-mins", prompt_mins, NULL);
	return TRUE;
}


static gboolean
ka_gconf_set_show_trayicon (GConfClient* client, KaApplet* applet)
{
	gboolean show_trayicon = FALSE;

	if(!ka_gconf_get_bool(client, KA_GCONF_KEY_SHOW_TRAYICON, &show_trayicon)) {
		show_trayicon = TRUE;
	}
	g_object_set(applet, "show-trayicon", show_trayicon, NULL);
	return TRUE;
}


static gboolean
ka_gconf_set_tgt_forwardable (GConfClient* client, KaApplet* applet)
{
	gboolean forwardable = FALSE;

	if(!ka_gconf_get_bool(client, KA_GCONF_KEY_FORWARDABLE, &forwardable)) {
		forwardable = TRUE;
	}
	g_object_set(applet, "tgt-forwardable", forwardable, NULL);
	return TRUE;
}


static gboolean
ka_gconf_set_tgt_renewable (GConfClient* client, KaApplet* applet)
{
	gboolean renewable = FALSE;

	if(!ka_gconf_get_bool(client, KA_GCONF_KEY_RENEWABLE, &renewable)) {
		renewable = TRUE;
	}
	g_object_set(applet, "tgt-renewable", renewable, NULL);
	return TRUE;
}


static gboolean
ka_gconf_set_tgt_proxiable (GConfClient* client, KaApplet* applet)
{
	gboolean proxiable = FALSE;

	if(!ka_gconf_get_bool(client, KA_GCONF_KEY_PROXIABLE, &proxiable)) {
		proxiable = TRUE;
	}
	g_object_set(applet, "tgt-proxiable", proxiable, NULL);
	return TRUE;
}


static void
ka_gconf_key_changed_callback (GConfClient* client,
                               guint cnxn_id G_GNUC_UNUSED,
                               GConfEntry* entry,
                               gpointer user_data)
{
	const char* key;

	KaApplet* applet = KA_APPLET(user_data);
	key = gconf_entry_get_key (entry);
	if (!key)
		return;
	KA_DEBUG("Key %s changed", key);

	if (g_strcmp0 (key, KA_GCONF_KEY_PRINCIPAL) == 0) {
		ka_gconf_set_principal (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_PROMPT_MINS) == 0) {
		ka_gconf_set_prompt_mins (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_SHOW_TRAYICON) == 0) {
		ka_gconf_set_show_trayicon (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_PK_USERID) == 0) {
		ka_gconf_set_pk_userid (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_FORWARDABLE) == 0) {
		ka_gconf_set_tgt_forwardable (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_RENEWABLE) == 0) {
		ka_gconf_set_tgt_renewable (client, applet);
	} else if (g_strcmp0 (key, KA_GCONF_KEY_PROXIABLE) == 0) {
		ka_gconf_set_tgt_proxiable (client, applet);
	} else
		g_warning("Received notification for unknown gconf key %s", key);
	return;
}


gboolean
ka_gconf_init (KaApplet* applet,
               int argc G_GNUC_UNUSED,
               char* argv[] G_GNUC_UNUSED)
{
	GError *error = NULL;
	GConfClient* client;
	gboolean success = FALSE;

	client = gconf_client_get_default ();
	gconf_client_add_dir (client, KA_GCONF_PATH, GCONF_CLIENT_PRELOAD_ONELEVEL, &error);
	if (error)
		goto out;

	gconf_client_notify_add (client, KA_GCONF_PATH,
				 ka_gconf_key_changed_callback, applet, NULL, &error);
	if (error)
		goto out;

	/* setup defaults */
	ka_gconf_set_principal (client, applet);
	ka_gconf_set_prompt_mins (client, applet);
	ka_gconf_set_show_trayicon (client, applet);
	ka_gconf_set_pk_userid(client, applet);
	ka_gconf_set_tgt_forwardable(client, applet);
	ka_gconf_set_tgt_renewable(client, applet);
	ka_gconf_set_tgt_proxiable(client, applet);

	success = TRUE;
out:
	if(error) {
		g_print (error->message);
		g_error_free (error);
	}
	return success;
}
