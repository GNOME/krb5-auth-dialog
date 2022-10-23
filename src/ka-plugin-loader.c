/*
 * Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ka-plugin-loader.h"
#include "ka-plugin.h"
#include "ka-settings.h"
#include "ka-applet-priv.h"

#include <gmodule.h>



typedef struct _KaPluginLoaderPrivate KaPluginLoaderPrivate;

struct _KaPluginLoaderPrivate {
	KaApplet *applet;
	GSList *active_plugins;
};

G_DEFINE_TYPE_WITH_PRIVATE (KaPluginLoader, ka_plugin_loader, G_TYPE_OBJECT)


static void
module_close (GModule *module, GObject *unused G_GNUC_UNUSED)
{
    g_module_close (module);
}


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
		g_object_weak_ref (G_OBJECT (plugin), (GWeakNotify) module_close, module);
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
	KaPluginLoaderPrivate *priv = ka_plugin_loader_get_instance_private (self);
	GSettings *settings;
	char **plugins = NULL;

	if (!g_module_supported ()) {
		g_warning ("GModules are not supported on your platform!");
		return;
	}
	settings = g_settings_get_child(ka_applet_get_settings (priv->applet),
                                        KA_SETTING_CHILD_PLUGINS);

	/* For now we only load the plugins on program startup */
	plugins = g_settings_get_strv(settings,
                                      KA_SETTING_KEY_PLUGINS_ENABLED);

	if (!plugins) {
		g_message ("No plugins to load");
		return;
	}

	for (i = 0; plugins[i]; i++) {
		char *path;
		char *fname;
		KaPlugin *plugin;

		fname = g_strdup_printf("libka-plugin-%s.%s",
                                        plugins[i],
                                        G_MODULE_SUFFIX);
		path = g_module_build_path (KA_PLUGINS_DIR, fname);

		plugin = load_plugin (path);
		if (plugin) {
			ka_plugin_activate(plugin, priv->applet);
			priv->active_plugins = g_slist_prepend (priv->active_plugins, plugin);
		}
		g_free (fname);
		g_free (path);
	}
	g_strfreev (plugins);
	g_object_unref (settings);
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
	KaPluginLoaderPrivate *priv = ka_plugin_loader_get_instance_private (self);
	GObjectClass *parent_class = G_OBJECT_CLASS (ka_plugin_loader_parent_class);

	/* We need to do this before dropping the ref on applet */
	g_slist_foreach (priv->active_plugins, deactivate_plugin, priv->applet);
	g_slist_free (priv->active_plugins);

	if (priv->applet)
		priv->applet = NULL;

        parent_class->dispose (object);
}


static void
ka_plugin_loader_class_init (KaPluginLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->dispose = ka_plugin_loader_dispose;
}


static void
ka_plugin_loader_init (KaPluginLoader *self)
{
	KaPluginLoaderPrivate *priv = ka_plugin_loader_get_instance_private (self);

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
	priv = ka_plugin_loader_get_instance_private (loader);
	priv->applet = applet;
	load_plugins (loader);

	return loader;
}
