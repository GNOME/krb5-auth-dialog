subdir('scalable')
subdir('22x22')
subdir('48x48')

appstream_file = i18n.merge_file(
  input: '@0@.metainfo.xml.in'.format(app_id),
  output: '@0@.metainfo.xml'.format(app_id),
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

appstream_cli = find_program('appstreamcli', required: false)
if appstream_cli.found()
  test('Validate appstream file', appstream_cli,
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

man_conf = configuration_data()
man_conf.set('minimum_lifetime', minimum_lifetime)
man_conf.set('check_interval', check_interval)
install_man(configure_file(input: 'krb5-auth-dialog.1.in',
			   output: 'krb5-auth-dialog.1',
			   configuration: man_conf))
