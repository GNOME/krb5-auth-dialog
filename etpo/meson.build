if build_etpo

  run_data = configuration_data()
  run_data.set('ABS_BUILDDIR', meson.current_build_dir())
  run_data.set('ABS_TOP_SRCDIR', meson.source_root())
  configure_file(
    input: 'update-etpo.in',
    output: 'update-etpo',
    configuration: run_data)
  
  lgen = generator(flex,
                   output : '@PLAINNAME@.yy.c',
                   arguments : ['-o', '@OUTPUT@', '@INPUT@'])

  ggen = generator(bison,
                   output : ['@BASENAME@.c', '@BASENAME@.h'],
                   arguments : ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@'])

  lfiles = lgen.process('lexer.l')
  gfiles = ggen.process('grammar.y')

  executable('etpo', lfiles, gfiles,
             dependencies: glib_dep)
endif
