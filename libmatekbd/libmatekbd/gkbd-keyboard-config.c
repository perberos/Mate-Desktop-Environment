/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <X11/keysym.h>

#include <glib/gi18n.h>

#include <gkbd-keyboard-config.h>
#include <gkbd-config-private.h>

/**
 * GkbdKeyboardConfig
 */
#define GKBD_KEYBOARD_CONFIG_KEY_PREFIX GKBD_CONFIG_KEY_PREFIX "/kbd"

#define GROUP_SWITCHERS_GROUP "grp"
#define DEFAULT_GROUP_SWITCH "grp:shift_caps_toggle"

const gchar GKBD_KEYBOARD_CONFIG_DIR[] = GKBD_KEYBOARD_CONFIG_KEY_PREFIX;
const gchar GKBD_KEYBOARD_CONFIG_KEY_MODEL[] =
    GKBD_KEYBOARD_CONFIG_KEY_PREFIX "/model";
const gchar GKBD_KEYBOARD_CONFIG_KEY_LAYOUTS[] =
    GKBD_KEYBOARD_CONFIG_KEY_PREFIX "/layouts";
const gchar GKBD_KEYBOARD_CONFIG_KEY_OPTIONS[] =
    GKBD_KEYBOARD_CONFIG_KEY_PREFIX "/options";

const gchar *GKBD_KEYBOARD_CONFIG_ACTIVE[] = {
	GKBD_KEYBOARD_CONFIG_KEY_MODEL,
	GKBD_KEYBOARD_CONFIG_KEY_LAYOUTS,
	GKBD_KEYBOARD_CONFIG_KEY_OPTIONS
};

/**
 * static common functions
 */
static void
gkbd_keyboard_config_string_list_reset (GSList ** plist)
{
	while (*plist != NULL) {
		GSList *p = *plist;
		*plist = (*plist)->next;
		g_free (p->data);
		g_slist_free_1 (p);
	}
}

static gboolean
gslist_str_equal (GSList * l1, GSList * l2)
{
	if (l1 == l2)
		return TRUE;
	while (l1 != NULL && l2 != NULL) {
		if ((l1->data != l2->data) &&
		    (l1->data != NULL) &&
		    (l2->data != NULL) &&
		    g_ascii_strcasecmp (l1->data, l2->data))
			return False;

		l1 = l1->next;
		l2 = l2->next;
	}
	return (l1 == NULL && l2 == NULL);
}

gboolean
gkbd_keyboard_config_get_lv_descriptions (XklConfigRegistry *
					  config_registry,
					  const gchar * layout_name,
					  const gchar * variant_name,
					  gchar ** layout_short_descr,
					  gchar ** layout_descr,
					  gchar ** variant_short_descr,
					  gchar ** variant_descr)
{
	/* TODO make it not static */
	static XklConfigItem *litem = NULL;
	static XklConfigItem *vitem = NULL;

	if (litem == NULL)
		litem = xkl_config_item_new ();
	if (vitem == NULL)
		vitem = xkl_config_item_new ();

	layout_name = g_strdup (layout_name);

	g_snprintf (litem->name, sizeof litem->name, "%s", layout_name);
	if (xkl_config_registry_find_layout (config_registry, litem)) {
		*layout_short_descr = litem->short_description;
		*layout_descr = litem->description;
	} else
		*layout_short_descr = *layout_descr = NULL;

	if (variant_name != NULL) {
		variant_name = g_strdup (variant_name);
		g_snprintf (vitem->name, sizeof vitem->name, "%s",
			    variant_name);
		if (xkl_config_registry_find_variant
		    (config_registry, layout_name, vitem)) {
			*variant_short_descr = vitem->short_description;
			*variant_descr = vitem->description;
		} else
			*variant_short_descr = *variant_descr = NULL;

		g_free ((char *) variant_name);
	} else
		*variant_descr = NULL;

	g_free ((char *) layout_name);
	return *layout_descr != NULL;
}

/**
 * extern common functions
 */
const gchar *
gkbd_keyboard_config_merge_items (const gchar * parent,
				  const gchar * child)
{
	static gchar buffer[XKL_MAX_CI_NAME_LENGTH * 2 - 1];
	*buffer = '\0';
	if (parent != NULL) {
		if (strlen (parent) >= XKL_MAX_CI_NAME_LENGTH)
			return NULL;
		strcat (buffer, parent);
	}
	if (child != NULL && *child != 0) {
		if (strlen (child) >= XKL_MAX_CI_NAME_LENGTH)
			return NULL;
		strcat (buffer, "\t");
		strcat (buffer, child);
	}
	return buffer;
}

gboolean
gkbd_keyboard_config_split_items (const gchar * merged, gchar ** parent,
				  gchar ** child)
{
	static gchar pbuffer[XKL_MAX_CI_NAME_LENGTH];
	static gchar cbuffer[XKL_MAX_CI_NAME_LENGTH];
	int plen, clen;
	const gchar *pos;
	*parent = *child = NULL;

	if (merged == NULL)
		return FALSE;

	pos = strchr (merged, '\t');
	if (pos == NULL) {
		plen = strlen (merged);
		clen = 0;
	} else {
		plen = pos - merged;
		clen = strlen (pos + 1);
		if (clen >= XKL_MAX_CI_NAME_LENGTH)
			return FALSE;
		strcpy (*child = cbuffer, pos + 1);
	}
	if (plen >= XKL_MAX_CI_NAME_LENGTH)
		return FALSE;
	memcpy (*parent = pbuffer, merged, plen);
	pbuffer[plen] = '\0';
	return TRUE;
}

/**
 * static GkbdKeyboardConfig functions
 */
static void
gkbd_keyboard_config_options_add_full (GkbdKeyboardConfig * kbd_config,
				       const gchar * full_option_name)
{
	kbd_config->options =
	    g_slist_append (kbd_config->options,
			    g_strdup (full_option_name));
}

static void
gkbd_keyboard_config_layouts_add_full (GkbdKeyboardConfig * kbd_config,
				       const gchar * full_layout_name)
{
	kbd_config->layouts_variants =
	    g_slist_append (kbd_config->layouts_variants,
			    g_strdup (full_layout_name));
}

static void
gkbd_keyboard_config_copy_from_xkl_config (GkbdKeyboardConfig * kbd_config,
					   XklConfigRec * pdata)
{
	char **p, **p1;
	gkbd_keyboard_config_model_set (kbd_config, pdata->model);
	xkl_debug (150, "Loaded Kbd model: [%s]\n", pdata->model);

	gkbd_keyboard_config_layouts_reset (kbd_config);
	p = pdata->layouts;
	p1 = pdata->variants;
	while (p != NULL && *p != NULL) {
		const gchar *full_layout =
		    gkbd_keyboard_config_merge_items (*p, *p1);
		xkl_debug (150,
			   "Loaded Kbd layout (with variant): [%s]\n",
			   full_layout);
		gkbd_keyboard_config_layouts_add_full (kbd_config,
						       full_layout);
		p++;
		p1++;
	}

	gkbd_keyboard_config_options_reset (kbd_config);
	p = pdata->options;
	while (p != NULL && *p != NULL) {
		char group[XKL_MAX_CI_NAME_LENGTH];
		char *option = *p;
		char *delim =
		    (option != NULL) ? strchr (option, ':') : NULL;
		int len;
		if ((delim != NULL) &&
		    ((len = (delim - option)) < XKL_MAX_CI_NAME_LENGTH)) {
			strncpy (group, option, len);
			group[len] = 0;
			xkl_debug (150, "Loaded Kbd option: [%s][%s]\n",
				   group, option);
			gkbd_keyboard_config_options_add (kbd_config,
							  group, option);
		}
		p++;
	}
}

static void
gkbd_keyboard_config_copy_to_xkl_config (GkbdKeyboardConfig * kbd_config,
					 XklConfigRec * pdata)
{
	int i;
	int num_layouts, num_options;
	pdata->model =
	    (kbd_config->model ==
	     NULL) ? NULL : g_strdup (kbd_config->model);

	num_layouts =
	    (kbd_config->layouts_variants ==
	     NULL) ? 0 : g_slist_length (kbd_config->layouts_variants);
	num_options =
	    (kbd_config->options ==
	     NULL) ? 0 : g_slist_length (kbd_config->options);

	xkl_debug (150, "Taking %d layouts\n", num_layouts);
	if (num_layouts != 0) {
		GSList *the_layout_variant = kbd_config->layouts_variants;
		char **p1 = pdata->layouts =
		    g_new0 (char *, num_layouts + 1);
		char **p2 = pdata->variants =
		    g_new0 (char *, num_layouts + 1);
		for (i = num_layouts; --i >= 0;) {
			char *layout, *variant;
			if (gkbd_keyboard_config_split_items
			    (the_layout_variant->data, &layout, &variant)
			    && variant != NULL) {
				*p1 =
				    (layout ==
				     NULL) ? g_strdup ("") :
				    g_strdup (layout);
				*p2 =
				    (variant ==
				     NULL) ? g_strdup ("") :
				    g_strdup (variant);
			} else {
				*p1 =
				    (the_layout_variant->data ==
				     NULL) ? g_strdup ("") :
				    g_strdup (the_layout_variant->data);
				*p2 = g_strdup ("");
			}
			xkl_debug (150, "Adding [%s]/%p and [%s]/%p\n",
				   *p1 ? *p1 : "(nil)", *p1,
				   *p2 ? *p2 : "(nil)", *p2);
			p1++;
			p2++;
			the_layout_variant = the_layout_variant->next;
		}
	}

	if (num_options != 0) {
		GSList *the_option = kbd_config->options;
		char **p = pdata->options =
		    g_new0 (char *, num_options + 1);
		for (i = num_options; --i >= 0;) {
			char *group, *option;
			if (gkbd_keyboard_config_split_items
			    (the_option->data, &group, &option)
			    && option != NULL)
				*(p++) = g_strdup (option);
			else {
				*(p++) = g_strdup ("");
				xkl_debug (150, "Could not split [%s]\n",
					   the_option->data);
			}
			the_option = the_option->next;
		}
	}
}

static void
gkbd_keyboard_config_load_params (GkbdKeyboardConfig * kbd_config,
				  const gchar * param_names[])
{
	GError *gerror = NULL;
	gchar *pc;
	GSList *pl, *l;

	pc = mateconf_client_get_string (kbd_config->conf_client,
				      param_names[0], &gerror);
	if (pc == NULL || gerror != NULL) {
		if (gerror != NULL) {
			g_warning ("Error reading configuration:%s\n",
				   gerror->message);
			g_error_free (gerror);
			g_free (pc);
			gerror = NULL;
		}
		gkbd_keyboard_config_model_set (kbd_config, NULL);
	} else {
		gkbd_keyboard_config_model_set (kbd_config, pc);
		g_free (pc);
	}
	xkl_debug (150, "Loaded Kbd model: [%s]\n",
		   kbd_config->model ? kbd_config->model : "(null)");

	gkbd_keyboard_config_layouts_reset (kbd_config);

	l = pl = mateconf_client_get_list (kbd_config->conf_client,
					param_names[1],
					MATECONF_VALUE_STRING, &gerror);
	if (pl == NULL || gerror != NULL) {
		if (gerror != NULL) {
			g_warning ("Error reading configuration:%s\n",
				   gerror->message);
			g_error_free (gerror);
			gerror = NULL;
		}
	}

	while (l != NULL) {
		xkl_debug (150, "Loaded Kbd layout: [%s]\n", l->data);
		gkbd_keyboard_config_layouts_add_full (kbd_config,
						       l->data);
		l = l->next;
	}
	gkbd_keyboard_config_string_list_reset (&pl);

	gkbd_keyboard_config_options_reset (kbd_config);

	l = pl = mateconf_client_get_list (kbd_config->conf_client,
					param_names[2],
					MATECONF_VALUE_STRING, &gerror);
	if (pl == NULL || gerror != NULL) {
		if (gerror != NULL) {
			g_warning ("Error reading configuration:%s\n",
				   gerror->message);
			g_error_free (gerror);
			gerror = NULL;
		}
	}

	while (l != NULL) {
		xkl_debug (150, "Loaded Kbd option: [%s]\n", l->data);
		gkbd_keyboard_config_options_add_full (kbd_config,
						       (const gchar *)
						       l->data);
		l = l->next;
	}
	gkbd_keyboard_config_string_list_reset (&pl);
}

static void
gkbd_keyboard_config_save_params (GkbdKeyboardConfig * kbd_config,
				  MateConfChangeSet * cs,
				  const gchar * param_names[])
{
	GSList *pl;

	if (kbd_config->model)
		mateconf_change_set_set_string (cs, param_names[0],
					     kbd_config->model);
	else
		mateconf_change_set_unset (cs, param_names[0]);
	xkl_debug (150, "Saved Kbd model: [%s]\n",
		   kbd_config->model ? kbd_config->model : "(null)");

	if (kbd_config->layouts_variants) {
		pl = kbd_config->layouts_variants;
		while (pl != NULL) {
			xkl_debug (150, "Saved Kbd layout: [%s]\n",
				   pl->data);
			pl = pl->next;
		}
		mateconf_change_set_set_list (cs,
					   param_names[1],
					   MATECONF_VALUE_STRING,
					   kbd_config->layouts_variants);
	} else {
		xkl_debug (150, "Saved Kbd layouts: []\n");
		mateconf_change_set_unset (cs, param_names[1]);
	}

	if (kbd_config->options) {
		pl = kbd_config->options;
		while (pl != NULL) {
			xkl_debug (150, "Saved Kbd option: [%s]\n",
				   pl->data);
			pl = pl->next;
		}
		mateconf_change_set_set_list (cs,
					   param_names[2],
					   MATECONF_VALUE_STRING,
					   kbd_config->options);
	} else {
		xkl_debug (150, "Saved Kbd options: []\n");
		mateconf_change_set_unset (cs, param_names[2]);
	}
}

/**
 * extern GkbdKeyboardConfig config functions
 */
void
gkbd_keyboard_config_init (GkbdKeyboardConfig * kbd_config,
			   MateConfClient * conf_client, XklEngine * engine)
{
	GError *gerror = NULL;

	memset (kbd_config, 0, sizeof (*kbd_config));
	kbd_config->conf_client = conf_client;
	kbd_config->engine = engine;
	g_object_ref (kbd_config->conf_client);

	mateconf_client_add_dir (kbd_config->conf_client,
			      GKBD_KEYBOARD_CONFIG_DIR,
			      MATECONF_CLIENT_PRELOAD_NONE, &gerror);
	if (gerror != NULL) {
		g_warning ("err: %s\n", gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
}

void
gkbd_keyboard_config_term (GkbdKeyboardConfig * kbd_config)
{
	gkbd_keyboard_config_model_set (kbd_config, NULL);

	gkbd_keyboard_config_layouts_reset (kbd_config);
	gkbd_keyboard_config_options_reset (kbd_config);

	g_object_unref (kbd_config->conf_client);
	kbd_config->conf_client = NULL;
}

void
gkbd_keyboard_config_load_from_mateconf (GkbdKeyboardConfig * kbd_config,
				      GkbdKeyboardConfig *
				      kbd_config_default)
{
	gkbd_keyboard_config_load_params (kbd_config,
					  GKBD_KEYBOARD_CONFIG_ACTIVE);

	if (kbd_config_default != NULL) {
		GSList *pl;

		if (kbd_config->model == NULL)
			kbd_config->model =
			    g_strdup (kbd_config_default->model);

		if (kbd_config->layouts_variants == NULL) {
			pl = kbd_config_default->layouts_variants;
			while (pl != NULL) {
				kbd_config->layouts_variants =
				    g_slist_append
				    (kbd_config->layouts_variants,
				     g_strdup (pl->data));
				pl = pl->next;
			}
		}

		if (kbd_config->options == NULL) {
			pl = kbd_config_default->options;
			while (pl != NULL) {
				kbd_config->options =
				    g_slist_append (kbd_config->options,
						    g_strdup (pl->data));
				pl = pl->next;
			}
		}
	}
}

void
gkbd_keyboard_config_load_from_x_current (GkbdKeyboardConfig * kbd_config,
					  XklConfigRec * data)
{
	gboolean own_data = data == NULL;
	xkl_debug (150, "Copying config from X(current)\n");
	if (own_data)
		data = xkl_config_rec_new ();
	if (xkl_config_rec_get_from_server (data, kbd_config->engine))
		gkbd_keyboard_config_copy_from_xkl_config (kbd_config,
							   data);
	else
		xkl_debug (150,
			   "Could not load keyboard config from server: [%s]\n",
			   xkl_get_last_error ());
	if (own_data)
		g_object_unref (G_OBJECT (data));
}

void
gkbd_keyboard_config_load_from_x_initial (GkbdKeyboardConfig * kbd_config,
					  XklConfigRec * data)
{
	gboolean own_data = data == NULL;
	xkl_debug (150, "Copying config from X(initial)\n");
	if (own_data)
		data = xkl_config_rec_new ();
	if (xkl_config_rec_get_from_backup (data, kbd_config->engine))
		gkbd_keyboard_config_copy_from_xkl_config (kbd_config,
							   data);
	else
		xkl_debug (150,
			   "Could not load keyboard config from backup: [%s]\n",
			   xkl_get_last_error ());
	if (own_data)
		g_object_unref (G_OBJECT (data));
}

gboolean
gkbd_keyboard_config_equals (GkbdKeyboardConfig * kbd_config1,
			     GkbdKeyboardConfig * kbd_config2)
{
	if (kbd_config1 == kbd_config2)
		return True;
	if ((kbd_config1->model != kbd_config2->model) &&
	    (kbd_config1->model != NULL) &&
	    (kbd_config2->model != NULL) &&
	    g_ascii_strcasecmp (kbd_config1->model, kbd_config2->model))
		return False;
	return gslist_str_equal (kbd_config1->layouts_variants,
				 kbd_config2->layouts_variants)
	    && gslist_str_equal (kbd_config1->options,
				 kbd_config2->options);
}

void
gkbd_keyboard_config_save_to_mateconf (GkbdKeyboardConfig * kbd_config)
{
	MateConfChangeSet *cs;
	GError *gerror = NULL;

	cs = mateconf_change_set_new ();

	gkbd_keyboard_config_save_params (kbd_config, cs,
					  GKBD_KEYBOARD_CONFIG_ACTIVE);

	mateconf_client_commit_change_set (kbd_config->conf_client, cs, TRUE,
					&gerror);
	if (gerror != NULL) {
		g_warning ("Error saving active configuration: %s\n",
			   gerror->message);
		g_error_free (gerror);
		gerror = NULL;
	}
	mateconf_change_set_unref (cs);
}

void
gkbd_keyboard_config_model_set (GkbdKeyboardConfig * kbd_config,
				const gchar * model_name)
{
	if (kbd_config->model != NULL)
		g_free (kbd_config->model);
	kbd_config->model =
	    (model_name == NULL
	     || model_name[0] == '\0') ? NULL : g_strdup (model_name);
}

void
gkbd_keyboard_config_layouts_add (GkbdKeyboardConfig * kbd_config,
				  const gchar * layout_name,
				  const gchar * variant_name)
{
	const gchar *merged;
	if (layout_name == NULL)
		return;
	merged =
	    gkbd_keyboard_config_merge_items (layout_name, variant_name);
	if (merged == NULL)
		return;
	gkbd_keyboard_config_layouts_add_full (kbd_config, merged);
}

void
gkbd_keyboard_config_layouts_reset (GkbdKeyboardConfig * kbd_config)
{
	gkbd_keyboard_config_string_list_reset
	    (&kbd_config->layouts_variants);
}

void
gkbd_keyboard_config_options_reset (GkbdKeyboardConfig * kbd_config)
{
	gkbd_keyboard_config_string_list_reset (&kbd_config->options);
}

void
gkbd_keyboard_config_options_add (GkbdKeyboardConfig * kbd_config,
				  const gchar * group_name,
				  const gchar * option_name)
{
	const gchar *merged;
	if (group_name == NULL || option_name == NULL)
		return;
	merged =
	    gkbd_keyboard_config_merge_items (group_name, option_name);
	if (merged == NULL)
		return;
	gkbd_keyboard_config_options_add_full (kbd_config, merged);
}

gboolean
gkbd_keyboard_config_options_is_set (GkbdKeyboardConfig * kbd_config,
				     const gchar * group_name,
				     const gchar * option_name)
{
	const gchar *merged =
	    gkbd_keyboard_config_merge_items (group_name, option_name);
	if (merged == NULL)
		return FALSE;

	return NULL != g_slist_find_custom (kbd_config->options, (gpointer)
					    merged, (GCompareFunc)
					    g_ascii_strcasecmp);
}

gboolean
gkbd_keyboard_config_activate (GkbdKeyboardConfig * kbd_config)
{
	gboolean rv;
	XklConfigRec *data = xkl_config_rec_new ();

	gkbd_keyboard_config_copy_to_xkl_config (kbd_config, data);
	rv = xkl_config_rec_activate (data, kbd_config->engine);
	g_object_unref (G_OBJECT (data));

	return rv;
}

void
gkbd_keyboard_config_start_listen (GkbdKeyboardConfig * kbd_config,
				   MateConfClientNotifyFunc func,
				   gpointer user_data)
{
	gkbd_desktop_config_add_listener (kbd_config->conf_client,
					  GKBD_KEYBOARD_CONFIG_DIR, func,
					  user_data,
					  &kbd_config->config_listener_id);
}

void
gkbd_keyboard_config_stop_listen (GkbdKeyboardConfig * kbd_config)
{
	gkbd_desktop_config_remove_listener (kbd_config->conf_client,
					     &kbd_config->
					     config_listener_id);
}

gboolean
gkbd_keyboard_config_get_descriptions (XklConfigRegistry * config_registry,
				       const gchar * name,
				       gchar ** layout_short_descr,
				       gchar ** layout_descr,
				       gchar ** variant_short_descr,
				       gchar ** variant_descr)
{
	char *layout_name = NULL, *variant_name = NULL;
	if (!gkbd_keyboard_config_split_items
	    (name, &layout_name, &variant_name))
		return FALSE;
	return gkbd_keyboard_config_get_lv_descriptions (config_registry,
							 layout_name,
							 variant_name,
							 layout_short_descr,
							 layout_descr,
							 variant_short_descr,
							 variant_descr);
}

const gchar *
gkbd_keyboard_config_format_full_layout (const gchar * layout_descr,
					 const gchar * variant_descr)
{
	static gchar full_descr[XKL_MAX_CI_DESC_LENGTH * 2];
	if (variant_descr == NULL || variant_descr[0] == 0)
		g_snprintf (full_descr, sizeof (full_descr), "%s",
			    layout_descr);
	else
		g_snprintf (full_descr, sizeof (full_descr), "%s %s",
			    layout_descr, variant_descr);
	return full_descr;
}

gchar *
gkbd_keyboard_config_to_string (const GkbdKeyboardConfig * config)
{
	gchar *layouts = NULL, *options = NULL;
	GString *buffer = g_string_new (NULL);

	GSList *iter;
	gint count;
	gchar *result;

	if (config->layouts_variants) {
		/* g_slist_length is "expensive", so we determinate the length on the fly */
		for (iter = config->layouts_variants, count = 0; iter;
		     iter = iter->next, ++count) {
			if (buffer->len)
				g_string_append (buffer, " ");

			g_string_append (buffer,
					 (const gchar *) iter->data);
		}

		/* Translators: The count is related to the number of options. The %s
		 * format specifier should not be modified, left "as is". */
		layouts =
		    g_strdup_printf (ngettext
				     ("layout \"%s\"", "layouts \"%s\"",
				      count), buffer->str);
		g_string_truncate (buffer, 0);
	}
	if (config->options) {
		/* g_slist_length is "expensive", so we determinate the length on the fly */
		for (iter = config->options, count = 0; iter;
		     iter = iter->next, ++count) {
			if (buffer->len)
				g_string_append (buffer, " ");

			g_string_append (buffer,
					 (const gchar *) iter->data);
		}

		/* Translators: The count is related to the number of options. The %s
		 * format specifier should not be modified, left "as is". */
		options =
		    g_strdup_printf (ngettext
				     ("option \"%s\"", "options \"%s\"",
				      count), buffer->str);
		g_string_truncate (buffer, 0);
	}

	g_string_free (buffer, TRUE);

	result =
	    g_strdup_printf (_("model \"%s\", %s and %s"), config->model,
			     layouts ? layouts : _("no layout"),
			     options ? options : _("no options"));

	g_free (options);
	g_free (layouts);

	return result;
}

GSList *
gkbd_keyboard_config_add_default_switch_option_if_necessary (GSList *
							     layouts_list,
							     GSList *
							     options_list, gboolean *was_appended)
{
	*was_appended = FALSE;
	if (g_slist_length (layouts_list) >= 2) {
		gboolean any_switcher = False;
		GSList *option = options_list;
		while (option != NULL) {
			char *g, *o;
			if (gkbd_keyboard_config_split_items
			    (option->data, &g, &o)) {
				if (!g_ascii_strcasecmp
				    (g, GROUP_SWITCHERS_GROUP)) {
					any_switcher = True;
					break;
				}
			}
			option = option->next;
		}
		if (!any_switcher) {
			const gchar *id =
			    gkbd_keyboard_config_merge_items
			    (GROUP_SWITCHERS_GROUP,
			     DEFAULT_GROUP_SWITCH);
			options_list =
			    g_slist_append (options_list, g_strdup (id));
			*was_appended = TRUE;
		}
	}
	return options_list;
}
