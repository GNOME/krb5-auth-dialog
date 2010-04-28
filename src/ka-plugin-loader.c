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

#include <gconf/gconf-client.h>

#include "ka-plugin-loader.h"
#include "ka-plugin.h"
#include "ka-applet-priv.h"
#include "ka-gconf-tools.h"

#include <gmodule.h>

G_DEFINE_TYPE (KaPluginLoader, ka_plugin_loader, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), KA_TYPE_PLUGIN_LOADER, KaPluginLoaderPrivate))

typedef struct _KaPluginLoaderPrivate KaPluginLoaderPrivate;

struct _KaPluginLoaderPrivate {
	KaApplet *applet;
	GSList *active_plugins;
};


static KaPlugin*
load_plugin (const char *path)
{
	KaPlugin *plugin = NULL;
	GModule *module;
	KaPluginCreateFunc plugin_create_func;
	int *major_plugin_version, *minor_plugin_version;

	module = g_module_open (path, G_MODULE_BIND_LAZY);
	if (!module) {
		g_warning ("Could not load plugin %s: %s", path, g_module_error ());
		return NULL;
	}

	if (!g_module_symbol (module, "ka_plugin_major_version",(gpointer *) &major_plugin_version)) {
		g_warning ("Could not load plugin %s: Missing major version info", path);
		goto out;
	}

	if (*major_plugin_version != KA_PLUGIN_MAJOR_VERSION) {
		g_warning ("Could not load plugin %s: Plugin major version %d, %d is required",
				   path, *major_plugin_version, KA_PLUGIN_MAJOR_VERSION);
		goto out;
	}

	if (!g_module_symbol (module, "ka_plugin_minor_version", (gpointer *) &minor_plugin_version)) {
		g_warning ("Could not load plugin %s: Missing minor version info", path);
		goto out;
	}

	if (*minor_plugin_version != KA_PLUGIN_MINOR_VERSION) {
		g_warning ("Could not load plugin %s: Plugin minor version %d, %d is required",
				   path, *minor_plugin_version, KA_PLUGIN_MINOR_VERSION);
		goto out;
	}

	if (!g_module_symbol (module, "ka_plugin_create", (gpointer *) &plugin_create_func)) {
		g_warning ("Could not load plugin %s: %s", path, g_module_error ());
		goto out;
	}

	plugin = (*plugin_create_func) ();
	if (plugin) {
		g_object_weak_ref (G_OBJECT (plugin), (GWeakNotify) g_module_close, module);
		g_message ("Loaded plugin %s", ka_plugin_get_name (plugin));
	} else
		g_warning ("Could not load plugin %s: initialization failed", path);
out:
	if (!plugin)
		g_module_close (module);

	return plugin;
}


static void
load_plugins (KaPluginLoader *self)
{
	int i;
	KaPluginLoaderPrivate *priv = GET_PRIVATE (self);
	const char *pname;
	GConfClient *gconf;
	GSList *plugins = NULL;

	if (!g_module_supported ()) {
		g_warning ("GModules are not supported on your platform!");
		return;
	}
	gconf = ka_applet_get_gconf_client (priv->applet);

	/* For now we only load the plugins on program startup */
	ka_gconf_get_string_list(gconf, KA_GCONF_KEY_PLUGINS_ENABLED, &plugins);
	if (!plugins) {
		g_message ("No plugins to load");
		return ;
	}

	for (i=0; (pname = g_slist_nth_data (plugins, i)) != NULL; i++) {
		char *path;
		char *fname;
		KaPlugin *plugin;

		fname = g_strdup_printf("libka-plugin-%s.%s", pname, G_MODULE_SUFFIX);
		path = g_module_build_path (KA_PLUGINS_DIR, fname);

		plugin = load_plugin (path);
		if (plugin) {
			ka_plugin_activate(plugin, priv->applet);
			priv->active_plugins = g_slist_prepend (priv->active_plugins, plugin);
		}
		g_free (fname);
		g_free (path);
	}
	g_slist_free (plugins);
}


static void
deactivate_plugin(gpointer plugin, gpointer user_data)
{
	KaApplet *applet = KA_APPLET (user_data);

	KA_DEBUG ("Deactivating plugin %s", ka_plugin_get_name (plugin));
	ka_plugin_deactivate (plugin, applet);
}


static void
ka_plugin_loader_dispose(GObject *object)
{
	KaPluginLoader *self = KA_PLUGIN_LOADER(object);
	KaPluginLoaderPrivate *priv = GET_PRIVATE (self);
	GObjectClass *parent_class = G_OBJECT_CLASS (ka_plugin_loader_parent_class);

	/* We need to do this before dropping the ref on applet */
	g_slist_foreach (priv->active_plugins, deactivate_plugin, priv->applet);
	g_slist_free (priv->active_plugins);

	if (priv->applet)
		priv->applet = NULL;

	if (parent_class->dispose != NULL)
		parent_class->dispose (object);
}


static void
ka_plugin_loader_class_init (KaPluginLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->dispose = ka_plugin_loader_dispose;
	g_type_class_add_private (klass, sizeof (KaPluginLoaderPrivate));
}


static void
ka_plugin_loader_init (KaPluginLoader *self)
{
	KaPluginLoaderPrivate *priv = GET_PRIVATE (self);
	priv->active_plugins = NULL;
}


static KaPluginLoader*
ka_plugin_loader_new (void)
{
	return g_object_new (KA_TYPE_PLUGIN_LOADER, NULL);
}


KaPluginLoader*
ka_plugin_loader_create (KaApplet* applet)
{
	KaPluginLoader *loader;
	KaPluginLoaderPrivate *priv;

	loader = ka_plugin_loader_new();
	priv = GET_PRIVATE (loader);
	priv->applet = applet;
	load_plugins (loader);

	return loader;
}
