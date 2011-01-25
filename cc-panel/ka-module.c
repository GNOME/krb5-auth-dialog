#include <config.h>

#include "cc-ka-panel.h"

#include <glib/gi18n.h>

void
g_io_module_load (GIOModule *module)
{
  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  /* register the panel */
  cc_ka_panel_register (module);
}

void
g_io_module_unload (GIOModule *module G_GNUC_UNUSED)
{
}
