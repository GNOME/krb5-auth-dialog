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

#include "ka-plugin.h"

G_DEFINE_TYPE (KaPlugin, ka_plugin, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN, KaPluginPrivate))

enum {
	PROP_0,
	PROP_NAME,
	LAST_PROP
};

typedef struct _KaPluginPrivate KaPluginPrivate;
struct _KaPluginPrivate {
	char *name;
};

const char*
ka_plugin_get_name (KaPlugin *self)
{
	g_return_val_if_fail (KA_IS_PLUGIN (self), NULL);
	KaPluginPrivate *priv = GET_PRIVATE (self);

	return priv->name;
}


static void
set_property (GObject *object, guint prop_id,
              const GValue *value, GParamSpec *pspec)
{
	KaPluginPrivate *priv = GET_PRIVATE (object);

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
	KaPluginPrivate *priv = GET_PRIVATE (object);

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
	KaPluginPrivate *priv = GET_PRIVATE (object);

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

	g_type_class_add_private (klass, sizeof (KaPluginPrivate));

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
