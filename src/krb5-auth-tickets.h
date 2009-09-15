/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2009 Guido Guenther <agx@sigxcpu.org>
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

#ifndef KRB5_AUTH_TICKETS_H
#define KRB5_AUTH_TICKETS_H

enum ticket_columns {
	PRINCIPAL_COLUMN,
	START_TIME_COLUMN,
	END_TIME_COLUMN,
	N_COLUMNS
};


GtkWidget* ka_tickets_dialog_create(GtkBuilder *xml);
void ka_tickets_dialog_run(void);


#endif
