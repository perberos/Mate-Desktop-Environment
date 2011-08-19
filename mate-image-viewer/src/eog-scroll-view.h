#ifndef _EOG_SCROLL_VIEW_H_
#define _EOG_SCROLL_VIEW_H_

#include <gtk/gtk.h>
#include "eog-image.h"

G_BEGIN_DECLS

typedef struct _EogScrollView EogScrollView;
typedef struct _EogScrollViewClass EogScrollViewClass;
typedef struct _EogScrollViewPrivate EogScrollViewPrivate;

#define EOG_TYPE_SCROLL_VIEW              (eog_scroll_view_get_type ())
#define EOG_SCROLL_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), EOG_TYPE_SCROLL_VIEW, EogScrollView))
#define EOG_SCROLL_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), EOG_TYPE_SCROLL_VIEW, EogScrollViewClass))
#define EOG_IS_SCROLL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EOG_TYPE_SCROLL_VIEW))
#define EOG_IS_SCROLL_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), EOG_TYPE_SCROLL_VIEW))


struct _EogScrollView {
	GtkTable  widget;

	EogScrollViewPrivate *priv;
};

struct _EogScrollViewClass {
	GtkTableClass parent_class;

	void (* zoom_changed) (EogScrollView *view, double zoom);
};

typedef enum {
	EOG_TRANSP_BACKGROUND,
	EOG_TRANSP_CHECKED,
	EOG_TRANSP_COLOR
} EogTransparencyStyle;

GType    eog_scroll_view_get_type         (void) G_GNUC_CONST;
GtkWidget* eog_scroll_view_new            (void);

/* loading stuff */
void     eog_scroll_view_set_image        (EogScrollView *view, EogImage *image);

/* general properties */
void     eog_scroll_view_set_scroll_wheel_zoom (EogScrollView *view, gboolean scroll_wheel_zoom);
void     eog_scroll_view_set_zoom_upscale (EogScrollView *view, gboolean upscale);
void     eog_scroll_view_set_zoom_multiplier (EogScrollView *view, gdouble multiplier);
void     eog_scroll_view_set_antialiasing_in (EogScrollView *view, gboolean state);
void     eog_scroll_view_set_antialiasing_out (EogScrollView *view, gboolean state);
void     eog_scroll_view_set_transparency (EogScrollView *view, EogTransparencyStyle style, GdkColor *color);
gboolean eog_scroll_view_scrollbars_visible (EogScrollView *view);
void	 eog_scroll_view_set_popup (EogScrollView *view, GtkMenu *menu);
void	 eog_scroll_view_set_background_color (EogScrollView *view,
					       const GdkColor *color);
void	 eog_scroll_view_override_bg_color (EogScrollView *view,
					    const GdkColor *color);
void     eog_scroll_view_set_use_bg_color (EogScrollView *view, gboolean use);
/* zoom api */
void     eog_scroll_view_zoom_in          (EogScrollView *view, gboolean smooth);
void     eog_scroll_view_zoom_out         (EogScrollView *view, gboolean smooth);
void     eog_scroll_view_zoom_fit         (EogScrollView *view);
void     eog_scroll_view_set_zoom         (EogScrollView *view, double zoom);
double   eog_scroll_view_get_zoom         (EogScrollView *view);
gboolean eog_scroll_view_get_zoom_is_min  (EogScrollView *view);
gboolean eog_scroll_view_get_zoom_is_max  (EogScrollView *view);
void     eog_scroll_view_show_cursor      (EogScrollView *view);
void     eog_scroll_view_hide_cursor      (EogScrollView *view);

G_END_DECLS

#endif /* _EOG_SCROLL_VIEW_H_ */


