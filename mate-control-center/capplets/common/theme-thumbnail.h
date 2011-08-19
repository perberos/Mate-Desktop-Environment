#ifndef __THEME_THUMBNAIL_H__
#define __THEME_THUMBNAIL_H__


#include <gtk/gtk.h>
#include "mate-theme-info.h"

typedef void (* ThemeThumbnailFunc)          (GdkPixbuf          *pixbuf,
                                              gchar              *theme_name,
                                              gpointer            data);

GdkPixbuf *generate_meta_theme_thumbnail     (MateThemeMetaInfo *theme_info);
GdkPixbuf *generate_gtk_theme_thumbnail      (MateThemeInfo     *theme_info);
GdkPixbuf *generate_marco_theme_thumbnail (MateThemeInfo     *theme_info);
GdkPixbuf *generate_icon_theme_thumbnail     (MateThemeIconInfo *theme_info);

void generate_meta_theme_thumbnail_async     (MateThemeMetaInfo *theme_info,
                                              ThemeThumbnailFunc  func,
                                              gpointer            data,
                                              GDestroyNotify      destroy);
void generate_gtk_theme_thumbnail_async      (MateThemeInfo     *theme_info,
                                              ThemeThumbnailFunc  func,
                                              gpointer            data,
                                              GDestroyNotify      destroy);
void generate_marco_theme_thumbnail_async (MateThemeInfo     *theme_info,
                                              ThemeThumbnailFunc  func,
                                              gpointer            data,
                                              GDestroyNotify      destroy);
void generate_icon_theme_thumbnail_async     (MateThemeIconInfo *theme_info,
                                              ThemeThumbnailFunc  func,
                                              gpointer            data,
                                              GDestroyNotify      destroy);

void theme_thumbnail_factory_init            (int                 argc,
                                              char               *argv[]);

#endif /* __THEME_THUMBNAIL_H__ */
