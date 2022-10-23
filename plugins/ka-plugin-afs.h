/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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
