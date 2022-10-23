#!/usr/bin/python3
#
# Copyright (C) 2010 Guido Guenther <agx@sigxcpu.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
"""Example on howto handle the different DBus signals emitted by
   krb5-auth-dialog"""

import datetime
import time
import subprocess
from optparse import OptionParser

from gi.repository import GLib
import dbus
import dbus.mainloop.glib


def print_info(action, principal, when):
    w = datetime.datetime.fromtimestamp(when)
    if options.verbose:
        print(f"Ticket {action} Principal {principal} expires {time.asctime(w.timetuple())}")


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
        print(f"Principal {principal} expired")
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
    bus.add_signal_receiver(tgt_renewed_handler, dbus_interface="org.gnome.KrbAuthDialog", signal_name="krb_tgt_renewed")
    bus.add_signal_receiver(tgt_acquired_handler, dbus_interface="org.gnome.KrbAuthDialog", signal_name="krb_tgt_acquired")
    bus.add_signal_receiver(tgt_expired_handler, dbus_interface="org.gnome.KrbAuthDialog", signal_name="krb_tgt_expired")

    GLib.MainLoop().run()
