#ifndef MATE_CANVAS_PATH_DEF_H
#define MATE_CANVAS_PATH_DEF_H

/*
 * MateCanvasPathDef
 *
 * (C) 1999-2000 Lauris Kaplinski <lauris@ximian.com>
 * Released under LGPL
 *
 * This is mostly like MateCanvasBpathDef, but with added functionality:
 * - can be constructed from scratch, from existing bpath of from static bpath
 * - Path is always terminated with ART_END
 * - Has closed flag
 * - has concat, split and copy methods
 *
 */

#include <glib-object.h>
#include <libart_lgpl/art_bpath.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateCanvasPathDef MateCanvasPathDef;

#define MATE_TYPE_CANVAS_PATH_DEF	(mate_canvas_path_def_get_type ())
GType mate_canvas_path_def_get_type (void) G_GNUC_CONST;

/* Constructors */

MateCanvasPathDef * mate_canvas_path_def_new (void);
MateCanvasPathDef * mate_canvas_path_def_new_sized (gint length);
MateCanvasPathDef * mate_canvas_path_def_new_from_bpath (ArtBpath * bpath);
MateCanvasPathDef * mate_canvas_path_def_new_from_static_bpath (ArtBpath * bpath);
MateCanvasPathDef * mate_canvas_path_def_new_from_foreign_bpath (ArtBpath * bpath);

void mate_canvas_path_def_ref (MateCanvasPathDef * path);
void mate_canvas_path_def_finish (MateCanvasPathDef * path);
void mate_canvas_path_def_ensure_space (MateCanvasPathDef * path, gint space);

/*
 * Misc constructors
 * All these return NEW path, not unrefing old
 * Also copy and duplicate force bpath to be private (otherwise you
 * would use ref :)
 */

void mate_canvas_path_def_copy (MateCanvasPathDef * dst, const MateCanvasPathDef * src);
MateCanvasPathDef * mate_canvas_path_def_duplicate (const MateCanvasPathDef * path);
MateCanvasPathDef * mate_canvas_path_def_concat (const GSList * list);
GSList * mate_canvas_path_def_split (const MateCanvasPathDef * path);
MateCanvasPathDef * mate_canvas_path_def_open_parts (const MateCanvasPathDef * path);
MateCanvasPathDef * mate_canvas_path_def_closed_parts (const MateCanvasPathDef * path);
MateCanvasPathDef * mate_canvas_path_def_close_all (const MateCanvasPathDef * path);

/* Destructor */

void mate_canvas_path_def_unref (MateCanvasPathDef * path);

/* Methods */

/* Sets MateCanvasPathDef to zero length */

void mate_canvas_path_def_reset (MateCanvasPathDef * path);

/* Drawing methods */

void mate_canvas_path_def_moveto (MateCanvasPathDef * path, gdouble x, gdouble y);
void mate_canvas_path_def_lineto (MateCanvasPathDef * path, gdouble x, gdouble y);

/* Does not create new ArtBpath, but simply changes last lineto position */

void mate_canvas_path_def_lineto_moving (MateCanvasPathDef * path, gdouble x, gdouble y);
void mate_canvas_path_def_curveto (MateCanvasPathDef * path, gdouble x0, gdouble y0,gdouble x1, gdouble y1, gdouble x2, gdouble y2);
void mate_canvas_path_def_closepath (MateCanvasPathDef * path);

/* Does not draw new line to startpoint, but moves last lineto */

void mate_canvas_path_def_closepath_current (MateCanvasPathDef * path);

/* Various methods */

ArtBpath * mate_canvas_path_def_bpath (const MateCanvasPathDef * path);
gint mate_canvas_path_def_length (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_is_empty (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_has_currentpoint (const MateCanvasPathDef * path);
void mate_canvas_path_def_currentpoint (const MateCanvasPathDef * path, ArtPoint * p);
ArtBpath * mate_canvas_path_def_last_bpath (const MateCanvasPathDef * path);
ArtBpath * mate_canvas_path_def_first_bpath (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_any_open (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_all_open (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_any_closed (const MateCanvasPathDef * path);
gboolean mate_canvas_path_def_all_closed (const MateCanvasPathDef * path);

G_END_DECLS

#endif
