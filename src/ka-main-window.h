/*
 * (C) 2009 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <glib-object.h>

#include "ka-applet.h"

G_BEGIN_DECLS

enum ticket_columns {
    PRINCIPAL_COLUMN,
    START_TIME_COLUMN,
    END_TIME_COLUMN,
    FORWARDABLE_COLUMN,
    RENEWABLE_COLUMN,
    PROXIABLE_COLUMN,
    N_COLUMNS
};

#define KA_TYPE_MAIN_WINDOW (ka_main_window_get_type ())
G_DECLARE_FINAL_TYPE        (KaMainWindow, ka_main_window, KA, MAIN_WINDOW, GtkApplicationWindow)

KaMainWindow *ka_main_window_new (KaApplet *self);
void          ka_main_window_show (KaMainWindow *self, gboolean show_conf_tickets);

G_END_DECLS
