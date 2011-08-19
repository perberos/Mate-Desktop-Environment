#ifndef LOAD_GRAPH_H__
#define LOAD_GRAPH_H__

#include "global.h"

/* Create new load graph. */
G_GNUC_INTERNAL LoadGraph *
load_graph_new (MultiloadApplet *multiload, guint n, const gchar *label,
		guint id, guint speed, guint size, gboolean visible, 
		const gchar *name, LoadGraphDataFunc get_data);

/* Start load graph. */
G_GNUC_INTERNAL void
load_graph_start (LoadGraph *g);

/* Stop load graph. */
G_GNUC_INTERNAL void
load_graph_stop (LoadGraph *g);

/* free load graph */
G_GNUC_INTERNAL void
load_graph_unalloc (LoadGraph *g);
		      
#endif
