/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "oaf-helper.h"
#include "component-details.h"
#include <gtk/gtk.h>

#define PARENT_TYPE GTK_TYPE_VBOX

struct _ComponentDetailsPrivate {
	gchar *iid;
	GList *components;
	GtkWidget *description_label;
	GtkWidget *name_label;
	GtkWidget *iid_label;
	GtkWidget *loc_label;
	GtkWidget *tree;
	GtkTreeStore *model;
};

static void component_details_class_init (ComponentDetailsClass *klass);
static void component_details_init (ComponentDetails *comp_details,
				    ComponentDetailsClass *klass);
static void component_details_finalize (GObject *object);

static GObjectClass *parent_class = NULL;

/*
 * Tree View functions.
 */
static void
set_detailed_func (GtkTreeViewColumn *tree_column,
		   GtkCellRenderer *cell,
		   GtkTreeModel *model,
		   GtkTreeIter *iter,
		   gpointer user_data)
{
	gchar *data;

	gtk_tree_model_get (model, iter, 0, &data, -1);

	g_object_set (G_OBJECT (cell), "text", data, NULL);

	g_free (data);
}

/*
 * ComponentDetails class implementation
 */

static void
component_details_class_init (ComponentDetailsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = component_details_finalize;
}

static void
component_details_init (ComponentDetails *comp_details,
			ComponentDetailsClass *klass)
{
	GtkWidget *notebook;
	GtkWidget *general_label;
	GtkWidget *general_vbox;
	GtkWidget *details_label;
	GtkWidget *details_frame;
	GtkWidget *description_frame, *name_frame, *iid_frame, *loc_frame;
	GtkWidget *details_scroll;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_renderer;
	
	g_return_if_fail (IS_COMPONENT_DETAILS (comp_details));

	/* Allocate the private struct */
	comp_details->priv = g_new0 (ComponentDetailsPrivate, 1);

	notebook = gtk_notebook_new();
	gtk_box_pack_start (GTK_BOX (comp_details), notebook, TRUE, TRUE, 6);

	/* Build the general tab */
	general_label = gtk_label_new (_("General"));
	general_vbox = gtk_vbox_new (TRUE, 0);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), general_vbox,
				  general_label);

	name_frame = gtk_frame_new (_("Name"));
	description_frame = gtk_frame_new (_("Description"));
	iid_frame = gtk_frame_new (_("IID"));
	loc_frame = gtk_frame_new (_("Location"));
	gtk_box_pack_start (GTK_BOX (general_vbox),
			    iid_frame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (general_vbox),
			    name_frame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (general_vbox),
			    loc_frame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (general_vbox),
			    description_frame, TRUE, TRUE, 0);

	comp_details->priv->iid_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (comp_details->priv->iid_label),
			       GTK_JUSTIFY_RIGHT);
	gtk_label_set_selectable (GTK_LABEL (comp_details->priv->iid_label),
				  TRUE);
	gtk_container_add (GTK_CONTAINER (iid_frame),
			   comp_details->priv->iid_label);

	comp_details->priv->name_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (comp_details->priv->name_label),
			       GTK_JUSTIFY_RIGHT);
	gtk_label_set_selectable (
		GTK_LABEL (comp_details->priv->name_label), TRUE);
	gtk_container_add (GTK_CONTAINER (name_frame),
			   comp_details->priv->name_label);


	comp_details->priv->loc_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (
		GTK_LABEL (comp_details->priv->loc_label), TRUE);
	gtk_label_set_justify (
		GTK_LABEL (comp_details->priv->loc_label), TRUE);
	gtk_label_set_selectable (
		GTK_LABEL (comp_details->priv->loc_label), TRUE);
	gtk_container_add (GTK_CONTAINER (loc_frame),
			   comp_details->priv->loc_label);
	

	comp_details->priv->description_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap (
		GTK_LABEL (comp_details->priv->description_label), TRUE);
	gtk_label_set_justify (
		GTK_LABEL (comp_details->priv->description_label),
		GTK_JUSTIFY_LEFT);
	gtk_label_set_selectable (
		GTK_LABEL (comp_details->priv->description_label), TRUE);
	gtk_container_add (GTK_CONTAINER (description_frame),
			   comp_details->priv->description_label);

	/* Build the details tab */
	details_label = gtk_label_new (_("Details"));
	details_frame = gtk_frame_new (_("Detailed Information"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), details_frame,
				  details_label);

	details_scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(details_scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER (details_frame), details_scroll);

	comp_details->priv->tree = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible (
		GTK_TREE_VIEW (comp_details->priv->tree),
		FALSE);
	gtk_container_add(GTK_CONTAINER (details_scroll),
			  comp_details->priv->tree);

	comp_details->priv->model = gtk_tree_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (comp_details->priv->tree),
				 GTK_TREE_MODEL (comp_details->priv->model));
	g_object_unref (comp_details->priv->model);

	text_renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   text_renderer,
							   NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
						 set_detailed_func,
						 NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (comp_details->priv->tree),
				     column);

	gtk_widget_show_all (notebook);
}

static void
component_details_finalize (GObject *object)
{
	ComponentDetails *comp_details;

	comp_details = (ComponentDetails *) object;
	
	g_return_if_fail (IS_COMPONENT_DETAILS (comp_details));

	g_free (comp_details->priv);
	comp_details->priv = NULL;
	
	/* Chain to parent class */
	parent_class->finalize (object);
}

GType
component_details_get_type (void)
{
	static GType type = 0;

        if (!type) {
                if (type == 0) {
                        static GTypeInfo info = {
                                sizeof (ComponentDetailsClass),
                                (GBaseInitFunc) NULL,
                                (GBaseFinalizeFunc) NULL,
                                (GClassInitFunc) component_details_class_init,
                                NULL, NULL,
                                sizeof (ComponentDetails),
                                0,
                                (GInstanceInitFunc) component_details_init
                        };
                        type = g_type_register_static (PARENT_TYPE,
						       "ComponentDetails",
						       &info, 0);
                }
        }

        return type;
}

GtkWidget *
component_details_new (gchar *iid)
{
	ComponentDetails *comp_details;

	comp_details = g_object_new (COMPONENT_DETAILS_TYPE, NULL);

	component_details_get_info (comp_details, iid);

	return GTK_WIDGET (comp_details);
}

void
component_details_get_info (ComponentDetails *comp_details, gchar *iid) {
	gchar *query;
	MateComponentComponentInfo *info;
	GtkTreeIter parent_iter, iter;
	GList *repo_ids;
	int len;

	query = g_strdup_printf ("iid == '%s'", iid);

	comp_details->priv->components = matecomponent_browser_get_components_list (
		query);
	g_free (query);

	len = g_list_length (comp_details->priv->components);
	if (len == 0) {
		/* We don't handle this correctly yet. */
		g_print ("No components found\n");
		g_assert_not_reached ();
	} else if (len > 1) {
		/* This should never happen. No two components */
		/* should have the same IID. */
		g_print ("Two component with the same IID!!!\n");
		g_assert_not_reached ();
	}

	/* Build the General tab */
	info = (MateComponentComponentInfo *) comp_details->priv->components->data;

	gtk_label_set_label (GTK_LABEL (comp_details->priv->iid_label),
			     info->component_iid);
	gtk_label_set_label (GTK_LABEL (comp_details->priv->name_label),
			     info->component_name);
	gtk_label_set_label (GTK_LABEL (comp_details->priv->loc_label),
			     info->component_location);
	gtk_label_set_label (GTK_LABEL (comp_details->priv->description_label),
			     info->component_description);

	/* Build the Details tab */
	gtk_tree_store_append (comp_details->priv->model, &parent_iter, NULL);
	gtk_tree_store_set (comp_details->priv->model, &parent_iter,
			    0, "Repo Ids", -1);

	/* Get a GList of this components repo ids */
	repo_ids = matecomponent_component_get_repoids (info);

	while (repo_ids != NULL) {
		gtk_tree_store_append (comp_details->priv->model, &iter,
				       &parent_iter);

		gtk_tree_store_set (comp_details->priv->model, &iter,
				    0, repo_ids->data, -1);

		repo_ids = repo_ids->next;
	}

	/* Clean up time... */
	matecomponent_component_info_free (info);
	g_list_free (comp_details->priv->components);
}
