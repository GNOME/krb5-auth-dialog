[krb5-auth-dialog](https://gitlab.gnome.org/GNOME/krb5-auth-dialog/)
is a simple dialog that monitors Kerberos tickets, and pops up a
dialog when they are about to expire.

Configuration
=============
Configuration settings are handled via GSettings.

You can set the principal that is used to acquire tickets via:

```sh
gsettings set org.gnome.KrbAuthDialog principal "principal@YOUR.REALM"
```

You can set the time of the first password prompt via:

```sh
gsettings set org.gnome.KrbAuthDialog prompt-minutes 30
```

You can set the principals pkinit identifier via:

```sh
gsettings set org.gnome.KrbAuthDialog pk-userid "FILE:/path/to/user.pem,/path/to/user.key"
```

or if you're using a smartcard:

```sh
gsettings set org.gnome.KrbAuthDialog pk-userid "PKCS11:/usr/lib/opensc/opensc-pkcs11.so"
```

All of these settings are also be set via the applications preferences dialog.

DBus API
========
You can request a ticket granting ticket via DBus:

```sh
gdbus call -e -d org.gnome.KrbAuthDialog -o /org/gnome/KrbAuthDialog -m org.gnome.KrbAuthDialog.acquireTgt 'principal'
```

If the sent principal doesn't match the one currently in the ticket cache the
request fails. To request a TGT for the "default" principal use the empty
string ''.

See [examples/tgt-signals.py](examples/tgt-signals.py) for information
about sent DBus signals.

Plugins
=======
Plugins are currently disabled by default. Individual plugins can be enabled via gsettings:

Enable pam and dummy plugions:

```
gsettings set org.gnome.KrbAuthDialog.plugins enabled "['pam', 'dummy']"
```

To list currently enabled plugins:

```
gsettings get org.gnome.KrbAuthDialog.plugins enabled
```

A Note on Translations
======================
Kerberos doesn't translate either its prompts or its error messages.
As the prompt is very visible, we need to translate it externally.  To
do this, the etpo binary in etpo/ can be used to extract the public
strings that Kerberos uses.  We are checking that in for now, until
Kerberos gets translated.

If your language doesn't have a translation yet and you want to provide one do a

```
	meson . _build
	ninja -C _build krb5-auth-dialog-update-po
```

in the source tree to get a template of translatable strings.

