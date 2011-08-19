/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
#include <libmateui/mate-init.h>
#include <libmateui/mate-mdi.h>
#include <libmateui/mate-mdi-generic-child.h>
#include <libmateui/mate-mdi-child.h>
#include <matecomponent/matecomponent-ui-component.h>

static MateMDI *mdi;
static gint child_counter = 1;

#define COUNTER_KEY "counter"

static void
exit_cb(GtkWidget *w, gpointer user_data)
{
	gtk_main_quit();
}

static void
quit_cmd (MateComponentUIComponent *uic,
		  gpointer           user_data,
		  const char        *verbname)
{
	gtk_main_quit();
}

static void
mode_cb(GtkWidget *w, gpointer user_data)
{
	gpointer real_user_data;
	MateMDIMode mode;

	real_user_data = gtk_object_get_data(GTK_OBJECT(w), MATEUIINFO_KEY_UIDATA);
	mode = GPOINTER_TO_INT(real_user_data);
	mate_mdi_set_mode(mdi, mode);
}

static GtkWidget *
view_creator(MateMDIChild *child, gpointer user_data)
{
	GtkWidget *label;

	label = gtk_label_new(mate_mdi_child_get_name(child));
	
	return label;
}

static gchar *
child_config(MateMDIChild *child, gpointer user_data)
{
	gchar *conf;
	guint *counter;

	counter = gtk_object_get_data(GTK_OBJECT(child), COUNTER_KEY);
	if(counter)
		conf = g_strdup_printf("%d", *counter);
	else
		conf = NULL;

	return conf;
}

static void
add_view_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;

	view = mate_mdi_get_active_view(mdi);
	if(!view)
		return;
	mate_mdi_add_view(mdi, mate_mdi_get_child_from_view(view));
}

static void
remove_view_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;

	view = mate_mdi_get_active_view(mdi);
	if(!view)
		return;
	mate_mdi_remove_view(mdi, view);
}

static void
increase_cb(GtkWidget *w, gpointer user_data)
{
	GtkWidget *view;
	MateMDIChild *child;
	const GList *view_node;
	gint *counter;
	gchar *name;

	view = mate_mdi_get_active_view(mdi);
	if(!view)
		return;
	child = mate_mdi_get_child_from_view(view);
	counter = gtk_object_get_data(GTK_OBJECT(child), COUNTER_KEY);
	(*counter) = child_counter++;
	name = g_strdup_printf("Child #%d", *counter);
	mate_mdi_child_set_name(child, name);
	g_free(name);
	view_node = mate_mdi_child_get_views(child);
	while(view_node) {
		view = view_node->data;
		gtk_label_set(GTK_LABEL(view), mate_mdi_child_get_name(child));
		view_node = view_node->next;
	}
}

MateUIInfo real_child_menu[] =
{
	MATEUIINFO_ITEM("Add view", NULL, add_view_cb, NULL),
	MATEUIINFO_ITEM("Remove view", NULL, remove_view_cb, NULL),
	MATEUIINFO_SEPARATOR,
	MATEUIINFO_ITEM("Increase counter", NULL, increase_cb, NULL),
	MATEUIINFO_END
};

static MateUIInfo child_menu[] =
{
	MATEUIINFO_SUBTREE("Child", real_child_menu),
	MATEUIINFO_END
};

static MateUIInfo child_toolbar[] =
{
	MATEUIINFO_ITEM_NONE(N_("Child Item 1"),
						  N_("Hint for item 1"),
						  NULL),
	MATEUIINFO_ITEM_NONE(N_("Child Item 2"),
						  N_("Hint for item 2"),
						  NULL),
	MATEUIINFO_END
};

static void
add_child_cmd (MateComponentUIComponent *uic,
			   gpointer           user_data,
			   const char        *verbname)
{
	MateMDIGenericChild *child;
	gchar *name;
	gint *counter;

	name = g_strdup_printf("Child #%d", child_counter);
	child = mate_mdi_generic_child_new(name);
	mate_mdi_generic_child_set_view_creator(child, view_creator, NULL);
	mate_mdi_generic_child_set_config_func(child, child_config, NULL);
	mate_mdi_child_set_menu_template(MATE_MDI_CHILD(child), child_menu);
	mate_mdi_child_set_toolbar_template(MATE_MDI_CHILD(child), child_toolbar);
	mate_mdi_child_set_toolbar_position(MATE_MDI_CHILD(child), MATE_DOCK_ITEM_BEH_NORMAL,
										 MATE_DOCK_TOP, 1, 1, 0);
	counter = g_malloc(sizeof(gint));
	*counter = child_counter++;
	gtk_object_set_data(GTK_OBJECT(child), COUNTER_KEY, counter);
	g_free(name);
	mate_mdi_add_child(mdi, MATE_MDI_CHILD(child));
	mate_mdi_add_view(mdi, MATE_MDI_CHILD(child));
}

static void
remove_child_cmd (MateComponentUIComponent *uic,
				  gpointer           user_data,
				  const char        *verbname)
{
	GtkWidget *view;

	view = mate_mdi_get_active_view(mdi);
	if(!view)
		return;
	mate_mdi_remove_child(mdi, mate_mdi_get_child_from_view(view));
}

MateUIInfo empty_menu[] =
{
	MATEUIINFO_END
};

MateUIInfo mode_list [] = 
{
	MATEUIINFO_RADIOITEM_DATA("Toplevel", NULL, mode_cb, GINT_TO_POINTER(MATE_MDI_TOPLEVEL), NULL),
	MATEUIINFO_RADIOITEM_DATA("Notebook", NULL, mode_cb, GINT_TO_POINTER(MATE_MDI_NOTEBOOK), NULL),
	MATEUIINFO_RADIOITEM_DATA("Window-in-Window", NULL, mode_cb, GINT_TO_POINTER(MATE_MDI_WIW), NULL),
	MATEUIINFO_RADIOITEM_DATA("Modal", NULL, mode_cb, GINT_TO_POINTER(MATE_MDI_MODAL), NULL),
	MATEUIINFO_END
};

MateUIInfo mode_menu[] =
{
	MATEUIINFO_RADIOLIST(mode_list),
	MATEUIINFO_END
};

MateUIInfo mdi_menu[] =
{
#if 0
	MATEUIINFO_ITEM("Add child", NULL, add_child_cb, NULL),
	MATEUIINFO_ITEM("Remove child", NULL, remove_child_cb, NULL),
#endif
	MATEUIINFO_SEPARATOR,
	MATEUIINFO_SUBTREE("Mode", mode_menu),
	MATEUIINFO_SEPARATOR,
	MATEUIINFO_MENU_EXIT_ITEM(exit_cb, NULL),
	MATEUIINFO_END
};

MateUIInfo main_menu[] =
{
	MATEUIINFO_SUBTREE("MDI", mdi_menu),
	MATEUIINFO_SUBTREE("Children", empty_menu),
	MATEUIINFO_END
};

static const gchar *menu_xml = 
"<menu>\n"
"    <submenu name=\"MDI\" _label=\"_MDI\">\n"
"        <menuitem name=\"AddChild\" verb=\"\" _label=\"Add Child\"/>\n"
"        <menuitem name=\"RemoveChild\" verb=\"\" _label=\"Remove Child\"/>\n"
"        <separator/>"
"        <menuitem name=\"FileExit\" verb=\"\" _label=\"Exit\"/>\n"
"    </submenu>\n"
"    <submenu name=\"Children\" _label=\"_Children\">\n"
"        <placeholder/>\n"
"    </submenu>\n"
"</menu>\n";

MateComponentUIVerb verbs [] = {
    MATECOMPONENT_UI_VERB ("FileExit", quit_cmd),

    MATECOMPONENT_UI_VERB ("AddChild", add_child_cmd),
    MATECOMPONENT_UI_VERB ("RemoveChild", remove_child_cmd),

    MATECOMPONENT_UI_VERB_END
};

static void
app_created_cb (MateMDI *mdi, MateComponentWindow *win, MateComponentUIComponent *component)
{
	matecomponent_ui_component_add_verb_list (component, verbs);
}

int
main(int argc, char **argv)
{
  MateProgram *program;

  program = mate_program_init ("mdi_demo", "2.0", &libmateui_module_info,
					  argc, argv, NULL);

#if 1
    g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
#endif

  mdi = mate_mdi_new("MDIDemo", "MDI Demo");
  mate_mdi_set_mode(mdi, MATE_MDI_TOPLEVEL);
  mate_mdi_set_menubar_template(mdi, menu_xml);
  mate_mdi_set_child_menu_path(mdi, "MDI");
  mate_mdi_set_child_list_path(mdi, "/menu/Children");
  g_signal_connectc(G_OBJECT(mdi), "app_created",
					G_CALLBACK(app_created_cb), NULL, FALSE);
  gtk_signal_connect(GTK_OBJECT(mdi), "destroy",
					 GTK_SIGNAL_FUNC(exit_cb), NULL);
  mate_mdi_open_toplevel(mdi);

  gtk_main ();

  g_object_unref (program);

  return 0;
}
