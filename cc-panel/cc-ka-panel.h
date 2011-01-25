/* cc-ka-panel.h */

#ifndef _CC_KA_PANEL
#define _CC_KA_PANEL

#include <libgnome-control-center/cc-panel.h>

G_BEGIN_DECLS

#define CC_TYPE_KA_PANEL cc_ka_panel_get_type()

#define CC_KA_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),CC_TYPE_KA_PANEL, CcKaPanel))

#define CC_KA_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), CC_TYPE_KA_PANEL, CcKaPanelClass))

#define CC_IS_KA_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CC_TYPE_KA_PANEL))

#define CC_IS_KA_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), CC_TYPE_KA_PANEL))

#define CC_KA_PANEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), CC_TYPE_KA_PANEL, CcKaPanelClass))

typedef struct {
    CcPanel parent;
} CcKaPanel;

typedef struct {
    CcPanelClass parent_class;
} CcKaPanelClass;

GType cc_ka_panel_get_type (void) G_GNUC_CONST;

void cc_ka_panel_register (GIOModule *module);

G_END_DECLS

#endif /* _CC_KA_PANEL */

/*
 * vim:ts:sts=4:sw=4:et:
 */
