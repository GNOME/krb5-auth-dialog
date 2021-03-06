/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
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


GtkApplicationWindow *ka_main_window_create (KaApplet *applet);
void ka_main_window_show (KaApplet *applet);
void ka_main_window_hide (void);


#endif
