#!/usr/bin/python
#
# Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
"""Example on howto handle the different DBus signals emitted by
   krb5-auth-dialog"""

import datetime
import time
import subprocess
from optparse import OptionParser

import gobject
import dbus
import dbus.mainloop.glib

def print_info(action, principal, when):
    w = datetime.datetime.fromtimestamp(when)
    if options.verbose:
        print "Ticket %s. Principal %s expires %s" % (action, principal, time.asctime(w.timetuple()))

def run_action(cmd):
    if cmd:
        subprocess.call(cmd, shell=True)

def tgt_acquired_handler(principal, when):
    print_info("acquired", principal, when)
    run_action(options.acquired_action)

def tgt_renewed_handler(principal, when):
    print_info("renewed", principal, when)
    run_action(options.renewed_action)

def tgt_expired_handler(principal, when):
    if options.verbose:
        print "Principal %s expired" % principal
    run_action(options.expired_action)

if __name__ == '__main__':
    global options

    parser = OptionParser()
    parser.add_option("--acquired-action", dest="acquired_action",
                      help="action on ticket acquisition")
    parser.add_option("--renewed-action", dest="renewed_action",
                      help="action on ticket renewal")
    parser.add_option("--expired-action", dest="expired_action",
                      help="action on ticket expiration")
    parser.add_option("-q", "--quiet",
                      action="store_false", dest="verbose", default=True,
                      help="don't print status messages to stdout")

    (options, args) = parser.parse_args()

    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()
    bus.add_signal_receiver(tgt_renewed_handler, dbus_interface = "org.gnome.KrbAuthDialog", signal_name = "krb_tgt_renewed")
    bus.add_signal_receiver(tgt_acquired_handler, dbus_interface = "org.gnome.KrbAuthDialog", signal_name = "krb_tgt_acquired")
    bus.add_signal_receiver(tgt_expired_handler, dbus_interface = "org.gnome.KrbAuthDialog", signal_name = "krb_tgt_expired")

    loop = gobject.MainLoop()
    loop.run()
