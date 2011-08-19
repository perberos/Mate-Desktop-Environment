/* charpick.c  This is a mate panel applet that allow users to select
 * accented (and other) characters to be pasted into other apps.
 */

#include <config.h>
#include <string.h>
#include <mate-panel-applet.h>
#ifdef HAVE_GUCHARMAP
#	include <gucharmap/gucharmap.h>
#endif
#include "charpick.h"

/* The comment for each char list has the html entity names of the chars */
/* All gunicar codes should end in 0 */


/* This is the default list used when starting charpick the first time */
/* aacute, agrave, eacute, iacute, oacute, frac12, copy*/
/* static const gchar *def_list = "áàéíñóœ©"; */
static const gunichar def_code[] = {225, 224, 233, 237, 241, 243, 189, 169, 1579, 8364, 0};

/* aacute, agrave, acirc, atilde, auml. aring, aelig, ordf */
/* static const gchar *a_list = "áàâãäåæª"; */
static const gunichar a_code[] = {225, 224, 226, 227, 228, 229, 230, 170, 0};
/* static const gchar *cap_a_list = "ÁÀÂÃÄÅÆª"; */
static const gunichar cap_a_code[] = {192, 193, 194, 195, 196, 197, 198, 170, 0}; 
/* ccedil, cent, copy */
/* static const gchar *c_list = "çÇ¢©"; */
static const gunichar c_code[] = {231, 199, 162, 169, 0};
/* eacute, egrave, ecirc, euml, aelig */
/* static const gchar *e_list = "éèêëæ"; */
static const gunichar e_code[] = {232, 233, 234, 235, 230, 0};
/* static const gchar *cap_e_list = "ÉÈÊËÆ"; */
static const gunichar cap_e_code[] = {200, 201, 202, 203, 198, 0};
/* iacute, igrave, icirc, iuml */
/* static const gchar *i_list = "íìîï"; */
static const gunichar i_code[] = {236, 237, 238, 239, 0};
/* static const gchar *cap_i_list = "ÍÌÎÏ"; */
static const gunichar cap_i_code[] = {204, 205, 206, 207, 0};
/* ntilde (this is the most important line in this program.) */
/* static const gchar *n_list = "ñ, Ñ"; */
static const gunichar n_code[] = {241, 209, 0};
/* oacute, ograve, ocirc, otilde, ouml, oslash, ordm */
/* static const gchar *o_list = "óòôõöøº"; */
static const gunichar o_code[] = {242, 243, 244, 245, 246, 248, 176, 0};
/* static const gchar *cap_o_list = "ÓÒÔÕÖØº"; */
static const gunichar cap_o_code[] = {210, 211, 212, 213, 214, 216, 176, 0};
/* szlig, sect, dollar */
/* static const gchar *s_list = "ß§$"; */
static const gunichar s_code[] = {223, 167, 36, 0};
/* eth, thorn */
/* static const gchar *t_list = "ðÐþÞ"; */
static const gunichar t_code[] = {240, 208, 254, 222, 0};
/* uacute, ugrave, ucirc, uuml */
/* static const gchar *u_list = "úùûü"; */
static const gunichar u_code[] = {249, 250, 251, 252, 0};
/* static const gchar *cap_u_list = "ÚÙÛÜ"; */
static const gunichar cap_u_code[] = {217, 218, 219, 220, 0};
/* yacute, yuml, yen Yes, there is no capital yuml in iso 8859-1.*/
/* static const gchar *y_list = "ýÝÿ¥"; */
static const gunichar y_code[] = {253, 221, 255, 165, 0};

/* extra characters unrelated to the latin alphabet. All characters in 
   ISO-8859-1 should now be accounted for.*/
/* not shy macr plusmn */
/* static const gchar *dash_list = "¬­¯±"; */
static const gunichar dash_code[] = {172, 173, 175, 177, 0};
/* laquo raquo uml */
/* static const gchar *quote_list = "«»š·×"; */
static const gunichar quote_code[] = {171, 187, 168, 183, 215, 0};
/* curren, pound, yen, cent, dollar */
/* static const gchar *currency_list = "€£¥¢$"; */
static const gunichar currency_code[] = {164, 163, 165, 162, 36, 8364, 0}; 
/* sup1 frac12 */
/* static const gchar *one_list = "¹œŒ"; */
static const gunichar one_code[] = {185, 178, 179, 188, 189, 190, 0};
/* µ ¶ ® ¿ ¡ |  */
static const gunichar misc_code[] = {181, 182, 174, 191, 161, 124, 0};

/* language/region specific character groups */
/* South Africa: Venda, Tswana and Northern Sotho */
/* static const gchar *ZA_list = "ḓḽṋṱḒḼṊṰṅṄšŠ"; */
static const gunichar ZA_code[] = {7699, 7741, 7755, 7793, 7698, 7740, 7754, 7792, 7749, 7748, 353, 352, 0};
/* South Africa: Afrikaans */
/* static const gchar *af_ZA_list = "áéíóúýêîôûèäëïöüÁÉÍÓÚÝÊÎÔÛÈÄËÏÖÜ"; */
static const gunichar af_ZA_code[] = {225, 233, 237, 243, 250, 253, 234, 238, 244, 251, 232, 228, 235, 239, 246, 252, 193, 201, 205, 211, 218, 221, 202, 206, 212, 219, 200, 196, 203, 207, 214, 220, 0};


static const gunichar * const chartable[] = {
	def_code,
	a_code,
	cap_a_code,
	c_code,
	e_code,
	cap_e_code,
	i_code,
	cap_i_code,
	n_code,
	o_code,
	cap_o_code,
	s_code,
	t_code,
	u_code,
	cap_u_code,
	y_code,
	dash_code,
	quote_code,
	currency_code,
	one_code,
	misc_code,
	ZA_code,
	af_ZA_code
};

gboolean
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


/* sets the picked character as the selection when it gets a request */
static void
charpick_selection_handler(GtkWidget *widget,
			   GtkSelectionData *selection_data,
			   guint info,
			   guint time,
		           gpointer data)
{
  charpick_data *p_curr_data = data;
  gint num;
  gchar tmp[7];
  num = g_unichar_to_utf8 (p_curr_data->selected_unichar, tmp);
  tmp[num] = '\0';
  
  gtk_selection_data_set_text (selection_data, tmp, -1);

  return;
}

/* untoggles the active toggle_button when we lose the selection */
static gint
selection_clear_cb (GtkWidget *widget, GdkEventSelection *event,
                    gpointer data)
{
  charpick_data *curr_data = data;
  
  if (curr_data->last_toggle_button)
    gtk_toggle_button_set_active (curr_data->last_toggle_button, FALSE);

  curr_data->last_toggle_button = NULL;
  return TRUE;
}


static gint
toggle_button_toggled_cb(GtkToggleButton *button, gpointer data)
{
  charpick_data *curr_data = data;
  gint button_index;
  gboolean toggled;
   
  button_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "index")); 
  toggled = gtk_toggle_button_get_active (button); 

  if (toggled)
  { 
    gunichar unichar;
    if (curr_data->last_toggle_button && (button != curr_data->last_toggle_button))
    	gtk_toggle_button_set_active (curr_data->last_toggle_button,
    				      FALSE);
    				      
    curr_data->last_toggle_button = button; 
    gtk_widget_grab_focus(curr_data->applet);
    unichar = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "unichar"));
    curr_data->selected_unichar = unichar;
    /* set this? widget as the selection owner */
    gtk_selection_owner_set (curr_data->applet,
	  		     GDK_SELECTION_PRIMARY,
                             GDK_CURRENT_TIME); 
    gtk_selection_owner_set (curr_data->applet,
	  		     GDK_SELECTION_CLIPBOARD,
                             GDK_CURRENT_TIME); 
    curr_data->last_index = button_index;
  }	
	     
  return TRUE;
}

/* This is a hack around the fact that gtk+ doesn't
 * propogate button presses on button2/3.
 */
static gboolean
button_press_hack (GtkWidget      *widget,
		   GdkEventButton *event,
		   GtkWidget      *applet)
{
    if (event->button == 3 || event->button == 2) {
	gtk_propagate_event (applet, (GdkEvent *) event);

	return TRUE;
    }

    return FALSE;
}

static gint
key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
#if 0
  charpick_data *p_curr_data = data;
  const gunichar *code = NULL;
  gchar inputchar = event->keyval;

  switch (inputchar)
    {
    case 'a' : code = a_code;
               break;
    case 'A' : code = cap_a_code;
               break;
    case 'c' : 
    case 'C' : code = c_code;
               break;
    case 'e' : code = e_code;
               break;
    case 'E' : code = cap_e_code;
               break;
    case 'i' : code = i_code;
               break;
    case 'I' : code =  cap_i_code;
               break;
    case 'n' : 
    case 'N' : code = n_code;
               break;
    case 'o' : code = o_code;
               break;
    case 'O' : code = cap_o_code;
               break;
    case 's' : code = s_code;
               break;
    case 't' : 
    case 'T' : code = t_code;
               break;
    case 'u' : code = u_code;
               break;
    case 'U' : code = cap_u_code;
               break;
    case 'y' : 
    case 'Y' : code = y_code;
               break;
    case '-' : code = dash_code;
               break;
    case '\"' : code = quote_code;
               break;
    case '$' : code = currency_code;
               break;
    case '1' : 
    case '2' :
    case '3' : code = one_code;
               break;
    case 'd' : code = NULL;
               break;
    default :
    		return FALSE;
    }
  /* FIXME: what's wrong here ? */
  if (code)
    p_curr_data->charlist = g_ucs4_to_utf8 (code, -1, NULL, NULL, NULL);
  else
    p_curr_data->charlist = "hello";
  p_curr_data->last_index = NO_LAST_INDEX;
  p_curr_data->last_toggle_button = NULL;
  build_table(p_curr_data);
#endif
  return FALSE;
}

static void
menuitem_activated (GtkMenuItem *menuitem, charpick_data *curr_data)
{
	gchar *string;
	MatePanelApplet *applet = MATE_PANEL_APPLET (curr_data->applet);
	
	string = g_object_get_data (G_OBJECT (menuitem), "string");
	if (g_ascii_strcasecmp (curr_data->charlist, string) == 0)
		return;
	
	curr_data->charlist = string;
	build_table (curr_data);
	if (key_writable (applet, "current_list"))
		mate_panel_applet_mateconf_set_string (applet, "current_list", curr_data->charlist, NULL);
}

void
populate_menu (charpick_data *curr_data)
{
	GList *list = curr_data->chartable;
	GSList *group = NULL;
	GtkMenu *menu;
	GtkWidget *menuitem;

	if (curr_data->menu)
		gtk_widget_destroy (curr_data->menu);

	curr_data->menu = gtk_menu_new ();
	menu  = GTK_MENU (curr_data->menu);
	
	while (list) {
		gchar *string = list->data;
		menuitem = gtk_radio_menu_item_new_with_label (group, string);
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
		gtk_widget_show (menuitem);
		g_object_set_data (G_OBJECT (menuitem), "string", string);
		g_signal_connect (G_OBJECT (menuitem), "activate",
				   G_CALLBACK (menuitem_activated), curr_data);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		if (g_ascii_strcasecmp (curr_data->charlist, string) == 0)
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
		list = g_list_next (list);
	}
	build_table(curr_data);
}

static void
get_menu_pos (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
	charpick_data *curr_data = data;
	GtkRequisition  reqmenu;
	gint tempx, tempy, width, height;
	gint screen_width, screen_height;
	
	gtk_widget_size_request (GTK_WIDGET (menu), &reqmenu);
	gdk_window_get_origin (GDK_WINDOW (gtk_widget_get_window(curr_data->applet)), &tempx, &tempy);
     	gdk_window_get_geometry (GDK_WINDOW (gtk_widget_get_window(curr_data->applet)), NULL, NULL,
     				 &width, &height, NULL);
     			      
     	switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (curr_data->applet))) {
     	case MATE_PANEL_APPLET_ORIENT_DOWN:
        	tempy += height;
     		break;
     	case MATE_PANEL_APPLET_ORIENT_UP:
        	tempy -= reqmenu.height;
     		break;
     	case MATE_PANEL_APPLET_ORIENT_LEFT:
     		tempx -= reqmenu.width;
		break;
     	case MATE_PANEL_APPLET_ORIENT_RIGHT:
     		tempx += width;
		break;
     	}
	screen_width = gdk_screen_width ();
     	screen_height = gdk_screen_height ();
     	*x = CLAMP (tempx, 0, MAX (0, screen_width - reqmenu.width));
     	*y = CLAMP (tempy, 0, MAX (0, screen_height - reqmenu.height));
}

static void
chooser_button_clicked (GtkButton *button, charpick_data *curr_data)
{
	if (gtk_widget_get_visible (curr_data->menu))
		gtk_menu_popdown (GTK_MENU (curr_data->menu));
	else {
		gtk_menu_set_screen (GTK_MENU (curr_data->menu),
				gtk_widget_get_screen (GTK_WIDGET (curr_data->applet)));
		
		gtk_menu_popup (GTK_MENU (curr_data->menu), NULL, NULL, get_menu_pos, curr_data,
				0, gtk_get_current_event_time());
	}				    
}

/* Force the button not to have any focus padding and let the focus
   indication be drawn on the label itself when space is tight. Taken from the clock applet.
   FIXME : This is an Evil Hack and should be fixed when the focus padding can be overridden at the gtk+ level */

static inline void force_no_focus_padding (GtkWidget *widget)
{
  gboolean first_time=TRUE;

  if (first_time) {
    gtk_rc_parse_string ("\n"
			 "   style \"charpick-applet-button-style\"\n"
			 "   {\n"
			 "      GtkWidget::focus-line-width=0\n"
			 "      GtkWidget::focus-padding=0\n"
			 "   }\n"
			 "\n"
			 "    widget \"*.charpick-applet-button\" style \"charpick-applet-button-style\"\n"
			 "\n");
    first_time = FALSE;
  }

  gtk_widget_set_name (widget, "charpick-applet-button");
}

/* creates table of buttons, sets up their callbacks, and packs the table in
   the event box */

void
build_table(charpick_data *p_curr_data)
{
  GtkWidget *box, *button_box, **row_box;
  GtkWidget *button, *arrow;
  gint i = 0, len = g_utf8_strlen (p_curr_data->charlist, -1);
  GtkWidget **toggle_button;
  gchar *charlist;
  gint max_width=1, max_height=1;
  gint size_ratio;

  toggle_button = g_new (GtkWidget *, len);
  
  if (p_curr_data->box)
    gtk_widget_destroy(p_curr_data->box);
    
  if (p_curr_data->panel_vertical)
    box = gtk_vbox_new (FALSE, 0);
  else 
    box = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (box);
  p_curr_data->box = box;
  
  button = gtk_button_new ();
  if (g_list_length (p_curr_data->chartable) != 1)
  {
    gtk_widget_set_tooltip_text (button, _("Available palettes"));
  
    switch (mate_panel_applet_get_orient (MATE_PANEL_APPLET (p_curr_data->applet))) {
       	case MATE_PANEL_APPLET_ORIENT_DOWN:
          	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
       		break;
       	case MATE_PANEL_APPLET_ORIENT_UP:
          	arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);  
       		break;
       	case MATE_PANEL_APPLET_ORIENT_LEFT:
       		arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);  
  		break;
       	case MATE_PANEL_APPLET_ORIENT_RIGHT:
       		arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);  
  		break;
    default:
  	  g_assert_not_reached ();
    }
    gtk_container_add (GTK_CONTAINER (button), arrow);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    /* FIXME : evil hack (see force_no_focus_padding) */
    force_no_focus_padding (button);
    gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (button), "clicked",
                              G_CALLBACK (chooser_button_clicked),
			      p_curr_data);
    g_signal_connect (G_OBJECT (button), "button_press_event",
                               G_CALLBACK (button_press_hack),
			       p_curr_data->applet);
  
  }
  
  charlist = g_strdup (p_curr_data->charlist);
  for (i = 0; i < len; i++) {
    gchar label[7];
    GtkRequisition req;
    gchar *atk_desc;
    gchar *name;
    
    g_utf8_strncpy (label, charlist, 1);
    charlist = g_utf8_next_char (charlist);

#ifdef HAVE_GUCHARMAP
    /* TRANSLATOR: This sentance reads something like 'Insert "PILCROW SIGN"'
     *             hopefully, the name of the unicode character has already
     *             been translated.
     */
    name = g_strdup_printf (_("Insert \"%s\""),
		    gucharmap_get_unicode_name (g_utf8_get_char (label)));
#else
    name = g_strdup (_("Insert special character"));
#endif
   
    toggle_button[i] = gtk_toggle_button_new_with_label (label);
    atk_desc =  g_strdup_printf (_("insert special character %s"), label);
    set_atk_name_description (toggle_button[i], NULL, atk_desc);
    g_free (atk_desc);
    gtk_widget_show (toggle_button[i]);
    gtk_button_set_relief(GTK_BUTTON(toggle_button[i]), GTK_RELIEF_NONE);
    /* FIXME : evil hack (see force_no_focus_padding) */
    force_no_focus_padding (toggle_button[i]);
    gtk_widget_set_tooltip_text (toggle_button[i], name);
    g_free (name);
                      
    gtk_widget_size_request (toggle_button[i], &req);
    
    max_width = MAX (max_width, req.width);
    max_height = MAX (max_height, req.height-2);
  
    g_object_set_data (G_OBJECT (toggle_button[i]), "unichar", 
				GINT_TO_POINTER(g_utf8_get_char (label)));
    g_signal_connect (GTK_OBJECT (toggle_button[i]), "toggled",
		      G_CALLBACK (toggle_button_toggled_cb),
                        p_curr_data);
    g_signal_connect (GTK_OBJECT (toggle_button[i]), "button_press_event", 
                      G_CALLBACK (button_press_hack), p_curr_data->applet);
  }
  
  if (p_curr_data->panel_vertical) {
    size_ratio = p_curr_data->panel_size / max_width;
    button_box = gtk_hbox_new (TRUE, 0);
  } else {
    size_ratio = p_curr_data->panel_size / max_height;
    button_box = gtk_vbox_new (TRUE, 0);
  }

  gtk_box_pack_start (GTK_BOX (box), button_box, TRUE, TRUE, 0);
  
  size_ratio = MAX (size_ratio, 1);
  row_box = g_new0 (GtkWidget *, size_ratio);
  for (i=0; i < size_ratio; i++) {
  	if (!p_curr_data->panel_vertical) row_box[i] = gtk_hbox_new (TRUE, 0);
  	else row_box[i] = gtk_vbox_new (TRUE, 0);
  	gtk_box_pack_start (GTK_BOX (button_box), row_box[i], TRUE, TRUE, 0);
  }
  
  for (i = 0; i <len; i++) {  	
  	int delta = len/size_ratio;
  	int index;
  
	if (delta > 0)
	  	index = i / delta;
	else
		index = i;

	index = CLAMP (index, 0, size_ratio-1);	
  	gtk_box_pack_start (GTK_BOX (row_box[index]), toggle_button[i], TRUE, TRUE, 0);
  }
 
  g_free (toggle_button);
  
  gtk_container_add (GTK_CONTAINER(p_curr_data->applet), box);
  gtk_widget_show_all (p_curr_data->box);

  p_curr_data->last_index = NO_LAST_INDEX;
  p_curr_data->last_toggle_button = NULL;
  
}

static void applet_size_allocate(MatePanelApplet *applet, GtkAllocation *allocation, gpointer data)
{
  charpick_data *curr_data = data;
  if (curr_data->panel_vertical) {
    if (curr_data->panel_size == allocation->width)
      return;
    curr_data->panel_size = allocation->width;
  } else {
    if (curr_data->panel_size == allocation->height)
      return;
    curr_data->panel_size = allocation->height;
  }

  build_table (curr_data);
  return;
}

static void applet_change_orient(MatePanelApplet *applet, MatePanelAppletOrient o, gpointer data)
{
  charpick_data *curr_data = data;
  if (o == MATE_PANEL_APPLET_ORIENT_UP ||
      o == MATE_PANEL_APPLET_ORIENT_DOWN)
    curr_data->panel_vertical = FALSE;
  else
    curr_data->panel_vertical = TRUE;
  build_table (curr_data);
  return;
}


static void
about (GtkAction     *action,
       charpick_data *curr_data)
{
  static const char * const authors[] = {
	  "Alexandre Muñiz <munizao@xprt.net>",
	  "Kevin Vandersloot",
	  NULL
  };

  static const gchar * const documenters[] = {
          "Dan Mueth <d-mueth@uchicago.edu>",
          "Sun MATE Documentation Team <gdocteam@sun.com>",
	  NULL
  };

  gtk_show_about_dialog (NULL,
	"version",	VERSION,
	"copyright",	"\xC2\xA9 1998, 2004-2005 MATE Applets Maintainers "
			"and others",
	"comments",	_("Mate Panel applet for selecting strange "
			  "characters that are not on my keyboard. "
			  "Released under GNU General Public Licence."),
	"authors",	authors,
	"documenters",	documenters,
	"translator-credits",	_("translator-credits"),
	"logo-icon-name",	"accessories-character-map",
	NULL);
}


static void
help_cb (GtkAction     *action,
	 charpick_data *curr_data)
{
  GError *error = NULL;

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (curr_data->applet)),
                "ghelp:char-palette",
                gtk_get_current_event_time (),
                &error);

  if (error) { /* FIXME: the user needs to see this */
    g_warning ("help error: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
applet_destroy (GtkWidget *widget, gpointer data)
{
  charpick_data *curr_data = data;

  g_return_if_fail (curr_data);
   
  if (curr_data->about_dialog)
    gtk_widget_destroy (curr_data->about_dialog);   
  if (curr_data->propwindow)
    gtk_widget_destroy (curr_data->propwindow);
  if (curr_data->box)
    gtk_widget_destroy (curr_data->box);
  if (curr_data->menu)
    gtk_widget_destroy (curr_data->menu);
  g_free (curr_data);
  
}

void 
save_chartable (charpick_data *curr_data)
{
	MatePanelApplet *applet = MATE_PANEL_APPLET (curr_data->applet);
	MateConfValue *value;
	GList *list = curr_data->chartable;
	GSList *slist = NULL;
	
	while (list) {
		gchar *charlist = list->data;
		MateConfValue *v1;
		v1 = mateconf_value_new_from_string (MATECONF_VALUE_STRING, charlist, NULL);
		slist = g_slist_append (slist, v1);
		list = g_list_next (list);
	}
	
	value = mateconf_value_new (MATECONF_VALUE_LIST);
	mateconf_value_set_list_type (value, MATECONF_VALUE_STRING);
	mateconf_value_set_list_nocopy (value, slist);
	mate_panel_applet_mateconf_set_value (applet, "chartable", value, NULL);
	mateconf_value_free (value);
}

static void
get_chartable (charpick_data *curr_data)
{
	MatePanelApplet *applet = MATE_PANEL_APPLET (curr_data->applet);
	MateConfValue *value;
	gint i, n;
	
	value = mate_panel_applet_mateconf_get_value (applet, "chartable", NULL);
	if (value) {
		GSList *slist = mateconf_value_get_list (value);
		while (slist) {
			MateConfValue *v1 = slist->data;
			gchar *charlist;
			
			charlist = g_strdup (mateconf_value_get_string (v1));
			curr_data->chartable = g_list_append (curr_data->chartable, charlist);
			
			slist = g_slist_next (slist);
		}
		mateconf_value_free (value);
	}
	else {
		n = G_N_ELEMENTS (chartable);
		for (i=0; i<n; i++) {
			gchar *string;
		
			string = g_ucs4_to_utf8 (chartable[i], -1, NULL, NULL, NULL);
			curr_data->chartable = g_list_append (curr_data->chartable, string);
		
		}
		if ( ! key_writable (MATE_PANEL_APPLET (curr_data->applet), "chartable"))
			save_chartable (curr_data);
	}
	

}

static const GtkActionEntry charpick_applet_menu_actions [] = {
	{ "Preferences", GTK_STOCK_PROPERTIES, N_("_Preferences"),
	  NULL, NULL,
	  G_CALLBACK (show_preferences_dialog) },
	{ "Help", GTK_STOCK_HELP, N_("_Help"),
	  NULL, NULL,
	  G_CALLBACK (help_cb) },
	{ "About", GTK_STOCK_ABOUT, N_("_About"),
	  NULL, NULL,
	  G_CALLBACK (about) }
};

void
set_atk_name_description (GtkWidget *widget, const gchar *name,
                          const gchar *description)
{
  AtkObject *aobj;
  aobj = gtk_widget_get_accessible (widget);
  /* return if gail is not loaded */
  if (GTK_IS_ACCESSIBLE (aobj) == FALSE)
     return;
  if (name)
     atk_object_set_name (aobj, name);
  if (description)
     atk_object_set_description (aobj, description);
}

static void
make_applet_accessible (GtkWidget *applet)
{
  set_atk_name_description (applet, _("Character Palette"), _("Insert characters"));
}

static gboolean
charpicker_applet_fill (MatePanelApplet *applet)
{
  MatePanelAppletOrient orientation;
  charpick_data *curr_data;
  GdkAtom utf8_atom;
  GList *list;
  gchar *string;
  GtkActionGroup *action_group;
  gchar *ui_path;

  g_set_application_name (_("Character Palette"));
  
  gtk_window_set_default_icon_name ("accessories-character-map");

  mate_panel_applet_set_background_widget (applet, GTK_WIDGET (applet));

  mate_panel_applet_add_preferences (applet, "/schemas/apps/charpick/prefs", NULL);
  mate_panel_applet_set_flags (applet, MATE_PANEL_APPLET_EXPAND_MINOR);
   
  curr_data = g_new0 (charpick_data, 1);
  curr_data->last_index = NO_LAST_INDEX;
  curr_data->applet = GTK_WIDGET (applet);
  curr_data->about_dialog = NULL;
  curr_data->add_edit_dialog = NULL;
 
  get_chartable (curr_data);
  
  string  = mate_panel_applet_mateconf_get_string (applet, "current_list", NULL);
  if (string) {
  	list = curr_data->chartable;
  	while (list) {
  		if (g_ascii_strcasecmp (list->data, string) == 0)
  			curr_data->charlist = list->data;
  		list = g_list_next (list);
  	}
	/* FIXME: yeah leak, but this code is full of leaks and evil
	   point shuffling.  This should really be rewritten
	   -George */
	if (curr_data->charlist == NULL)
		curr_data->charlist = string;
	else
		g_free (string);
  } else {
  	curr_data->charlist = curr_data->chartable->data;  
  }
 
  curr_data->panel_size = mate_panel_applet_get_size (applet);
  
  orientation = mate_panel_applet_get_orient (applet);
  curr_data->panel_vertical = (orientation == MATE_PANEL_APPLET_ORIENT_LEFT) 
                              || (orientation == MATE_PANEL_APPLET_ORIENT_RIGHT);
  build_table (curr_data);
    
  g_signal_connect (G_OBJECT (curr_data->applet), "key_press_event",
		             G_CALLBACK (key_press_event), curr_data);

  utf8_atom = gdk_atom_intern ("UTF8_STRING", FALSE);
  gtk_selection_add_target (curr_data->applet, 
			    GDK_SELECTION_PRIMARY,
                            utf8_atom,
			    0);
  gtk_selection_add_target (curr_data->applet, 
			    GDK_SELECTION_CLIPBOARD,
                            utf8_atom,
			    0);
  g_signal_connect (GTK_OBJECT (curr_data->applet), "selection_get",
		      G_CALLBACK (charpick_selection_handler),
		      curr_data);
  g_signal_connect (GTK_OBJECT (curr_data->applet), "selection_clear_event",
		      G_CALLBACK (selection_clear_cb),
		      curr_data);
 
  make_applet_accessible (GTK_WIDGET (applet));

  /* session save signal */ 
  g_signal_connect (G_OBJECT (applet), "change_orient",
		    G_CALLBACK (applet_change_orient), curr_data);

  g_signal_connect (G_OBJECT (applet), "size_allocate",
		    G_CALLBACK (applet_size_allocate), curr_data);
		    
  g_signal_connect (G_OBJECT (applet), "destroy",
  		    G_CALLBACK (applet_destroy), curr_data);
  
  gtk_widget_show_all (GTK_WIDGET (applet));

  action_group = gtk_action_group_new ("Charpicker Applet Actions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group,
				charpick_applet_menu_actions,
				G_N_ELEMENTS (charpick_applet_menu_actions),
				curr_data);
  ui_path = g_build_filename (CHARPICK_MENU_UI_DIR, "charpick-applet-menu.xml", NULL);
  mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
                                     ui_path, action_group);
  g_free (ui_path);

  if (mate_panel_applet_get_locked_down (MATE_PANEL_APPLET (applet))) {
	  GtkAction *action;

	  action = gtk_action_group_get_action (action_group, "Preferences");
	  gtk_action_set_visible (action, FALSE);
  }
  g_object_unref (action_group);

  register_stock_for_edit ();			             
  populate_menu (curr_data);
  
  return TRUE;
}

static gboolean
charpicker_applet_factory (MatePanelApplet *applet,
			   const gchar          *iid,
			   gpointer              data)
{
	gboolean retval = FALSE;
    
	if (!strcmp (iid, "CharpickerApplet"))
		retval = charpicker_applet_fill (applet); 
    
	return retval;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY ("CharpickerAppletFactory",
				  PANEL_TYPE_APPLET,
				  "char-palette",
				  charpicker_applet_factory,
				  NULL)

