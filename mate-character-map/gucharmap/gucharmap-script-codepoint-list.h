/*
 * Copyright © 2004 Noah Levitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#if !defined (__GUCHARMAP_GUCHARMAP_H_INSIDE__) && !defined (GUCHARMAP_COMPILATION)
#error "Only <gucharmap/gucharmap.h> can be included directly."
#endif

#ifndef GUCHARMAP_SCRIPT_CODEPOINT_LIST_H
#define GUCHARMAP_SCRIPT_CODEPOINT_LIST_H

#include <glib-object.h>

#include <gucharmap/gucharmap-codepoint-list.h>

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST             (gucharmap_script_codepoint_list_get_type ())
#define GUCHARMAP_SCRIPT_CODEPOINT_LIST(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST, GucharmapScriptCodepointList))
#define GUCHARMAP_SCRIPT_CODEPOINT_LIST_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST, GucharmapScriptCodepointListClass))
#define GUCHARMAP_IS_SCRIPT_CODEPOINT_LIST(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST))
#define GUCHARMAP_IS_SCRIPT_CODEPOINT_LIST_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST))
#define GUCHARMAP_SCRIPT_CODEPOINT_LIST_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_SCRIPT_CODEPOINT_LIST, GucharmapScriptCodepointListClass))

typedef struct _GucharmapScriptCodepointList        GucharmapScriptCodepointList;
typedef struct _GucharmapScriptCodepointListPrivate GucharmapScriptCodepointListPrivate;
typedef struct _GucharmapScriptCodepointListClass   GucharmapScriptCodepointListClass;

struct _GucharmapScriptCodepointList
{
  GucharmapCodepointList parent;

  /*< private >*/
  GucharmapScriptCodepointListPrivate *priv;
};

struct _GucharmapScriptCodepointListClass
{
  GucharmapCodepointListClass parent_class;
};

GType                    gucharmap_script_codepoint_list_get_type       (void);
GucharmapCodepointList * gucharmap_script_codepoint_list_new            (void);
gboolean                 gucharmap_script_codepoint_list_set_script     (GucharmapScriptCodepointList  *list,
	                                                                 const gchar                   *script);
gboolean                 gucharmap_script_codepoint_list_set_scripts    (GucharmapScriptCodepointList  *list,
	                                                                 const gchar                  **scripts);
gboolean                 gucharmap_script_codepoint_list_append_script  (GucharmapScriptCodepointList  *list,
                                                                         const gchar                   *script);
/* XXX: gucharmap_script_codepoint_list_get_script? seems unnecessary */

G_END_DECLS

#endif /* #ifndef GUCHARMAP_SCRIPT_CODEPOINT_LIST_H */
