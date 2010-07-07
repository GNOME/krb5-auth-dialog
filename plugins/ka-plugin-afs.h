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
#define KA_TYPE_PLUGIN_AFS ka_plugin_afs_get_type()
#define KA_PLUGIN_AFS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN_AFS, KaPluginAfs))
#define KA_PLUGIN_AFS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN_AFS, KaPluginAfsClass))
#define KA_IS_PLUGIN_AFS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN_AFS))
#define KA_IS_PLUGIN_AFS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN_AFS))
#define KA_PLUGIN_AFS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN_AFS, KaPluginAfsClass))

typedef struct {
    KaPlugin parent;
} KaPluginAfs;

typedef struct {
    KaPluginClass parent_class;
} KaPluginAfsClass;

GType ka_plugin_afs_get_type (void);

KaPluginAfs *ka_plugin_afs_new (void);

G_END_DECLS
#endif /* _KA_PLUGIN_AFS */
