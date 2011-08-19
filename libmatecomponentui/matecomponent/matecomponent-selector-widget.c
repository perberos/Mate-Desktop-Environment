/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-selector-widget.c: MateComponent Component Selector widget
 *
 * Authors:
 *   Michael Meeks    (michael@ximian.com)
 *   Richard Hestilow (hestgray@ionet.net)
 *   Miguel de Icaza  (miguel@kernel.org)
 *   Martin Baulig    (martin@
 *   Anders Carlsson  (andersca@gnu.org)
 *   Havoc Pennington (hp@redhat.com)
 *   Dietmar Maurer   (dietmar@maurer-it.com)
 *
 * Copyright 1999, 2001 Richard Hestilow, Ximian, Inc,
 *                      Martin Baulig, Anders Carlsson,
 *                      Havoc Pennigton, Dietmar Maurer
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h> /* strcmp */
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-selector-widget.h>
#include <matecomponent/matecomponent-ui-preferences.h>

#include "matecomponent-insert-component.xpm"

G_DEFINE_TYPE (MateComponentSelectorWidget, matecomponent_selector_widget, GTK_TYPE_VBOX)

#define GET_CLASS(o) MATECOMPONENT_SELECTOR_WIDGET_CLASS (GTK_OBJECT_GET_CLASS (o))

enum {
	FINAL_SELECT,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

struct _MateComponentSelectorWidgetPrivate {
	GtkTreeView  *list_view;
	GtkListStore *list_store;

	GtkWidget    *desc_label;
};

static char *
build_id_query_fragment (const char **required_ids)
{
        const char **required_ids_iter;
	const char **query_components_iter;
        char       **query_components;
	char        *query;
        guint        n_required = 0;

        /* We need to build a query up from the required_ids */
        required_ids_iter = required_ids;

        while (required_ids && *required_ids_iter) {
                ++n_required;
                ++required_ids_iter;
        }

        query_components = g_new0 (gchar*, n_required + 1);

        query_components_iter = (const gchar **) query_components;
        required_ids_iter = required_ids;

        while (*required_ids_iter) {
                *query_components_iter = g_strconcat ("repo_ids.has('",
                                                      *required_ids_iter,
                                                      "')",
                                                      NULL);
                ++query_components_iter;
                ++required_ids_iter;
        }

        query = g_strjoinv (" AND ", query_components);

        g_strfreev (query_components);

	return query;
}

static GSList *
get_lang_list (void)
{
	static GSList *ret = NULL;

	if (!ret) {
		const gchar * const *names;
		
		for (names = g_get_language_names (); *names; names++)
			ret = g_slist_prepend (ret, (gchar *) *names);
		ret = g_slist_reverse (ret);
	}

	return ret;
}

static void
get_filtered_objects (MateComponentSelectorWidgetPrivate *priv,
		      const gchar **required_ids)
{
        guint                  i;
        gchar                 *query;
        CORBA_Environment      ev;
        MateComponent_ServerInfoList *servers;
	GSList                *lang_list;
        
        g_return_if_fail (required_ids != NULL);
        g_return_if_fail (*required_ids != NULL);

	query = build_id_query_fragment (required_ids);

	/* FIXME: sorting ? can we get oaf to do it ? - would be nice. */

        CORBA_exception_init (&ev);
        servers = matecomponent_activation_query (query, NULL, &ev);
        g_free (query);
        CORBA_exception_free (&ev);

        if (!servers)
                return;

	lang_list = get_lang_list ();

	for (i = 0; i < servers->_length; i++) {
                MateComponent_ServerInfo *oafinfo = &servers->_buffer[i];
		const gchar *name = NULL, *desc = NULL;
		GtkTreeIter iter;

		name = matecomponent_server_info_prop_lookup (oafinfo, "name", lang_list);
		desc = matecomponent_server_info_prop_lookup (oafinfo, "description", lang_list);

		if (!name && !desc)
			name = desc = oafinfo->iid;

		if (!name)
			name = desc;

		if (!desc)
			desc = name;

		gtk_list_store_append (priv->list_store, &iter);
		gtk_list_store_set (priv->list_store, &iter,
				    0, name,
				    1, desc,
				    2, oafinfo->iid,
				    -1);
        }

        CORBA_free (servers);
}

static void
matecomponent_selector_widget_finalize (GObject *object)
{
	g_free (MATECOMPONENT_SELECTOR_WIDGET (object)->priv);

	G_OBJECT_CLASS (matecomponent_selector_widget_parent_class)->finalize (object);
}

static gchar *
get_field (MateComponentSelectorWidget *sel, int col)
{
	gchar *text;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	MateComponentSelectorWidgetPrivate *priv; 

	g_return_val_if_fail (sel != NULL, NULL);
	priv = sel->priv;	

	selection = gtk_tree_view_get_selection (priv->list_view);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (priv->list_store),
			    &iter, col, &text, -1);

	return text;
}

static gchar *
impl_get_id (MateComponentSelectorWidget *sel)
{
	return get_field (sel, 2);
}

/**
 * matecomponent_selector_widget_get_id:
 * @sel: A MateComponentSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the ID of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this. It will give an oaf iid back.
 */
gchar *
matecomponent_selector_widget_get_id (MateComponentSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_id (sel);
}

static gchar *
impl_get_name (MateComponentSelectorWidget *sel)
{
	return get_field (sel, 0);
}

/**
 * matecomponent_selector_widget_get_name:
 * @sel: A MateComponentSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the name of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
matecomponent_selector_widget_get_name (MateComponentSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_name (sel);
}

static gchar *
impl_get_description (MateComponentSelectorWidget *sel)
{
	return get_field (sel, 1);
}

/**
 * matecomponent_selector_widget_get_description:
 * @sel: A MateComponentSelectorWidget widget.
 *
 * Returns: A newly-allocated string containing the description of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
matecomponent_selector_widget_get_description (MateComponentSelectorWidget *sel)
{
	return GET_CLASS (sel)->get_description (sel);
}

static void
row_activated (GtkTreeView          *tree_view,
	       GtkTreePath          *path,
	       GtkTreeViewColumn    *column,
	       MateComponentSelectorWidget *sel)
{
/* FIXME: oddness here */
#if 0
	if (event && event->type == GDK_2BUTTON_PRESS) {
		g_signal_emit (sel, signals [FINAL_SELECT], 0, NULL);

	} else 
#endif
	{
		gchar *text;

		text = get_field (sel, 1);
		gtk_label_set_text (GTK_LABEL (sel->priv->desc_label), text);
		g_free (text);
	}
}

static void
matecomponent_selector_widget_init (MateComponentSelectorWidget *widget)
{
	MateComponentSelectorWidget *sel = MATECOMPONENT_SELECTOR_WIDGET (widget);
	GtkWidget *scrolled, *pixmap;
	GtkWidget *hbox;
	GtkWidget *frame;
	MateComponentSelectorWidgetPrivate *priv;
	GdkPixbuf *pixbuf;
	
	g_return_if_fail (sel != NULL);

	priv = sel->priv = g_new0 (MateComponentSelectorWidgetPrivate, 1);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	priv->list_store = gtk_list_store_new (
		3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	priv->list_view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (
		GTK_TREE_MODEL (priv->list_store)));

	gtk_tree_view_insert_column_with_attributes (
		priv->list_view,0, _("Name"),
		gtk_cell_renderer_text_new (),
		"text", 0, NULL);

/*	gtk_tree_view_insert_column_with_attributes (
		priv->list_view,0, _("Description"),
		gtk_cell_renderer_text_new (),
		"text", 1, NULL); */

	gtk_tree_selection_set_mode (
		gtk_tree_view_get_selection (priv->list_view),
		GTK_SELECTION_BROWSE);

	g_signal_connect (priv->list_view, "row_activated",
			  G_CALLBACK (row_activated), sel);

	gtk_tree_view_set_headers_clickable (priv->list_view, FALSE);

	gtk_container_add (GTK_CONTAINER (scrolled),
			   GTK_WIDGET (priv->list_view));
	gtk_box_pack_start (GTK_BOX (sel), scrolled, TRUE, TRUE, 0);

	frame = gtk_frame_new (_("Description"));
	gtk_box_pack_start (GTK_BOX (sel), frame, FALSE, TRUE, 0);
	
	priv->desc_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (priv->desc_label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (priv->desc_label), TRUE);
	gtk_label_set_justify (GTK_LABEL (priv->desc_label), GTK_JUSTIFY_LEFT);

	hbox = gtk_hbox_new (FALSE, 0);

	pixbuf = gdk_pixbuf_new_from_xpm_data (matecomponent_insert_component_xpm);
	pixmap = gtk_image_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);

	gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, TRUE, MATECOMPONENT_UI_PAD_SMALL);
	
	gtk_box_pack_start (GTK_BOX (hbox), priv->desc_label, TRUE, TRUE, MATECOMPONENT_UI_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	gtk_widget_show_all (GTK_WIDGET (widget));
}

static void
impl_set_interfaces (MateComponentSelectorWidget *widget,
		     const char           **required_interfaces)
{
	MateComponentSelectorWidgetPrivate *priv;
	
	g_return_if_fail (widget != NULL);
	
	priv = widget->priv;
	
	g_return_if_fail (priv->list_view != NULL);

	gtk_list_store_clear (priv->list_store);

	get_filtered_objects (priv, required_interfaces);
}

void
matecomponent_selector_widget_set_interfaces (MateComponentSelectorWidget *widget,
				       const char           **required_interfaces)
{
	GET_CLASS (widget)->set_interfaces (widget, required_interfaces);
}

/**
 * matecomponent_selector_widget_new:
 *
 * Creates a new MateComponentSelectorWidget widget, this contains
 * a list and a description pane for each component.
 *
 * Returns: A pointer to the newly-created MateComponentSelectorWidget widget.
 */
GtkWidget *
matecomponent_selector_widget_new (void)
{
	return g_object_new (matecomponent_selector_widget_get_type (), NULL);
}

static void
matecomponent_selector_widget_class_init (MateComponentSelectorWidgetClass *klass)
{
	GObjectClass *object_class;
	
	g_return_if_fail (klass != NULL);
	
	object_class = (GObjectClass *) klass;

	klass->get_id          = impl_get_id;
	klass->get_name        = impl_get_name;
	klass->get_description = impl_get_description;
	klass->set_interfaces  = impl_set_interfaces;

	signals [FINAL_SELECT] = g_signal_new (
		"final_select",
		G_TYPE_FROM_CLASS (object_class),
		G_SIGNAL_RUN_FIRST,
		G_STRUCT_OFFSET (MateComponentSelectorWidgetClass,
				 final_select),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

	object_class->finalize = matecomponent_selector_widget_finalize;
}

