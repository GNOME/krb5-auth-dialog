/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KA_PLUGIN
#define _KA_PLUGIN

#include <glib-object.h>
#include "ka-applet.h"

G_BEGIN_DECLS

#define KA_PLUGIN_MAJOR_VERSION 0
#define KA_PLUGIN_MINOR_VERSION 0

#define KA_TYPE_PLUGIN ka_plugin_get_type()

#define KA_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN, KaPlugin))

#define KA_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN, KaPluginClass))

#define KA_IS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN))

#define KA_IS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN))

#define KA_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN, KaPluginClass))

#define KA_PLUGIN_PROP_NAME "name"

typedef struct {
  GObject parent;
} KaPlugin;

typedef struct {
  GObjectClass parent_class;

  void (*activate)   (KaPlugin *self, KaApplet* applet);
  void (*deactivate) (KaPlugin *self, KaApplet* applet);

  /* we'll add functions for prefs handling later */
  void (*dummy1) (KaPlugin *self, KaApplet* applet);
  void (*dummy2) (KaPlugin *self, KaApplet* applet);
} KaPluginClass;

GType ka_plugin_get_type (void);

KaPlugin* ka_plugin_new (void);

typedef KaPlugin *(*KaPluginCreateFunc) (void);
const char* ka_plugin_get_name (KaPlugin *self);
void ka_plugin_activate (KaPlugin *self, KaApplet *applet);
void ka_plugin_deactivate (KaPlugin *self, KaApplet *applet);

G_END_DECLS

#endif /* _KA_PLUGIN */
