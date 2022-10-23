/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <ka-plugin.h>

G_BEGIN_DECLS
#define KA_TYPE_PLUGIN_PAM ka_plugin_pam_get_type()
#define KA_PLUGIN_PAM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PLUGIN_PAM, KaPluginPam))
#define KA_PLUGIN_PAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PLUGIN_PAM, KaPluginPamClass))
#define KA_IS_PLUGIN_PAM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PLUGIN_PAM))
#define KA_IS_PLUGIN_PAM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PLUGIN_PAM))
#define KA_PLUGIN_PAM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PLUGIN_PAM, KaPluginPamClass))

typedef struct {
    KaPlugin parent;
} KaPluginPam;

typedef struct {
    KaPluginClass parent_class;
} KaPluginPamClass;

GType ka_plugin_pam_get_type (void);

KaPluginPam *ka_plugin_pam_new (void);

G_END_DECLS
