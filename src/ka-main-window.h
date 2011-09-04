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

#ifndef KA_TICKETS_H
#define KA_TICKETS_H

#include "ka-applet.h"

enum ticket_columns {
    PRINCIPAL_COLUMN,
    START_TIME_COLUMN,
    END_TIME_COLUMN,
    FORWARDABLE_COLUMN,
    RENEWABLE_COLUMN,
    PROXIABLE_COLUMN,
    N_COLUMNS
};


GtkWindow *ka_main_window_create (KaApplet *applet, GtkBuilder *xml);
void ka_main_window_show (void);
void ka_main_window_hide (void);


#endif
