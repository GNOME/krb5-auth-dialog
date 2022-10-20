
dummy_plugin_sources = [
  'ka-plugin-dummy.c',
  'ka-plugin-dummy.h',
]

plugin_deps = [adw_dep, gio_dep, gtk_dep]

shared_module('ka-plugin-dummy',
              dummy_plugin_sources,
              include_directories: src_inc,
              dependencies: plugin_deps,
              install: true,
              install_dir: pluginsdir)

if build_pam_plugin
  pam_plugin_sources = [
    'ka-plugin-pam.c',
    'ka-plugin-pam.h',
  ]

  shared_module('ka-plugin-pam',
                pam_plugin_sources,
                include_directories: src_inc,
                dependencies: plugin_deps,
                install: true,
                install_dir: pluginsdir)
endif

afs_plugin_sources = [
  'ka-plugin-afs.c',
  'ka-plugin-afs.h',
]

shared_module('ka-plugin-afs',
              afs_plugin_sources,
              include_directories: src_inc,
              dependencies: plugin_deps,
              install: true,
              install_dir: pluginsdir)

gnomelock_plugin_sources = [
  'ka-plugin-gnomelock.c',
  'ka-plugin-gnomelock.h',
]

shared_module('ka-plugin-gnomelock',
              gnomelock_plugin_sources,
              include_directories: src_inc,
              dependencies: plugin_deps,
              install: true,
              install_dir: pluginsdir)
