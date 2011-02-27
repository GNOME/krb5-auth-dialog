import dbus

bus = dbus.SessionBus()
ka = bus.get_object('org.gnome.KrbAuthDialog',
                    '/org/gnome/KrbAuthDialog')
ret = ka.destroyCCache(dbus_interface='org.gnome.KrbAuthDialog')
if not ret:
    print >>sys.stderr, "Could not destroy credentials cache"
