/* ka-preferences.h */

#ifndef KA_PREFERENCES_H
#define KA_PREFERENCES_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "config.h"
#include "ka-applet.h"

G_BEGIN_DECLS

#define KA_TYPE_PREFERENCES            (ka_preferences_get_type ())
#define KA_PREFERENCES(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), KA_TYPE_PREFERENCES, KaPreferences))
#define KA_PREFERENCES_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), KA_TYPE_PREFERENCES, KaPreferencesClass))
#define KA_IS_PREFERENCES(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KA_TYPE_PREFERENCES))
#define KA_IS_PREFERENCES_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), KA_TYPE_PREFERENCES))
#define KA_PREFERENCES_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), KA_TYPE_PREFERENCES, KaPreferencesClass))
    typedef struct _KaPreferences KaPreferences;
typedef struct _KaPreferencesClass KaPreferencesClass;
typedef struct _KaPreferencesPrivate KaPreferencesPrivate;

GType ka_preferences_get_type (void);

KaPreferences* ka_preferences_new (KaApplet *applet);
void ka_preferences_run (KaPreferences *self);


G_END_DECLS

#endif /* KA_PREFERENCES */

/*
 * vim:ts:sts=4:sw=4:et:
 */
