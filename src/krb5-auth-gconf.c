/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
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
#include "krb5-auth-gconf.h"

#define KA_GCONF_PATH			"/apps/" PACKAGE
#define KA_GCONF_KEY_PRINCIPAL		KA_GCONF_PATH "/principal"
#define KA_GCONF_KEY_PROMPT_MINS	KA_GCONF_PATH "/prompt_minutes"
#define KA_GCONF_KEY_SHOW_TRAYICON	KA_GCONF_PATH "/show_trayicon"

static gboolean
ka_gconf_get_string (GConfClient* client,
		     const char* key,
		     char** value)
{
	GError*		error = NULL;
	gboolean	success = FALSE;
	GConfValue*	gc_value;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (*value == NULL, FALSE);

	if ((gc_value = gconf_client_get (client, key, &error))) {
		if (gc_value->type == GCONF_VALUE_STRING) {
			*value = g_strdup (gconf_value_get_string (gc_value));
			success = TRUE;
		} else if (error) {
				g_print (error->message);
				g_error_free (error);
		}
		gconf_value_free (gc_value);
	}
	return success;
}


static gboolean
ka_gconf_get_int (GConfClient* client,
		    const char* key,
		    int* value)
{
	GError*		error = NULL;
	gboolean	success = FALSE;
	GConfValue*	gc_value;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if ((gc_value = gconf_client_get (client, key, &error)))
	{
		if (gc_value->type == GCONF_VALUE_INT) {
			*value = gconf_value_get_int (gc_value);
			success = TRUE;
		} else if (error) {
				g_print (error->message);
				g_error_free (error);
		}
		gconf_value_free (gc_value);
	}
	return success;
}


static gboolean
ka_gconf_get_bool (GConfClient* client,
		    const char* key,
		    gboolean* value)
{
	GError*		error = NULL;
	gboolean	success = FALSE;
	GConfValue*	gc_value;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if ((gc_value = gconf_client_get (client, key, &error)))
	{
		if (gc_value->type == GCONF_VALUE_BOOL) {
			*value = gconf_value_get_bool (gc_value);
			success = TRUE;
		} else if (error) {
				g_print (error->message);
				g_error_free (error);
		}
		gconf_value_free (gc_value);
	}
	return success;
}


static gboolean
ka_gconf_set_principal (GConfClient* client, Krb5AuthApplet* applet)
{
	g_free (applet->principal);
	applet->principal = NULL;
	if(!ka_gconf_get_string (client, KA_GCONF_KEY_PRINCIPAL, &applet->principal)) {
		applet->principal = g_strdup (g_get_user_name());
	}
	KA_DEBUG("Setting principal to %s", applet->principal);
	// FIXME: need to send set-principal signal
	return TRUE;
}


static gboolean
ka_gconf_set_prompt_mins (GConfClient* client, Krb5AuthApplet* applet)
{
	if(!ka_gconf_get_int (client, KA_GCONF_KEY_PROMPT_MINS, &applet->pw_prompt_secs)) {
		applet->pw_prompt_secs = MINUTES_BEFORE_PROMPTING;
	}
	applet->pw_prompt_secs *= 60;
	KA_DEBUG("Setting prompting timer to %d seconds", applet->pw_prompt_secs);
	return TRUE;
}


static gboolean
ka_gconf_set_show_trayicon (GConfClient* client, Krb5AuthApplet* applet)
{
	if(!ka_gconf_get_bool(client, KA_GCONF_KEY_SHOW_TRAYICON, &applet->show_trayicon)) {
		applet->show_trayicon = TRUE;
	}
	KA_DEBUG("Show trayicon: %s", (applet->show_trayicon ? "yes" : "no" ));
	// FIXME: send show trayicon signal
	ka_show_tray_icon(applet);
	return TRUE;
}


static void
ka_gconf_key_changed_callback (GConfClient* client,
                               guint cnxn_id,
                               GConfEntry* entry,
                               gpointer user_data)
{
	const char* key;

	Krb5AuthApplet* applet = (Krb5AuthApplet*)user_data;
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
	} else
		g_warning("Received notification for unknown gconf key %s", key);
	return;
}


gboolean
ka_gconf_init (Krb5AuthApplet* applet, int argc, char* argv[])
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

	success = TRUE;
out:
	if(error) {
		g_print (error->message);
		g_error_free (error);
	}
	return success;
}
