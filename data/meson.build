subdir('scalable')
subdir('22x22')
subdir('48x48')

appstream_file = i18n.merge_file(
  input: 'krb5-auth-dialog.metainfo.xml.in',
  output: 'krb5-auth-dialog.metainfo.xml',
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
