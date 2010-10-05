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

/* Create an entry buffer that uses the secmem routines for password storage */

#include "config.h"

#include <gtk/gtk.h>
#include <string.h>

#include "ka-entry-buffer.h"
#include "memory.h"

struct _KaEntryBuffer {
  GtkEntryBuffer parent;

  KaEntryBufferPrivate *priv;
};

struct _KaEntryBufferClass {
  GtkEntryBufferClass parent_class;
};

G_DEFINE_TYPE (KaEntryBuffer, ka_entry_buffer, GTK_TYPE_ENTRY_BUFFER)

struct _KaEntryBufferPrivate {
    gchar *password;
    gsize password_size;
    gsize password_bytes;
    guint password_chars;
};


static const gchar *
ka_entry_buffer_pw_get_text (GtkEntryBuffer *buffer,
                             gsize * n_bytes)
{
    KaEntryBuffer *self = KA_ENTRY_BUFFER(buffer);

    if (n_bytes)
        *n_bytes = self->priv->password_bytes;
    if (!self->priv->password)
        return "";
    return self->priv->password;
}


static guint
ka_entry_buffer_pw_get_length (GtkEntryBuffer *buffer)
{
    KaEntryBuffer *self = KA_ENTRY_BUFFER(buffer);
    return self->priv->password_chars;
}


static guint
ka_entry_buffer_pw_insert_text (GtkEntryBuffer *buffer,
                                guint position,
                                const gchar *chars,
                                guint n_chars)
{
    KaEntryBuffer *self = KA_ENTRY_BUFFER(buffer);
    KaEntryBufferPrivate *pv = self->priv;
    gsize prev_size;
    gsize n_bytes;
    gsize at;

    n_bytes = g_utf8_offset_to_pointer (chars, n_chars) - chars;

    /* Need more memory */
    if (n_bytes + pv->password_bytes + 1 > pv->password_size) {
        gchar *et_new;

        prev_size = pv->password_size;

        /* Calculate our new buffer size */
        while (n_bytes + pv->password_bytes + 1 > pv->password_size) {
            if (pv->password_size == 0)
                pv->password_size = PW_MIN_SIZE;
            else {
                if (2 * pv->password_size < GTK_ENTRY_BUFFER_MAX_SIZE)
                    pv->password_size *= 2;
                else {
                    pv->password_size = GTK_ENTRY_BUFFER_MAX_SIZE;
                    if (n_bytes >
                        pv->password_size - pv->password_bytes - 1) {
                        n_bytes =
                            pv->password_size - pv->password_bytes - 1;
                        n_bytes =
                            g_utf8_find_prev_char (chars,
                                                   chars + n_bytes + 1) -
                            chars;
                        n_chars = g_utf8_strlen (chars, n_bytes);
                    }
                    break;
                }
            }
        }

        et_new = secmem_malloc (pv->password_size);
        memcpy (et_new, pv->password,
                MIN (prev_size, pv->password_size));
        secmem_free (pv->password);
        pv->password = et_new;
    }

    /* Actual text insertion */
    at = g_utf8_offset_to_pointer (pv->password,
                                   position) - pv->password;
    g_memmove (pv->password + at + n_bytes, pv->password + at,
               pv->password_bytes - at);
    memcpy (pv->password + at, chars, n_bytes);

    /* Book keeping */
    pv->password_bytes += n_bytes;
    pv->password_chars += n_chars;
    pv->password[pv->password_bytes] = '\0';

    gtk_entry_buffer_emit_inserted_text (GTK_ENTRY_BUFFER(self), position, chars, n_chars);
    return n_chars;
}


static guint
ka_entry_buffer_pw_delete_text (GtkEntryBuffer *buffer,
                                guint position, guint n_chars)
{
    KaEntryBuffer *self = KA_ENTRY_BUFFER(buffer);
    KaEntryBufferPrivate *pv = self->priv;
    gsize start, end;

    if (position > pv->password_chars)
        position = pv->password_chars;
    if (position + n_chars > pv->password_chars)
        n_chars = pv->password_chars - position;

    if (n_chars > 0) {
        start =
            g_utf8_offset_to_pointer (pv->password,
                                      position) - pv->password;
        end =
            g_utf8_offset_to_pointer (pv->password,
                                      position + n_chars) - pv->password;

        g_memmove (pv->password + start, pv->password + end,
                   pv->password_bytes + 1 - end);
        pv->password_chars -= n_chars;
        pv->password_bytes -= (end - start);
        gtk_entry_buffer_emit_deleted_text (GTK_ENTRY_BUFFER(self), position, n_chars);
    }
    return n_chars;
}


static void
ka_entry_buffer_dispose (GObject *object)
{
    G_OBJECT_CLASS (ka_entry_buffer_parent_class)->dispose (object);
}

static void
ka_entry_buffer_finalize (GObject *object)
{
    KaEntryBuffer *self = KA_ENTRY_BUFFER(object);

    if (self->priv->password) {
        secmem_free (self->priv->password);
        self->priv->password_size = 0;
        self->priv->password_bytes = 0;
        self->priv->password_chars = 0;
    }
    G_OBJECT_CLASS (ka_entry_buffer_parent_class)->finalize (object);
}

static void
ka_entry_buffer_class_init (KaEntryBufferClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkEntryBufferClass *eb_class = GTK_ENTRY_BUFFER_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KaEntryBufferPrivate));

    eb_class->get_text = ka_entry_buffer_pw_get_text;
    eb_class->get_length = ka_entry_buffer_pw_get_length;
    eb_class->insert_text = ka_entry_buffer_pw_insert_text;
    eb_class->delete_text = ka_entry_buffer_pw_delete_text;

    object_class->dispose = ka_entry_buffer_dispose;
    object_class->finalize = ka_entry_buffer_finalize;
}

static void
ka_entry_buffer_init (KaEntryBuffer *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
                                             KA_TYPE_ENTRY_BUFFER,
                                             KaEntryBufferPrivate);
    self->priv->password = NULL;
    self->priv->password_size = 0;
    self->priv->password_bytes = 0;
    self->priv->password_chars = 0;
}

KaEntryBuffer *
ka_entry_buffer_new (void)
{
    return g_object_new (KA_TYPE_ENTRY_BUFFER, NULL);
}

/*
 * vim:ts:sts=4:sw=4:et:
 */
