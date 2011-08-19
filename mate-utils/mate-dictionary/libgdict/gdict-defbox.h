/* gdict-defbox.h - display widget for dictionary definitions
 *
 * Copyright (C) 2005-2006  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __GDICT_DEFBOX_H__
#define __GDICT_DEFBOX_H__

#include <gtk/gtk.h>
#include "gdict-context.h"

G_BEGIN_DECLS

#define GDICT_TYPE_DEFBOX		(gdict_defbox_get_type ())
#define GDICT_DEFBOX(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_DEFBOX, GdictDefbox))
#define GDICT_IS_DEFBOX(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_DEFBOX))
#define GDICT_DEFBOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_DEFBOX, GdictDefboxClass))
#define GDICT_IS_DEFBOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_DEFBOX))
#define GDICT_DEFBOX_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_DEFBOX, GdictDefboxClass))

typedef struct _GdictDefbox        GdictDefbox;
typedef struct _GdictDefboxClass   GdictDefboxClass;
typedef struct _GdictDefboxPrivate GdictDefboxPrivate;

struct _GdictDefbox
{
  /*< private >*/
  GtkVBox parent_instance;
  
  GdictDefboxPrivate *priv;
};

struct _GdictDefboxClass
{
  GtkVBoxClass parent_class;
  
  /* these are all RUN_ACTION signals for key bindings */
  void (*show_find)     (GdictDefbox *defbox);
  void (*hide_find)     (GdictDefbox *defbox);
  void (*find_previous) (GdictDefbox *defbox);
  void (*find_next)     (GdictDefbox *defbox);

  /* signals */
  void (*link_clicked)  (GdictDefbox *defbox,
                         const gchar *link);
  
  /* padding for future expansion */
  void (*_gdict_defbox_1) (void);
  void (*_gdict_defbox_2) (void);
  void (*_gdict_defbox_3) (void);
  void (*_gdict_defbox_4) (void);
};

GType                 gdict_defbox_get_type           (void) G_GNUC_CONST;

GtkWidget *           gdict_defbox_new                (void);
GtkWidget *           gdict_defbox_new_with_context   (GdictContext *context);
void                  gdict_defbox_set_context        (GdictDefbox  *defbox,
						       GdictContext *context);
GdictContext *        gdict_defbox_get_context        (GdictDefbox  *defbox);
void                  gdict_defbox_set_database       (GdictDefbox  *defbox,
						       const gchar  *database);
G_CONST_RETURN gchar *gdict_defbox_get_database       (GdictDefbox  *defbox);
G_CONST_RETURN gchar *gdict_defbox_get_word           (GdictDefbox  *defbox);
gchar *               gdict_defbox_get_text           (GdictDefbox  *defbox,
						       gsize        *length) G_GNUC_MALLOC;
void                  gdict_defbox_select_all         (GdictDefbox  *defbox);
void                  gdict_defbox_copy_to_clipboard  (GdictDefbox  *defbox,
						       GtkClipboard *clipboard);
void                  gdict_defbox_clear              (GdictDefbox  *defbox);
void                  gdict_defbox_lookup             (GdictDefbox  *defbox,
						       const gchar  *word);
gint                  gdict_defbox_count_definitions  (GdictDefbox  *defbox);
void                  gdict_defbox_jump_to_definition (GdictDefbox  *defbox,
						       gint          number);
void                  gdict_defbox_set_show_find      (GdictDefbox  *defbox,
						       gboolean      show_find);
gboolean              gdict_defbox_get_show_find      (GdictDefbox  *defbox);
void                  gdict_defbox_find_next          (GdictDefbox  *defbox);
void                  gdict_defbox_find_previous      (GdictDefbox  *defbox);
void                  gdict_defbox_set_font_name      (GdictDefbox  *defbox,
						       const gchar  *font_name);
G_CONST_RETURN gchar *gdict_defbox_get_font_name      (GdictDefbox  *defbox);
gchar *               gdict_defbox_get_selected_word  (GdictDefbox  *defbox) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __GDICT_DEFBOX_H__ */
