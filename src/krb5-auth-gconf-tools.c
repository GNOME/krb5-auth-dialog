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
#include <krb5-auth-gconf-tools.h>

gboolean
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


gboolean
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


gboolean
ka_gconf_get_bool (GConfClient* client,
		    const char* key,
		    gboolean* value)
{
	GError*		error = NULL;
	gboolean	success = FALSE;
	GConfValue*	gc_value;

	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);

	if ((gc_value = gconf_client_get (client, key, &error))) {
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

