/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-selector.c: MateComponent Component Selector
 *
 * Authors:
 *   Richard Hestilow (hestgray@ionet.net)
 *   Miguel de Icaza  (miguel@kernel.org)
 *   Martin Baulig    (martin@home-of-linux.org)
 *   Anders Carlsson  (andersca@gnu.org)
 *   Havoc Pennington (hp@redhat.com)
 *   Dietmar Maurer   (dietmar@ximian.com)
 *   Michael Meeks    (michael@ximian.com)
 *
 * Copyright 1999, 2000 Richard Hestilow, Ximian, Inc,
 *                      Martin Baulig, Anders Carlsson,
 *                      Havoc Pennigton, Dietmar Maurer
 */
#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-selector.h>

G_DEFINE_TYPE (MateComponentSelector, matecomponent_selector, GTK_TYPE_DIALOG)

#define DEFAULT_INTERFACE "IDL:MateComponent/Control:1.0"
#define MATECOMPONENT_PAD_SMALL 4

struct _MateComponentSelectorPrivate {
	MateComponentSelectorWidget *selector;
};

enum {
	OK,
	CANCEL,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_INTERFACES
};

static guint matecomponent_selector_signals [LAST_SIGNAL] = { 0, 0 };

static void       
matecomponent_selector_finalize (GObject *object)
{
	g_return_if_fail (MATECOMPONENT_IS_SELECTOR (object));

	g_free (MATECOMPONENT_SELECTOR (object)->priv);

	G_OBJECT_CLASS (matecomponent_selector_parent_class)->finalize (object);
}

/**
 * matecomponent_selector_get_selected_id:
 * @sel: A MateComponentSelector widget.
 *
 * Returns: A newly-allocated string containing the ID of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this. It will give an oaf iid back.
 */
gchar *
matecomponent_selector_get_selected_id (MateComponentSelector *sel)
{
	g_return_val_if_fail (MATECOMPONENT_IS_SELECTOR (sel), NULL);

	return matecomponent_selector_widget_get_id (sel->priv->selector);
}

/**
 * matecomponent_selector_get_selected_name:
 * @sel: A MateComponentSelector widget.
 *
 * Returns: A newly-allocated string containing the name of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
matecomponent_selector_get_selected_name (MateComponentSelector *sel)
{
	g_return_val_if_fail (MATECOMPONENT_IS_SELECTOR (sel), NULL);

	return matecomponent_selector_widget_get_name (sel->priv->selector);
}

/**
 * matecomponent_selector_get_selected_description:
 * @sel: A MateComponentSelector widget.
 *
 * Returns: A newly-allocated string containing the description of the
 * currently-selected CORBA server (i.e., the corba server whose name
 * is highlighted in the list).  The user of this function is
 * responsible for freeing this.
 */
gchar *
matecomponent_selector_get_selected_description (MateComponentSelector *sel)
{
	g_return_val_if_fail (MATECOMPONENT_IS_SELECTOR (sel), NULL);

	return matecomponent_selector_widget_get_description (sel->priv->selector);
}

static void
ok_callback (GtkWidget *widget, gpointer data)
{
	char *text = matecomponent_selector_get_selected_id (
		MATECOMPONENT_SELECTOR (widget));

	g_object_set_data (G_OBJECT (widget), "UserData", text);
}

/**
 * matecomponent_selector_select_id:
 * @title: The title to be used for the dialog.
 * @interfaces_required: A list of required interfaces.  See
 * matecomponent_selector_new().
 *
 * Calls matecomponent_selector_new() to create a new
 * MateComponentSelector widget with the specified paramters, @title and
 * @interfaces_required.  Then runs the dialog modally and allows
 * the user to make a selection.
 *
 * Returns: The Oaf IID of the selected server, or NULL if no server is
 * selected.  The ID string has been allocated with g_strdup.
 */
gchar *
matecomponent_selector_select_id (const gchar  *title,
			   const gchar **interfaces_required)
{
	GtkWidget *sel = matecomponent_selector_new (title, interfaces_required);
	gchar     *name = NULL;
	int        n;

	g_return_val_if_fail (sel != NULL, NULL);

	g_signal_connect (sel, "ok",
			  G_CALLBACK (ok_callback), NULL);

	g_object_set_data (G_OBJECT (sel), "UserData", NULL);
	
	gtk_widget_show (sel);
		
	n = gtk_dialog_run (GTK_DIALOG (sel));

	switch (n) {
	case GTK_RESPONSE_CANCEL:
		name = NULL;
		break;
	case GTK_RESPONSE_APPLY:
	case GTK_RESPONSE_OK:
		name = g_object_get_data (G_OBJECT (sel), "UserData");
		break;
	default:
		break;
	}
		
	gtk_widget_destroy (sel);

	return name;
}

static void
response_callback (GtkWidget *widget,
		   gint       response_id,
		   gpointer   data) 
{
	switch (response_id) {
		case GTK_RESPONSE_OK:
			g_signal_emit (data, matecomponent_selector_signals [OK], 0);
			break;
		case GTK_RESPONSE_CANCEL:
			g_signal_emit (data, matecomponent_selector_signals [CANCEL], 0);
		default:
			break;
	}
}

static void
final_select_cb (GtkWidget *widget, MateComponentSelector *sel)
{
	gtk_dialog_response (GTK_DIALOG (sel), GTK_RESPONSE_OK);
}

/**
 * matecomponent_selector_construct:
 * @sel: the selector to construct
 * @title: the title for the window
 * @selector: the component view widget to put inside it.
 *
 * Don't use this ever - use construct-time properties instead.
 * TODO: Remove from header when we are allowed to change the API.
 * Constructs the innards of a matecomponent selector window.
 *
 * Return value: the constructed widget.
 **/
GtkWidget *
matecomponent_selector_construct       (MateComponentSelector       *sel,
					    const gchar          *title,
					    MateComponentSelectorWidget *selector)
{	
 	g_return_val_if_fail (MATECOMPONENT_IS_SELECTOR (sel), NULL);
 	g_return_val_if_fail (MATECOMPONENT_IS_SELECTOR_WIDGET (selector), NULL);
 
	sel->priv->selector = selector;

 	g_signal_connect (selector, "final_select",
 			  G_CALLBACK (final_select_cb), sel);
	
	gtk_window_set_title (GTK_WINDOW (sel), title ? title : "");
 
 	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (sel)->vbox),
 			    GTK_WIDGET (selector),
			    TRUE, TRUE, MATECOMPONENT_PAD_SMALL);

	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response (GTK_DIALOG (sel), GTK_RESPONSE_OK);

	g_signal_connect (sel, "response",
			  G_CALLBACK (response_callback), sel);

	gtk_window_set_default_size (GTK_WINDOW (sel), 400, 300);
	gtk_widget_show_all (GTK_DIALOG (sel)->vbox);
	return GTK_WIDGET (sel);
}

static void
matecomponent_selector_internal_construct (MateComponentSelector       *sel)
{
	MateComponentSelectorWidget* selector = sel->priv->selector;

	g_signal_connect (selector, "final_select",
			  G_CALLBACK (final_select_cb), sel);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (sel)->vbox),
			    GTK_WIDGET (selector),
			    TRUE, TRUE, MATECOMPONENT_PAD_SMALL);
	
	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_OK,
			       GTK_RESPONSE_OK);
	gtk_dialog_add_button (GTK_DIALOG (sel), GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response (GTK_DIALOG (sel), GTK_RESPONSE_OK);
	
	g_signal_connect (sel, "response",
			  G_CALLBACK (response_callback), sel);
	
	gtk_window_set_default_size (GTK_WINDOW (sel), 400, 300);
	gtk_widget_show_all  (GTK_DIALOG (sel)->vbox);
}

static void
matecomponent_selector_init (MateComponentSelector *sel)
{
	MateComponentSelectorWidget *selectorwidget = NULL;
	
	sel->priv = g_new0 (MateComponentSelectorPrivate, 1);
	
	selectorwidget = MATECOMPONENT_SELECTOR_WIDGET (matecomponent_selector_widget_new ());
	sel->priv->selector = selectorwidget;
	
	matecomponent_selector_internal_construct (sel);
}

static void
matecomponent_selector_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	MateComponentSelector *selector = MATECOMPONENT_SELECTOR (object);

	switch (prop_id) {
	case PROP_INTERFACES:
	{
		const gchar *query [2] = { DEFAULT_INTERFACE, NULL }; /* the default interfaces_required. */
		MateComponentSelectorWidget *selectorwidget = NULL;
		
		/* Get the supplied array or interfaces, replacing it with a default if none have been provided: */
		const gchar **interfaces_required = NULL;
		if (!interfaces_required)
			interfaces_required = query;
		
		/* Set the interfaces_required in the child SelectorWidget: */
		selectorwidget = selector->priv->selector;;
		if (selectorwidget)
			matecomponent_selector_widget_set_interfaces (selectorwidget, interfaces_required);
			
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
matecomponent_selector_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	/* PROP_INTERFACES is read-only, because there is a virtual MateComponentSelectorWidget::set_interfaces(), but no get_interfaces(). */
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
matecomponent_selector_class_init (MateComponentSelectorClass *klass)
{
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	object_class->finalize = matecomponent_selector_finalize;
	
	object_class->set_property = matecomponent_selector_set_property;
	object_class->get_property = matecomponent_selector_get_property;

	matecomponent_selector_signals [OK] =
		g_signal_new ("ok",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentSelectorClass, ok),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	
	matecomponent_selector_signals [CANCEL] =
		g_signal_new ("cancel",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentSelectorClass, cancel),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
		
	/* properties: */	
   	g_object_class_install_property (object_class,
				 PROP_INTERFACES,
				 g_param_spec_value_array ("interfaces_required",
							   _("Interfaces required"),
							   _("A NULL-terminated array of interfaces which a server must support in order to be listed in the selector. Defaults to \"IDL:MateComponent/Embeddable:1.0\" if no interfaces are listed"),
							   g_param_spec_string ("interface-required-entry",
										_("Interface required entry"),
										_("One of the interfaces that's required"),
										NULL,
										G_PARAM_READWRITE),
							   G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

/**
 * matecomponent_selector_new:
 * @title: A string which should go in the title of the
 * MateComponentSelector window.
 * @interfaces_required: A NULL-terminated array of interfaces which a
 * server must support in order to be listed in the selector.  Defaults
 * to "IDL:MateComponent/Embeddable:1.0" if no interfaces are listed.
 *
 * Creates a new MateComponentSelector widget.  The title of the dialog
 * is set to @title, and the list of selectable servers is populated
 * with those servers which support the interfaces specified in
 * @interfaces_required.
 *
 * Returns: A pointer to the newly-created MateComponentSelector widget.
 */
GtkWidget *
matecomponent_selector_new (const gchar *title,
		     const gchar **interfaces_required)
{
	MateComponentSelector *sel =  g_object_new (matecomponent_selector_get_type (), "title", title, "interfaces_required", interfaces_required, NULL);

	return GTK_WIDGET (sel);
}
