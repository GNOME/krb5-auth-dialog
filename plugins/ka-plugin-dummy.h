/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

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

KaPluginDummy *ka_plugin_dummy_new (void);

G_END_DECLS
