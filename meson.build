project('krb5-auth-dialog', 'c',
          version: '44.0.alpha1',
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

app_id = 'org.gnome.KrbAuthDialog'

glib_ver = '2.66'
glib_ver_str = 'GLIB_VERSION_@0@'.format(glib_ver.replace('.','_'))
glib_ver_cmp = '>=@0@'.format(glib_ver)

gtk_version = '4.8'
gtk_version_arr = gtk_version.split('.')
gtk_major = gtk_version_arr[0]
gtk_minor = gtk_version_arr[1]

add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver_str),
  '-DGLIB_VERSION_MAX_REQUIRED=@0@'.format(glib_ver_str),
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

adw_dep = dependency('libadwaita-1', version: '>=1.2')
glib_dep = dependency('glib-2.0', version: glib_ver_cmp)
gio_dep = dependency('gio-2.0', version: glib_ver_cmp)
gobject_dep = dependency('gobject-2.0', version: glib_ver_cmp)
gmodule_dep = dependency('gmodule-2.0', version: glib_ver_cmp)
gtk_dep = dependency('gtk4', version: '>=@0@'.format(gtk_version))

meson.add_install_script(
  join_paths('build-aux', 'post_install.py'),
  datadir
)

# Both heimdal and mit ship a krb5.pc:
krb5_dep    = dependency('krb5')

minimum_lifetime = get_option('minimum-lifetime')
check_interval = get_option('check-interval')

conf_data = configuration_data()
conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set('CREDENTIAL_CHECK_INTERVAL', check_interval)
conf_data.set('MINUTES_BEFORE_PROMPTING', minimum_lifetime)
conf_data.set_quoted('SC_PKCS11', get_option('pkcs11'))
conf_data.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf_data.set_quoted('LOCALE_DIR', localedir)
conf_data.set_quoted('KA_PLUGINS_DIR', pluginsdir)
conf_data.set_quoted('DATA_DIR', datadir)
conf_data.set_quoted('KA_APP_ID', app_id)

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

if cc.has_member('krb5_prompt', 'type',
                 prefix : '#include <krb5.h>',
		 dependencies: krb5_dep)
    conf_data.set('HAVE_KRB5_PROMPT_TYPE', 1)
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
  conf_data.set10('HAVE_HX509_ERR_H', true)
endif

have_creds_opt_set_pkinit = false
if cc.has_function('krb5_get_init_creds_opt_set_pkinit',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  have_creds_opt_set_pkinit = true
endif
conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PKINIT', have_creds_opt_set_pkinit)

have_creds_opt_set_pa = false
if cc.has_function('krb5_get_init_creds_opt_set_pa',
                   prefix: '''#include <krb5.h>''',
                   dependencies: krb5_dep)
  have_creds_opt_set_pa = true
  have_pkinit = true
endif
conf_data.set10('HAVE_KRB5_GET_INIT_CREDS_OPT_SET_PA', have_creds_opt_set_pa)

have_pkinit = have_creds_opt_set_pkinit or have_creds_opt_set_pa
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

subdir('data')
subdir('po')
subdir('src')
subdir('plugins')
subdir('help')
subdir('etpo')

summary({
           'Minimum Lifetime': minimum_lifetime,
           'Check Interval': check_interval,
           'pkinit': conf_data.get('ENABLE_PKINIT', 0),
           'pkcs11': conf_data.get('SC_PKCS11'),
           'pam-plugin': build_pam_plugin,
           'etpo': build_etpo,
})
