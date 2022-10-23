/*
 * Copyright (C) 2013 Zoran Pericic <zpericic@netst.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
