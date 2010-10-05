/* Krb5 Auth Applet -- Acquire and release kerberos tickets
 *
 * (C) 2010 Guido Guenther <agx@sigxcpu.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef KA_ENTRY_BUFFER_H
#define KA_ENTRY_BUFFER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define KA_TYPE_ENTRY_BUFFER ka_entry_buffer_get_type()

#define KA_ENTRY_BUFFER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_ENTRY_BUFFER, KaEntryBuffer))

#define KA_ENTRY_BUFFER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_ENTRY_BUFFER, KaEntryBufferClass))

#define KA_IS_ENTRY_BUFFER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_ENTRY_BUFFER))

#define KA_IS_ENTRY_BUFFER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_ENTRY_BUFFER))

#define KA_ENTRY_BUFFER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_ENTRY_BUFFER, KaEntryBufferClass))

/* Minimum buffer size */
#define PW_MIN_SIZE 32

typedef struct _KaEntryBuffer        KaEntryBuffer;
typedef struct _KaEntryBufferClass   KaEntryBufferClass;
typedef struct _KaEntryBufferPrivate KaEntryBufferPrivate;

GType ka_entry_buffer_get_type (void);

KaEntryBuffer* ka_entry_buffer_new (void);

G_END_DECLS

#endif /* _KA_ENTRY_BUFFER */
