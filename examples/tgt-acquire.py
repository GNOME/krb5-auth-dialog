#!/usr/bin/python3

import dbus
import sys

bus = dbus.SessionBus()
ka = bus.get_object('org.gnome.KrbAuthDialog',
                    '/org/gnome/KrbAuthDialog')
ret = ka.acquireTgt("", dbus_interface='org.gnome.KrbAuthDialog')
if not ret:
    print("Cannot acuire TGT, aborting.", file=sys.stderr)
