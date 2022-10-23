/*
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KA_DBUS_H
#define KA_DBUS_H

#include <glib.h>
#include "ka-applet-priv.h"

gboolean ka_dbus_connect (KaApplet *applet);
void ka_dbus_disconnect (void);
gboolean ka_dbus_acquire_tgt (KaApplet *applet,
                              const gchar *principal);
gboolean ka_dbus_destroy_ccache (KaApplet *applet);

#endif /* KA_DBUS_H */
