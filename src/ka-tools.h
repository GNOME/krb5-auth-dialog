/*
 * (C) 2009 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>
#include <ka-applet-priv.h>

G_BEGIN_DECLS

void ka_show_help (GtkWindow* window, const char* section);
void ka_show_about (KaApplet *applet);
void ka_window_destroy (gpointer window);

/* To ease GTK4 migration */
void        ka_editable_set_text (gpointer editable, const char *text);
const char *ka_editable_get_text (gpointer editable);

G_END_DECLS
