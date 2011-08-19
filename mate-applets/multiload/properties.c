/* MATE cpuload/memload panel applet
 * (C) 2002 The Free Software Foundation
 *
 * Authors: 
 *		  Todd Kulesza
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>

#include "global.h"

#define PROP_CPU		0
#define PROP_MEM		1
#define PROP_NET		2
#define PROP_SWAP		3
#define PROP_AVG		4
#define PROP_DISK		5

#define PROP_SPEED		6
#define PROP_SIZE		7
#define HIG_IDENTATION		"    "
#define NEVER_SENSITIVE		"never_sensitive"

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	gtk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		gtk_widget_set_sensitive (w, FALSE);
	else
		gtk_widget_set_sensitive (w, sensitivity);
}


static gboolean
key_writable (MatePanelApplet *applet, const char *key)
{
	gboolean writable;
	char *fullkey;
	static MateConfClient *client = NULL;
	if (client == NULL)
		client = mateconf_client_get_default ();

	fullkey = mate_panel_applet_mateconf_get_full_key (applet, key);

	writable = mateconf_client_key_is_writable (client, fullkey, NULL);

	g_free (fullkey);

	return writable;
}


static void
properties_set_insensitive(MultiloadApplet *ma)
{
	gint i, total_graphs, last_graph;
	
	total_graphs = 0;
	last_graph = 0;
		
	for (i = 0; i < NGRAPHS; i++)
		if (ma->graphs[i]->visible)
		{
			last_graph = i;
			total_graphs++;
		}
			
	if (total_graphs < 2)
		soft_set_sensitive(ma->check_boxes[last_graph], FALSE);
		
	return;
}

static void
properties_close_cb (GtkWidget *widget, gint arg, MultiloadApplet *ma)
{
	GError *error = NULL;

	switch (arg)
	{
		case GTK_RESPONSE_HELP:

			gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (ma->applet)),
					"ghelp:multiload?multiload-prefs",
					gtk_get_current_event_time (),
					&error);

			if (error) { /* FIXME: the user needs to see this */
				g_warning ("help error: %s\n", error->message);
				g_error_free (error);
				error = NULL;
			}
			break;
			
		case GTK_RESPONSE_CLOSE:
		default:
			gtk_widget_destroy (widget);
			ma->prop_dialog = NULL;
	}
}

static void
property_toggled_cb(GtkWidget *widget, gpointer name)
{
	MultiloadApplet *ma;
	gint prop_type, i;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	
	ma = g_object_get_data(G_OBJECT(widget), "MultiloadApplet");
	prop_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "prop_type"));
	
	/* FIXME: the first toggle button to be checked/dechecked does not work, but after that everything is cool.  what gives? */
	mate_panel_applet_mateconf_set_bool(ma->applet, (gchar *)name, 
			active, NULL);
	
	mate_panel_applet_mateconf_set_bool(ma->applet, (gchar *)name, 
			active, NULL);
	
	if (active)
	{
		for (i = 0; i < NGRAPHS; i++)
			soft_set_sensitive(ma->check_boxes[i], TRUE);	
		gtk_widget_show_all (ma->graphs[prop_type]->main_widget);
		ma->graphs[prop_type]->visible = TRUE;
		load_graph_start(ma->graphs[prop_type]);
	}
	else
	{
		load_graph_stop(ma->graphs[prop_type]);
		gtk_widget_hide (ma->graphs[prop_type]->main_widget);
		ma->graphs[prop_type]->visible = FALSE;
		properties_set_insensitive(ma);
	}
	
	return;
}

static void
spin_button_changed_cb(GtkWidget *widget, gpointer name)
{
	MultiloadApplet *ma;
	gint value, prop_type, i;
	
	ma = g_object_get_data(G_OBJECT(widget), "MultiloadApplet");
	prop_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "prop_type"));
	value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	
	/* FIXME: the first toggle button to be checked/dechecked does not work, but after that everything is cool.  what gives? */
	mate_panel_applet_mateconf_set_int(ma->applet, (gchar *)name, 
			value, NULL);
	
	mate_panel_applet_mateconf_set_int(ma->applet, (gchar *)name, 
			value, NULL);
	
	switch(prop_type)
	{
		case PROP_SPEED:
		{
			for (i = 0; i < NGRAPHS; i++)
			{
				load_graph_stop(ma->graphs[i]);
				ma->graphs[i]->speed = value;
				if (ma->graphs[i]->visible)
					load_graph_start(ma->graphs[i]);
			}
			
			break;
		}
		case PROP_SIZE:
		{
			for (i = 0; i < NGRAPHS; i++)
			{
				ma->graphs[i]->size = value ;
				
				if (ma->graphs[i]->orient)
					gtk_widget_set_size_request (
						ma->graphs[i]->main_widget, 
						ma->graphs[i]->pixel_size, 
						ma->graphs[i]->size);
			    else
					gtk_widget_set_size_request (
						ma->graphs[i]->main_widget, 
						ma->graphs[i]->size, 
						ma->graphs[i]->pixel_size);
			}
			
			break;
		}
		default:
			g_assert_not_reached();
	}
	
	return;
}

/* create a new page in the notebook widget, add it, and return a pointer to it */
static GtkWidget *
add_page(GtkWidget *notebook, gchar *label)
{
	GtkWidget *page;
	GtkWidget *page_label;
	
	page = gtk_hbox_new(TRUE, 0);
	page_label = gtk_label_new(label);
	gtk_container_set_border_width(GTK_CONTAINER(page), 6);
		
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, page_label);
	
	return page;
}

/* save the selected color to mateconf and apply it on the applet */
static void
color_picker_set_cb(GtkColorButton *color_picker, gpointer data)
{
	gchar color_string[8];
	const gchar *mateconf_path;
	guint8 prop_type;
	GdkColor color;
	MultiloadApplet *ma;

	mateconf_path = data;
	ma = g_object_get_data (G_OBJECT (color_picker), "MultiloadApplet");

	prop_type = 0;
	
	if (strstr(mateconf_path, "cpuload"))
		prop_type = PROP_CPU;
	else if (strstr(mateconf_path, "memload"))
		prop_type = PROP_MEM;
	else if (strstr(mateconf_path, "netload2"))
		prop_type = PROP_NET;
	else if (strstr(mateconf_path, "swapload"))
		prop_type = PROP_SWAP;
	else if (strstr(mateconf_path, "loadavg"))
		prop_type = PROP_AVG;
	else if (strstr(mateconf_path, "diskload"))
		prop_type = PROP_DISK;
	else
		g_assert_not_reached();
		
	gtk_color_button_get_color(color_picker, &color);
	
	snprintf(color_string, 8, "#%02X%02X%02X", 
		 color.red / 256, color.green / 256, color.blue / 256);
	mate_panel_applet_mateconf_set_string(MATE_PANEL_APPLET(ma->applet), mateconf_path, color_string, NULL);
	
	gdk_color_parse(color_string, 
			&(ma->graphs[prop_type]->colors[g_ascii_digit_value(mateconf_path[strlen(mateconf_path) - 1]) ]) );
	
	ma->graphs[prop_type]->colors_allocated = FALSE;
	
	return;
}

/* create a color selector */
static void
add_color_selector(GtkWidget *page, gchar *name, gchar *mateconf_path, MultiloadApplet *ma)
{
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *color_picker;
	GdkColor color;
	gchar *color_string;
	
	color_string = mate_panel_applet_mateconf_get_string(ma->applet, mateconf_path, NULL);
	if (!color_string)
		color_string = g_strdup ("#000000");
	color.red   = (g_ascii_xdigit_value(color_string[1]) * 16 
                       + g_ascii_xdigit_value(color_string[2])) * 256;
	color.green = (g_ascii_xdigit_value(color_string[3]) * 16 
                       + g_ascii_xdigit_value(color_string[4])) * 256;
	color.blue  = (g_ascii_xdigit_value(color_string[5]) * 16
                       + g_ascii_xdigit_value(color_string[6])) * 256;
	g_free (color_string);
		
	vbox = gtk_vbox_new (FALSE, 6);
	label = gtk_label_new_with_mnemonic(name);
	color_picker = gtk_color_button_new();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), color_picker);
	
	gtk_box_pack_start(GTK_BOX(vbox), color_picker, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(page), vbox, FALSE, FALSE, 0);	
	
	g_object_set_data (G_OBJECT (color_picker), "MultiloadApplet", ma);

	gtk_color_button_set_color(GTK_COLOR_BUTTON(color_picker), &color);

	g_signal_connect(G_OBJECT(color_picker), "color_set", G_CALLBACK(color_picker_set_cb), mateconf_path);

	if ( ! key_writable (ma->applet, mateconf_path))
		hard_set_sensitive (vbox, FALSE);
	
	return;
}

/* creates the properties dialog using up-to-the-minute info from mateconf */
static void
fill_properties(GtkWidget *dialog, MultiloadApplet *ma)
{
	GtkWidget *page;
	GtkWidget *hbox, *vbox;
	GtkWidget *categories_vbox;
	GtkWidget *category_vbox;
	GtkWidget *control_vbox;
	GtkWidget *control_hbox;
	GtkWidget *check_box;
	GtkWidget *indent;
	GtkWidget *spin_button;
	GtkWidget *label;
	MatePanelAppletOrient orient;
	GtkSizeGroup *label_size;
	GtkSizeGroup *spin_size;
	gchar *label_text;
	gchar *title;

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show (vbox);
	
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox,
			    TRUE, TRUE, 0);

	categories_vbox = gtk_vbox_new (FALSE, 18);
	gtk_box_pack_start (GTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);
	gtk_widget_show (categories_vbox);

	category_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	gtk_widget_show (category_vbox);
	
	title = g_strconcat ("<span weight=\"bold\">", _("Monitored Resources"), "</span>", NULL);
	label = gtk_label_new_with_mnemonic (_(title));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	g_free (title);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	
	indent = gtk_label_new (HIG_IDENTATION);
	gtk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	gtk_widget_show (indent);
	
	control_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	gtk_widget_show (control_vbox);
	
	control_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	gtk_widget_show (control_hbox);
	
	check_box = gtk_check_button_new_with_mnemonic(_("_Processor"));
	ma->check_boxes[0] = check_box;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				mate_panel_applet_mateconf_get_bool(ma->applet, "view_cpuload", NULL));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_CPU));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view_cpuload");
	gtk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "view_cpuload"))
		hard_set_sensitive (check_box, FALSE);
	
	check_box = gtk_check_button_new_with_mnemonic(_("_Memory"));
	ma->check_boxes[1] = check_box;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				mate_panel_applet_mateconf_get_bool(ma->applet, "view_memload", NULL));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_MEM));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view_memload");
	gtk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "view_memload"))
		hard_set_sensitive (check_box, FALSE);
	
	check_box = gtk_check_button_new_with_mnemonic(_("_Network"));
	ma->check_boxes[2] = check_box;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				mate_panel_applet_mateconf_get_bool(ma->applet, "view_netload", NULL));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_NET));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view_netload");
	gtk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "view_netload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = gtk_check_button_new_with_mnemonic (_("S_wap Space"));
	ma->check_boxes[3] = check_box;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				mate_panel_applet_mateconf_get_bool(ma->applet, "view_swapload", NULL));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_SWAP));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view_swapload");
	gtk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "view_swapload"))
		hard_set_sensitive (check_box, FALSE);

	check_box = gtk_check_button_new_with_mnemonic(_("_Load"));
	ma->check_boxes[4] = check_box;	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box),
				mate_panel_applet_mateconf_get_bool(ma->applet, "view_loadavg", NULL));
	g_object_set_data(G_OBJECT(check_box), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(check_box), "prop_type", GINT_TO_POINTER(PROP_AVG));
	g_signal_connect(G_OBJECT(check_box), "toggled",
				G_CALLBACK(property_toggled_cb), "view_loadavg");
	gtk_box_pack_start(GTK_BOX(control_hbox), check_box, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "view_loadavg"))
		hard_set_sensitive (check_box, FALSE);

	check_box = gtk_check_button_new_with_mnemonic(_("_Harddisk"));
	ma->check_boxes[5] = check_box;
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_box),
			mate_panel_applet_mateconf_get_bool (ma->applet,
				"view_diskload", NULL));
	g_object_set_data (G_OBJECT (check_box), "MultiloadApplet", ma);
	g_object_set_data (G_OBJECT (check_box), "prop_type",
			GINT_TO_POINTER (PROP_DISK));
	g_signal_connect (G_OBJECT (check_box), "toggled",
			G_CALLBACK (property_toggled_cb), "view_diskload");
	gtk_box_pack_start (GTK_BOX (control_hbox), check_box, FALSE, FALSE, 0);

	category_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	gtk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Options"), "</span>", NULL);
	label = gtk_label_new (title);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	g_free (title);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	indent = gtk_label_new (HIG_IDENTATION);
	gtk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	gtk_widget_show (indent);

	control_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	gtk_widget_show (control_vbox);
	
	control_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	gtk_widget_show (control_hbox);
	
	label_size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	orient = mate_panel_applet_get_orient(ma->applet);
	if ( (orient == MATE_PANEL_APPLET_ORIENT_UP) || (orient == MATE_PANEL_APPLET_ORIENT_DOWN) )
		label_text = g_strdup(_("System m_onitor width: "));
	else
		label_text = g_strdup(_("System m_onitor height: "));
	
	label = gtk_label_new_with_mnemonic(label_text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_size_group_add_widget (label_size, label);
        gtk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	spin_size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
			  
	spin_button = gtk_spin_button_new_with_range(10, 1000, 5);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GINT_TO_POINTER(PROP_SIZE));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)mate_panel_applet_mateconf_get_int(ma->applet, "size", NULL));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "size");
	
	if ( ! key_writable (ma->applet, "size")) {
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}

	gtk_size_group_add_widget (spin_size, spin_button);
	gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);
	
	label = gtk_label_new (_("pixels"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	control_hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (control_vbox), control_hbox, TRUE, TRUE, 0);
	gtk_widget_show (control_hbox);
	
	label = gtk_label_new_with_mnemonic(_("Sys_tem monitor update interval: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_size_group_add_widget (label_size, label);
	gtk_box_pack_start (GTK_BOX (control_hbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (control_hbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	
	spin_button = gtk_spin_button_new_with_range(50, 10000, 50);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_object_set_data(G_OBJECT(spin_button), "MultiloadApplet", ma);
	g_object_set_data(G_OBJECT(spin_button), "prop_type",
				GINT_TO_POINTER(PROP_SPEED));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button),
				(gdouble)mate_panel_applet_mateconf_get_int(ma->applet, "speed", NULL));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",
				G_CALLBACK(spin_button_changed_cb), "speed");
	gtk_size_group_add_widget (spin_size, spin_button);
	gtk_box_pack_start (GTK_BOX (hbox), spin_button, FALSE, FALSE, 0);

	if ( ! key_writable (ma->applet, "speed")) {
		hard_set_sensitive (label, FALSE);
		hard_set_sensitive (hbox, FALSE);
	}
	
	label = gtk_label_new(_("milliseconds"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	g_free(label_text);
	
	
	category_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	gtk_widget_show (category_vbox);

	title = g_strconcat ("<span weight=\"bold\">", _("Colors"), "</span>", NULL);
	label = gtk_label_new (title);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	g_free (title);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);

	indent = gtk_label_new (HIG_IDENTATION);
	gtk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	gtk_widget_show (indent);

	control_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	gtk_widget_show (control_vbox);

	ma->notebook = gtk_notebook_new();
	gtk_container_add (GTK_CONTAINER (control_vbox), ma->notebook);
	
	page = add_page(ma->notebook,  _("Processor"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_User"), "cpuload_color0", ma);
	add_color_selector(page, _("S_ystem"), "cpuload_color1", ma);
	add_color_selector(page, _("N_ice"), "cpuload_color2", ma);
	add_color_selector(page, _("I_OWait"), "cpuload_color3", ma);
	add_color_selector(page, _("I_dle"), "cpuload_color4", ma);
	
	page = add_page(ma->notebook,  _("Memory"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_User"), "memload_color0", ma);
	add_color_selector(page, _("Sh_ared"), "memload_color1", ma);
	add_color_selector(page, _("_Buffers"), "memload_color2", ma);
	add_color_selector (page, _("Cach_ed"), "memload_color3", ma);
	add_color_selector(page, _("F_ree"), "memload_color4", ma);
	
	page = add_page(ma->notebook,  _("Network"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector (page, _("_In"), "netload2_color0", ma);
	add_color_selector(page, _("_Out"), "netload2_color1", ma);
	add_color_selector (page, _("_Local"), "netload2_color2", ma);
	add_color_selector(page, _("_Background"), "netload2_color3", ma);
	
	page = add_page(ma->notebook,  _("Swap Space"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_Used"), "swapload_color0", ma);
	add_color_selector(page, _("_Free"), "swapload_color1", ma);
	
	page = add_page(ma->notebook,  _("Load"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector(page, _("_Average"), "loadavg_color0", ma);
	add_color_selector(page, _("_Background"), "loadavg_color1", ma);

	page = add_page (ma->notebook, _("Harddisk"));
	gtk_container_set_border_width (GTK_CONTAINER (page), 12);
	add_color_selector (page, _("_Read"), "diskload_color0", ma);
	add_color_selector (page, _("_Write"), "diskload_color1", ma);
	add_color_selector (page, _("_Background"), "diskload_color2", ma);
	
	return;
}

/* show properties dialog */
void
multiload_properties_cb (GtkAction       *action,
			 MultiloadApplet *ma)
{
	GtkWidget *dialog = NULL;

	if (ma->prop_dialog) {
		dialog = ma->prop_dialog;

		gtk_window_set_screen (GTK_WINDOW (dialog),
				gtk_widget_get_screen (GTK_WIDGET (ma->applet)));

		gtk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
					       ma->last_clicked);
		gtk_window_present (GTK_WINDOW (dialog));
		return;
	}
	
	dialog = gtk_dialog_new_with_buttons (_("System Monitor Preferences"),
					      NULL, 0,
					   GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					   GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	gtk_window_set_screen (GTK_WINDOW (dialog),
			       gtk_widget_get_screen (GTK_WIDGET (ma->applet)));
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);

	fill_properties(dialog, ma);

	properties_set_insensitive(ma);
	
	g_signal_connect(G_OBJECT(dialog), "response",
			 G_CALLBACK(properties_close_cb), ma);

	ma->prop_dialog = dialog;

	gtk_widget_show_all(dialog);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (ma->notebook),
				       ma->last_clicked);
}
