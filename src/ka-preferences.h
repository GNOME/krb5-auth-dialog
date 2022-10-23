/*
 * Copyright (C) 2022 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "config.h"
#include "ka-applet.h"

G_BEGIN_DECLS

#define KA_TYPE_PREFERENCES   (ka_preferences_get_type ())
G_DECLARE_FINAL_TYPE          (KaPreferences, ka_preferences, KA, PREFERENCES, AdwPreferencesWindow)

KaPreferences* ka_preferences_new (KaApplet *applet);
void ka_preferences_run (KaPreferences *self);


G_END_DECLS
