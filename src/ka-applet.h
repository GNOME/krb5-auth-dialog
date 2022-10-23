/*
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KA_TYPE_APPLET  (ka_applet_get_type ())
G_DECLARE_FINAL_TYPE    (KaApplet, ka_applet, KA, APPLET, AdwApplication)

KaApplet          *ka_applet_new                    (void);
const char        *ka_applet_get_principal          (KaApplet *self);

G_END_DECLS
