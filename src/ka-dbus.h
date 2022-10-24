/*
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include "ka-applet-priv.h"

gboolean ka_dbus_connect (KaApplet        *applet,
                          GDBusConnection *connection,
                          const char      *object_path);
void ka_dbus_disconnect (void);
gboolean ka_dbus_acquire_tgt (KaApplet *applet,
                              const gchar *principal);
gboolean ka_dbus_destroy_ccache (KaApplet *applet);
