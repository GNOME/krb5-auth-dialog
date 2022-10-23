/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin.h"

enum {
	PROP_0,
	PROP_NAME,
	LAST_PROP
};

typedef struct _KaPluginPrivate KaPluginPrivate;
struct _KaPluginPrivate {
	char *name;
};
G_DEFINE_TYPE_WITH_PRIVATE (KaPlugin, ka_plugin, G_TYPE_OBJECT)

const char*
ka_plugin_get_name (KaPlugin *self)
{
	KaPluginPrivate *priv = ka_plugin_get_instance_private (self);

	g_return_val_if_fail (KA_IS_PLUGIN (self), NULL);
	priv = ka_plugin_get_instance_private (self);

	return priv->name;
}


static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	KaPluginPrivate *priv = ka_plugin_get_instance_private (KA_PLUGIN (object));

	switch (prop_id) {
	case PROP_NAME:
		/* construct only */
		priv->name = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
get_property (GObject *object, guint prop_id,
              GValue *value, GParamSpec *pspec)
{
	KaPluginPrivate *priv = ka_plugin_get_instance_private (KA_PLUGIN (object));

	switch (prop_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}


static void
finalize (GObject *object)
{
	KaPluginPrivate *priv = ka_plugin_get_instance_private (KA_PLUGIN (object));

	g_free (priv->name);
}


void
ka_plugin_activate (KaPlugin *self, KaApplet *applet)
{
        g_return_if_fail (KA_IS_PLUGIN (self));

        KA_PLUGIN_GET_CLASS (self)->activate (self, applet);
}


void
ka_plugin_deactivate (KaPlugin *self, KaApplet *applet)
{
        g_return_if_fail (KA_IS_PLUGIN (self));

        KA_PLUGIN_GET_CLASS (self)->deactivate (self, applet);
}


static void
ka_plugin_class_init (KaPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->finalize = finalize;

	g_object_class_install_property
			(object_class, PROP_NAME,
			g_param_spec_string (KA_PLUGIN_PROP_NAME,
			"Name",
			"Plugin Name",
			NULL,
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


static void
ka_plugin_init (KaPlugin *self G_GNUC_UNUSED)
{
}
