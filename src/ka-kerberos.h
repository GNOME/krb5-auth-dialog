/*
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "ka-applet-priv.h"

gboolean ka_kerberos_init (KaApplet *applet);
gboolean ka_kerberos_destroy (void);

gboolean ka_destroy_ccache (KaApplet* applet);
gboolean ka_grab_credentials(KaApplet* applet);
gboolean ka_check_credentials (KaApplet *applet, const char* principal);
gboolean ka_get_service_tickets(GtkListStore *tickets,
                                gboolean hide_service_tickets);
char* ka_unparse_name(void);
int ka_tgt_valid_seconds(void);
