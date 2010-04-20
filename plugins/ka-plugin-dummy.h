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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _KA_PLUGIN_DUMMY
#define _KA_PLUGIN_DUMMY

#include "ka-plugin.h"

G_BEGIN_DECLS

#define KA_TYPE_PLUGIN_DUMMY ka_plugin_dummy_get_type()

#define KA_PLUGIN_DUMMY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN_DUMMY, KaPluginDummy))

#define KA_PLUGIN_DUMMY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN_DUMMY, KaPluginDummyClass))

#define KA_IS_PLUGIN_DUMMY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN_DUMMY))

#define KA_IS_PLUGIN_DUMMY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN_DUMMY))

#define KA_PLUGIN_DUMMY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN_DUMMY, KaPluginDummyClass))

typedef struct {
  KaPlugin parent;
} KaPluginDummy;

typedef struct {
  KaPluginClass parent_class;
} KaPluginDummyClass;

GType ka_plugin_dummy_get_type (void);

KaPluginDummy* ka_plugin_dummy_new (void);

G_END_DECLS

#endif /* _KA_PLUGIN_DUMMY */
