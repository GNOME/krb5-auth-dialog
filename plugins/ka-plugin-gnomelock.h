/*
 * Copyright (C) 2013 Zoran Pericic <zpericic@netst.org>
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

#ifndef _KA_PLUGIN_GNOMELOCK
#define _KA_PLUGIN_GNOMELOCK

#include "ka-plugin.h"

G_BEGIN_DECLS
#define KA_TYPE_PLUGIN_GNOMELOCK ka_plugin_gnomelock_get_type()
#define KA_PLUGIN_GNOMELOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN_GNOMELOCK, KaPluginGnomeLock))
#define KA_PLUGIN_GNOMELOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN_GNOMELOCK, KaPluginGnomeLockClass))
#define KA_IS_PLUGIN_GNOMELOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN_GNOMELOCK))
#define KA_IS_PLUGIN_GNOMELOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN_GNOMELOCK))
#define KA_PLUGIN_GNOMELOCK_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN_GNOMELOCK, KaPluginGnomeLockClass))

typedef struct {
    KaPlugin parent;
} KaPluginGnomeLock;

typedef struct {
    KaPluginClass parent_class;
} KaPluginGnomeLockClass;

GType ka_plugin_gnomelock_get_type (void);

KaPluginGnomeLock *ka_plugin_gnomelock_new (void);

G_END_DECLS
#endif /* _KA_PLUGIN_GNOMELOCK */
