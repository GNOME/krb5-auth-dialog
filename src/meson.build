desktop_file = 'org.gnome.KrbAuthDialog.desktop'
merged = i18n.merge_file('desktop',
    input: desktop_file + '.in',
    output: desktop_file,
    po_dir: '../po',
    install: true,
    install_dir: desktopdir,
    type: 'desktop'
)

merged = i18n.merge_file('autostart',
    input: desktop_file + '.in',
    output: 'krb5-auth-dialog.desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('sysconfdir'), 'xdg', 'autostart'),
    type: 'desktop'
)

serviceconf = configuration_data()
serviceconf.set('BINDIR', bindir)
service_file = 'org.gnome.KrbAuthDialog.service'
configure_file(
  input: service_file + '.in',
  output: service_file,
  configuration: serviceconf,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'dbus-1', 'services'),
)

appstream_file = i18n.merge_file(
  input: 'krb5-auth-dialog.appdata.xml.in',
  output: 'krb5-auth-dialog.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test('Validate appstream file', appstream_util,
    args: ['validate', '--nonet', appstream_file]
  )
endif

schema_src = 'org.gnome.KrbAuthDialog.gschema.xml'
compiled = gnome.compile_schemas(build_by_default: true,
                                 depend_files: files(schema_src))
install_data(schema_src,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test('Validate schema file', compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif

krb5_auth_dialog_resources = gnome.compile_resources(
  'krb5-auth-dialog-resource',
  'krb5-auth-dialog.gresource.xml',
  source_dir: 'resources',
  c_name: 'krb5_auth_dialog_resource',
)

krb5_auth_dialog_generated_sources = [
  configure_file,
  krb5_auth_dialog_resources,
]

krb5_auth_dialog_sources = [
  'dummy-strings.c',
  'ka-applet.c',
  'ka-closures.c',
  'ka-dbus.c',
  'ka-kerberos.c',
  'ka-main-window.c',
  'ka-plugin.c',
  'ka-plugin-loader.c',
  'ka-preferences.c',
  'ka-pwdialog.c',
  'ka-settings.c',
  'ka-tools.c',
  krb5_auth_dialog_generated_sources,
]

krb5_auth_dialog_deps = [
  gcr_dep,
  gio_dep,
  gmodule_dep,
  gobject_dep,
  gtk_dep,
  krb5_dep,
]

krb5_auth_dialog = executable(
  'krb5-auth-dialog',
  krb5_auth_dialog_sources,
  dependencies: krb5_auth_dialog_deps,
  install: true)
