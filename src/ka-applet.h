/* Krb5 Auth Applet -- Acquire and release Kerberos tickets
 *
 * (C) 2008 Guido Guenther <agx@sigxcpu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KA_APPLET_H
#define KA_APPLET_H

#include <glib-object.h>

G_BEGIN_DECLS

#define KA_TYPE_APPLET            (ka_applet_get_type ())
#define KA_APPLET(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_APPLET, KaApplet))
#define KA_APPLET_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_APPLET, KaAppletClass))
#define KA_IS_APPLET(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_APPLET))
#define KA_IS_APPLET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_APPLET))
#define KA_APPLET_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_APPLET, KaAppletClass))

typedef struct _KaApplet        KaApplet;
typedef struct _KaAppletClass   KaAppletClass;
typedef struct _KaAppletPrivate KaAppletPrivate;

GType ka_applet_get_type (void);

G_END_DECLS

#endif
