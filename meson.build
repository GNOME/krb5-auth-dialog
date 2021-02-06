project('krb5-auth-dialog', 'c',
          version: '3.26.1',
          license: 'GPLv2-or-later',
    meson_version: '>= 0.53.0',
  default_options: [ 'warning_level=1', 'buildtype=debugoptimized', 'c_std=gnu11' ],
)

prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
datadir = join_paths(prefix, get_option('datadir'))
localedir = join_paths(prefix, get_option('localedir'))
libdir = join_paths(prefix, get_option('libdir'))
desktopdir = join_paths(datadir, 'applications')
pluginsdir = join_paths(libdir, 'krb5-auth-dialog/plugins')

glib_ver = 'GLIB_VERSION_2_58'

add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DG_LOG_DOMAIN="KrbAuthDialog"',
  '-I' + meson.build_root(),
], language: 'c')

root_inc = include_directories('.')
src_inc = include_directories('src')

cc = meson.get_compiler('c')

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Wdate-time',
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Wendif-labels',
  '-Werror=incompatible-pointer-types',
  '-Werror=missing-declarations',
  '-Werror=overflow',
  '-Werror=return-type',
  '-Werror=shift-count-overflow',
  '-Werror=shift-overflow=2',
  '-Werror=implicit-fallthrough=3',
  '-Wfloat-equal',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wmissing-field-initializers',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wtype-limits',
  '-Wundef',
  '-Wunused-function',
]
if get_option('buildtype') != 'plain'
  test_c_args += '-fstack-protector-strong'
endif

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach
add_project_arguments(
  global_c_args,
  language: 'c'
)

gnome = import('gnome')
i18n  = import('i18n')

glib_dep = dependency('glib-2.0', version: '>=2.58')
gio_dep = dependency('gio-2.0', version: '>=2.58')
gtk_dep = dependency('gtk+-3.0', version: '>=3.14')
gobject_dep = dependency('gobject-2.0', version: '>=2.50.0')
gmodule_dep = dependency('gmodule-2.0')
gcr_dep = dependency('gcr-3', version: '>= 3.5.5')

meson.add_install_script(
  join_paths('build-aux', 'post_install.py'),
  datadir
)

# Both heimdal and mit ship a krb5.pc:
krb5_dep    = dependency('krb5')

conf_data = configuration_data()
conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set('CREDENTIAL_CHECK_INTERVAL', get_option('check-interval'))
conf_data.set('MINUTES_BEFORE_PROMPTING', get_option('minimum-lifetime'))
conf_data.set_quoted('SC_PKCS11', get_option('pkcs11'))
conf_data.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf_data.set_quoted('LOCALE_DIR', localedir)
conf_data.set_quoted('KA_PLUGINS_DIR', pluginsdir)
conf_data.set_quoted('DATA_DIR', datadir)

# Hooray to different Kerberos implementations:
if cc.has_member('krb5_creds', 'ticket_flags',
		 prefix: '''#include <krb5.h>''',
		 dependencies: krb5_dep)
  # MIT
  conf_data.set('HAVE_KRB5_CREDS_TICKET_FLAGS', 1)
elif cc.has_member('krb5_creds', 'flags',
		   prefix: '''#include <krb5.h>''',
		   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_CREDS_FLAGS', 1)

  if cc.has_member('krb5_creds', 'flags.b.forwardable',
		   prefix: '''#include <krb5.h>''',
		   dependencies: krb5_dep)
    conf_data.set('HAVE_KRB5_CREDS_FLAGS_B_FORWARDABLE', 1)
  endif

  if cc.has_member('krb5_creds', 'flags.b.renewable',
		   prefix: '''#include <krb5.h>''',
		   dependencies: krb5_dep)
    conf_data.set('HAVE_KRB5_CREDS_FLAGS_B_RENEWABLE', 1)
  endif

  if cc.has_member('krb5_creds', 'flags.b.proxiable',
		   prefix: '''#include <krb5.h>''',
		   dependencies: krb5_dep)
    conf_data.set('HAVE_KRB5_CREDS_FLAGS_B_PROXIABLE', 1)
  endif
endif

if cc.compiles('''
    #include <krb5.h>
    int main(int argc, char **argv)
    {
        static krb5_principal foo;
        static krb5_data bar;
        foo->realm = bar;
        return 0;
    }''',
	       name: 'Realm as data',
	       dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_PRINCIPAL_REALM_AS_DATA', 1)
elif cc.compiles('''
    #include <krb5.h>
    #include <string.h>
    int main(int argc, char **argv)
    {
        static krb5_principal foo;
        return strlen(foo->realm);
    }''',
		 name: 'Realm as string',
		 dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_PRINCIPAL_REALM_AS_STRING', 1)
endif

if cc.has_function('krb5_get_error_message',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_GET_ERROR_MESSAGE', 1)
endif

if cc.has_function('krb5_free_error_message',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_FREE_ERROR_MESSAGE', 1)
endif

if cc.has_function('krb5_free_error_string',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_FREE_ERROR_STRING', 1)
endif

if cc.has_function('krb5_get_renewed_creds',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_GET_RENEWED_CREDS', 1)
endif

if cc.has_function('krb5_get_init_creds_opt_set_default_flags',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_DEFAULT_FLAGS', 1)
endif

if cc.has_function('krb5_cc_clear_mcred',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set('HAVE_KRB5_CC_CLEAR_MCRED', 1)
endif

if cc.has_header('hx509_err.h',
                 prefix: '''#include <krb5.h>''',
                 dependencies: krb5_dep)
  conf_data.set10('HAVE_HX509_ERR_H', 1)
endif

have_pkinit = false
conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT', 0)
if cc.has_function('krb5_get_init_creds_opt_set_pkinit',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT', 1)
  have_pkinit = true
endif

conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA', 0)
if cc.has_function('krb5_get_init_creds_opt_set_pa',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA', 1)
  have_pkinit = true
endif

want_pkinit = get_option('pkinit')
if not want_pkinit.disabled()
  if have_pkinit == true
    conf_data.set('ENABLE_PKINIT', 1)
  elif want_pkinit.enabled()
    error('Neither Heimdal nor MIT pkinit available')
  endif
endif

want_pam_plugin = get_option('pam-plugin')
pam_dep = cc.find_library('pam', required: want_pam_plugin)
build_pam_plugin = pam_dep.found() and not want_pam_plugin.disabled()

configure_file = configure_file(configuration: conf_data, output: 'config.h')

want_etpo = get_option('etpo')
flex = find_program('flex', required: want_etpo)
bison = find_program('bison', required: want_etpo)
build_etpo = flex.found() and bison.found()

subdir('icons')
subdir('po')
subdir('src')
subdir('plugins')
subdir('help')
subdir('etpo')

summary({
           'Minimum Lifetime': conf_data.get('MINUTES_BEFORE_PROMPTING'),
           'Check Interval': conf_data.get('CREDENTIAL_CHECK_INTERVAL'),
           'pkinit': conf_data.get('ENABLE_PKINIT', 0),
           'pkcs11': conf_data.get('SC_PKCS11'),
           'pam-plugin': build_pam_plugin,
           'etpo': build_etpo,
})
