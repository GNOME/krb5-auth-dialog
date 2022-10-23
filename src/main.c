/*
 * Copyright (C) 2022 Guido Günther
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "config.h"

#include <glib/gi18n.h>

#include "ka-applet-priv.h"

int
main (int argc, char *argv[])
{
    g_autoptr(KaApplet) applet = NULL;

    textdomain (PACKAGE);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    bindtextdomain (PACKAGE, LOCALE_DIR);

    g_set_application_name (KA_NAME);

    gtk_init ();
    applet = ka_applet_new ();

    return g_application_run (G_APPLICATION(applet), argc, argv);
}
