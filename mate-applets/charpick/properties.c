/* properties.c -- properties dialog box and session management for character
 * picker applet. Much of this is adapted from modemlights/properties.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "charpick.h"

#include <string.h>

#include <atk/atkrelation.h>
#include <gtk/gtk.h>

#define CHARPICK_STOCK_EDIT "charpick-stock-edit"

void
register_stock_for_edit (void)
{
  static gboolean registered = FALSE;
  if (!registered)
  {
    GtkIconFactory *factory;
    GtkIconSet     *icons;
                                                                                
    static const GtkStockItem edit_item [] = {
           { CHARPICK_STOCK_EDIT, N_("_Edit"), 0, 0, GETTEXT_PACKAGE },
    };
    icons = gtk_icon_factory_lookup_default (GTK_STOCK_PREFERENCES);
    factory = gtk_icon_factory_new ();
    gtk_icon_factory_add (factory, CHARPICK_STOCK_EDIT, icons);
    gtk_icon_factory_add_default (factory);
    gtk_stock_add_static (edit_item, 1);
    registered = TRUE;
  }
}

#if 0
static void
set_atk_relation (GtkWidget *label, GtkWidget *widget)
{
  AtkObject *atk_widget;
  AtkObject *atk_label;
  AtkRelationSet *relation_set;
  AtkRelation *relation;
  AtkObject *targets[1];

  atk_widget = gtk_widget_get_accessible (widget);
  atk_label = gtk_widget_get_accessible (label);
  
  /* set label-for relation */
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);	

  /* return if gail is not loaded */
  if (GTK_IS_ACCESSIBLE (atk_widget) == FALSE)
    return;

  /* set label-by relation */
  relation_set = atk_object_ref_relation_set (atk_widget);
  targets[0] = atk_label;
  relation = atk_relation_new (targets, 1, ATK_RELATION_LABELLED_BY);
  atk_relation_set_add (relation_set, relation);
  g_object_unref (G_OBJECT (relation)); 
}
#endif /* 0 */

/* sets accessible name and description */

static void
set_access_namedesc (GtkWidget *widget, const gchar *name, const gchar *desc)
{
    AtkObject *obj;

    obj = gtk_widget_get_accessible (widget);
    if (! GTK_IS_ACCESSIBLE (obj))
       return;

    if ( desc )
       atk_object_set_description (obj, desc);
    if ( name )
       atk_object_set_name (obj, name);
}

void
add_edit_dialog_create (charpick_data *curr_data, gchar *string, gchar *title)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *dbox;
	GtkWidget *vbox, *hbox;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons (_(title), GTK_WINDOW (curr_data->propwindow),
							    GTK_DIALOG_DESTROY_WITH_PARENT |
							    GTK_DIALOG_NO_SEPARATOR, 
							    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							    GTK_STOCK_OK, GTK_RESPONSE_OK,
							    NULL);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (curr_data->propwindow));
	gtk_widget_set_sensitive (curr_data->propwindow, FALSE);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (dialog))), 2);

	dbox = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	
	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (dbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = gtk_label_new_with_mnemonic (_("_Palette:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		
	entry = gtk_entry_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
	 gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

	set_access_namedesc (entry, _("Palette entry"),
				         _("Modify a palette by adding or removing characters"));
	if (string)
		gtk_entry_set_text (GTK_ENTRY (entry), string);

	curr_data->add_edit_dialog = dialog;
	curr_data->add_edit_entry = entry;
}

static void
add_palette_cb (GtkDialog *dialog, int response_id, charpick_data *curr_data)
{
	GList *list = curr_data->chartable;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreePath *path;
	char *new;

	gtk_widget_set_sensitive (curr_data->propwindow, TRUE);

	if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (curr_data->add_edit_dialog);
		return;
	}
	
	new = gtk_editable_get_chars (GTK_EDITABLE (curr_data->add_edit_entry), 0, -1);

	gtk_widget_destroy (curr_data->add_edit_dialog);

	if (!new || strlen (new) == 0)
		return;
	
	list = g_list_append (list, new);

	if (curr_data->chartable == NULL) {
		curr_data->chartable = list;
		curr_data->charlist = curr_data->chartable->data;
		build_table (curr_data);

		if (key_writable (MATE_PANEL_APPLET (curr_data->applet), "current_list"))
			mate_panel_applet_mateconf_set_string (MATE_PANEL_APPLET (curr_data->applet),
						      "current_list", 
					  	       curr_data->charlist, NULL);
	}

	save_chartable (curr_data);
  	populate_menu (curr_data);
  	
  	model = gtk_tree_view_get_model (GTK_TREE_VIEW (curr_data->pref_tree));

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new, 1, new, -1);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (curr_data->pref_tree));
	gtk_tree_selection_select_iter (selection, &iter);

	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (curr_data->pref_tree), path,
						  NULL, FALSE, 0.0, 0.0);

	gtk_tree_path_free (path);
}

static void
edit_palette_cb (GtkDialog *dialog, int response_id, charpick_data *curr_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GList *list;
	char *new, *charlist;

	gtk_widget_set_sensitive (curr_data->propwindow, TRUE);

        if (response_id != GTK_RESPONSE_OK) {
		gtk_widget_destroy (curr_data->add_edit_dialog);
		return;
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (curr_data->pref_tree));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, 1, &charlist, -1);

	new = gtk_editable_get_chars (GTK_EDITABLE (curr_data->add_edit_entry), 0, -1);

	gtk_widget_destroy (curr_data->add_edit_dialog);
	
	if (!new || (g_ascii_strcasecmp (new, charlist) == 0))
		return;
		
	list = g_list_find (curr_data->chartable, charlist);
	list->data = new;
	save_chartable (curr_data);
  	populate_menu (curr_data);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, new, 1, new, -1);
	
	if (g_ascii_strcasecmp (curr_data->charlist, charlist) == 0) {
		curr_data->charlist = new;
		build_table (curr_data);

		if (key_writable (MATE_PANEL_APPLET (curr_data->applet), "current_list"))
			mate_panel_applet_mateconf_set_string (MATE_PANEL_APPLET (curr_data->applet), "current_list", curr_data->charlist, NULL);
	}
	
	g_free (charlist);
}

static void
add_palette (GtkButton *buttonk, charpick_data *curr_data)
{
	if (curr_data->add_edit_dialog == NULL) {
		add_edit_dialog_create (curr_data, NULL, _("Add Palette"));

		g_signal_connect (curr_data->add_edit_dialog, 
				  "response", 
				  G_CALLBACK (add_palette_cb),
				  curr_data);

		g_signal_connect (curr_data->add_edit_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &curr_data->add_edit_dialog);

		gtk_widget_show_all (curr_data->add_edit_dialog);
	} else {
		gtk_window_set_screen (GTK_WINDOW (curr_data->add_edit_dialog),
				       gtk_widget_get_screen (GTK_WIDGET (curr_data->applet)));

		gtk_window_present (GTK_WINDOW (curr_data->add_edit_dialog));
	}
}

static void
edit_palette (GtkButton *button, charpick_data *curr_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *charlist;

	if (curr_data->add_edit_dialog == NULL) {
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (curr_data->pref_tree));

		if (!gtk_tree_selection_get_selected (selection, &model, &iter))
			return;

		gtk_tree_model_get (model, &iter, 1, &charlist, -1);
		
		add_edit_dialog_create (curr_data, charlist, _("Edit Palette"));

		g_signal_connect (curr_data->add_edit_dialog, 
				  "response", 
				  G_CALLBACK (edit_palette_cb),
				  curr_data);

		g_signal_connect (curr_data->add_edit_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &curr_data->add_edit_dialog);

		gtk_widget_show_all (curr_data->add_edit_dialog);
	} else {
		gtk_window_set_screen (GTK_WINDOW (curr_data->add_edit_dialog),
				       gtk_widget_get_screen (GTK_WIDGET (curr_data->applet)));

		gtk_window_present (GTK_WINDOW (curr_data->add_edit_dialog));
	}
}

static void
delete_palette (GtkButton *button, charpick_data *curr_data)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter, next;
	GtkTreeModel *model;
	gchar *charlist;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (curr_data->pref_tree));
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
		
	gtk_tree_model_get (model, &iter, 1, &charlist, -1);
	
	curr_data->chartable = g_list_remove (curr_data->chartable, charlist);
	
	if (g_ascii_strcasecmp (curr_data->charlist, charlist) == 0) {
		curr_data->charlist = curr_data->chartable != NULL ? 
				      curr_data->chartable->data : "";
		if (key_writable (MATE_PANEL_APPLET (curr_data->applet), "current_list"))
			mate_panel_applet_mateconf_set_string (MATE_PANEL_APPLET (curr_data->applet), "current_list", curr_data->charlist, NULL);
	}
	g_free (charlist);
	
	save_chartable (curr_data);
  	populate_menu (curr_data);
  	
  	gtk_widget_grab_focus (curr_data->pref_tree);
  	next = iter;
	if (gtk_tree_model_iter_next (model, &next) )
		gtk_tree_selection_select_iter (selection, &next);
	else {
		GtkTreePath *path;
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_prev (path))
			gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);
	}
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

}

static void
selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkWidget *scrolled = data;
	GtkWidget *edit_button, *delete_button;
	gboolean selected;
	
	selected = gtk_tree_selection_get_selected (selection, NULL, NULL);
	edit_button = g_object_get_data (G_OBJECT (scrolled), "edit_button");
	gtk_widget_set_sensitive (edit_button, selected);
	delete_button = g_object_get_data (G_OBJECT (scrolled), "delete_button");
	gtk_widget_set_sensitive (delete_button, selected);
}

static GtkWidget *
create_palettes_tree (charpick_data *curr_data, GtkWidget *label)
{
	GtkWidget *scrolled;
	GtkWidget *tree;
	GtkListStore *model;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GList *list = curr_data->chartable;
	GtkTreeSelection *selection;
	
	scrolled = gtk_scrolled_window_new (NULL,NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
	
	model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	curr_data->pref_tree = tree;
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tree);
	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	set_access_namedesc (tree, 
				         _("Palettes list"),
				         _("List of available palettes"));
	g_object_unref (G_OBJECT (model));
	cell = gtk_cell_renderer_text_new ();
  	column = gtk_tree_view_column_new_with_attributes ("hello",
						    	   cell,
						     	   "text", 0,
						     	   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
        
        while (list) {
        	GtkTreeIter iter;
        	gchar *charlist = list->data;
        	
        	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, charlist, 1, charlist, -1);
        
        	list = g_list_next (list);
        }
        
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
        g_signal_connect (G_OBJECT (selection), "changed",
        			   G_CALLBACK (selection_changed), scrolled);

	return scrolled;

}

static GtkWidget *
create_hig_catagory (GtkWidget *main_box, gchar *title)
{
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *label;
	gchar *tmp;
		
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);

	tmp = g_strdup_printf ("<b>%s</b>", title);
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

	return vbox2;
		
}

static void default_chars_frame_create(charpick_data *curr_data)
{
  GtkWidget *dialog = curr_data->propwindow;
  GtkWidget *dbox, *vbox, *vbox1, *vbox2, *vbox3;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scrolled;
  GtkWidget *button;

  dbox = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

  vbox = gtk_vbox_new (FALSE, 18);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_box_pack_start (GTK_BOX (dbox), vbox, TRUE, TRUE, 0);

  vbox1 = create_hig_catagory (vbox, _("Character Palette")); 
  
  vbox3 = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox1), vbox3, TRUE, TRUE, 0);
  
  label = gtk_label_new_with_mnemonic(_("_Palettes:"));
  gtk_box_pack_start(GTK_BOX(vbox3), label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show(label);
	  
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox, TRUE, TRUE, 0); 
  scrolled = create_palettes_tree (curr_data, label);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled, TRUE, TRUE, 0);
  
  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (add_palette), curr_data);
  set_access_namedesc (button, _("Add button"),
				         _("Click to add a new palette"));
 
  button = gtk_button_new_from_stock (CHARPICK_STOCK_EDIT);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (edit_palette), curr_data);
  g_object_set_data (G_OBJECT (scrolled), "edit_button", button);
  set_access_namedesc (button, _("Edit button"),
				         _("Click to edit the selected palette"));
  
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
  			     G_CALLBACK (delete_palette), curr_data);
  g_object_set_data (G_OBJECT (scrolled), "delete_button", button);
  set_access_namedesc (button, _("Delete button"),
				         _("Click to delete the selected palette"));

  if ( ! key_writable (MATE_PANEL_APPLET (curr_data->applet), "chartable"))
	  gtk_widget_set_sensitive (vbox3, FALSE);
   
  return;
}

static void
phelp_cb (GtkDialog *dialog, gint tab, gpointer data)
{
  GError *error = NULL;

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
                "ghelp:char-palette?charpick-prefs",
                gtk_get_current_event_time (),
                &error);

  if (error) { /* FIXME: the user needs to see this */
    g_warning ("help error: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
response_cb (GtkDialog *dialog, gint id, gpointer data)
{
  charpick_data *curr_data = data;

  if(id == GTK_RESPONSE_HELP){
    phelp_cb (dialog,id,data);
    return;
  }
  
  gtk_widget_destroy (curr_data->propwindow);
  curr_data->propwindow = NULL;
  
}

void
show_preferences_dialog (GtkAction     *action,
			 charpick_data *curr_data)
{
  if (curr_data->propwindow) {
    gtk_window_set_screen (GTK_WINDOW (curr_data->propwindow),
			   gtk_widget_get_screen (curr_data->applet));
    gtk_window_present (GTK_WINDOW (curr_data->propwindow));
    return;
  }

  curr_data->propwindow = gtk_dialog_new_with_buttons (_("Character Palette Preferences"), 
  					    NULL,
					    GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					    GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					    NULL);
  gtk_window_set_screen (GTK_WINDOW (curr_data->propwindow),
			 gtk_widget_get_screen (curr_data->applet));
  gtk_window_set_default_size (GTK_WINDOW (curr_data->propwindow), 350, 350);
  gtk_container_set_border_width (GTK_CONTAINER (curr_data->propwindow), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (curr_data->propwindow))), 2);
  gtk_dialog_set_default_response (GTK_DIALOG (curr_data->propwindow), GTK_RESPONSE_CLOSE);
  gtk_dialog_set_has_separator (GTK_DIALOG (curr_data->propwindow), FALSE);

  default_chars_frame_create(curr_data);
  g_signal_connect (G_OBJECT (curr_data->propwindow), "response",
  		    G_CALLBACK (response_cb), curr_data);
  		    
  gtk_widget_show_all (curr_data->propwindow);
}
