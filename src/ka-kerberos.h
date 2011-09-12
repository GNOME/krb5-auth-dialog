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

#ifndef KA_DIALOG
#define KA_DIALOG

#include "ka-applet-priv.h"

gboolean ka_kerberos_init (KaApplet *applet);
gboolean ka_kerberos_destroy (void);

gboolean ka_destroy_ccache (KaApplet* applet);
gboolean ka_grab_credentials(KaApplet* applet);
gboolean ka_check_credentials (KaApplet *applet, const char* principal);
gboolean ka_get_service_tickets(GtkListStore *tickets);
char* ka_unparse_name(void);
int ka_tgt_valid_seconds(void);
#endif
