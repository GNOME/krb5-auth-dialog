#!/usr/bin/python3

import dbus
import sys


bus = dbus.SessionBus()
ka = bus.get_object('org.gnome.KrbAuthDialog',
                    '/org/gnome/KrbAuthDialog')
ret = ka.destroyCCache(dbus_interface='org.gnome.KrbAuthDialog')
if not ret:
    print("Could not destroy credentials cache", file=sys.stderr)
