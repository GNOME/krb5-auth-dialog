import dbus

bus = dbus.SessionBus()
ka = bus.get_object('org.gnome.KrbAuthDialog',
                    '/org/gnome/KrbAuthDialog')
ret = ka.acquireTgt("", dbus_interface='org.gnome.KrbAuthDialog')
if not ret:
    print >>sys.stderr, "Cannot acuire TGT, aborting."
