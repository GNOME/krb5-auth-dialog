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

#ifndef _KA_PLUGIN_LOADER
#define _KA_PLUGIN_LOADER

#include <glib-object.h>
#include "ka-applet.h"

G_BEGIN_DECLS

#define KA_TYPE_PLUGIN_LOADER ka_plugin_loader_get_type()

#define KA_PLUGIN_LOADER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN_LOADER, KaPluginLoader))

#define KA_PLUGIN_LOADER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN_LOADER, KaPluginLoaderClass))

#define KA_IS_PLUGIN_LOADER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN_LOADER))

#define KA_IS_PLUGIN_LOADER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN_LOADER))

#define KA_PLUGIN_LOADER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN_LOADER, KaPluginLoaderClass))

typedef struct {
  GObject parent;
} KaPluginLoader;

typedef struct {
  GObjectClass parent_class;
} KaPluginLoaderClass;

GType ka_plugin_loader_get_type (void);

KaPluginLoader* ka_plugin_loader_create (KaApplet *applet);

G_END_DECLS

#endif /* _KA_PLUGIN_LOADER */
