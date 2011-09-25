/* mate-about-me.c
 * Copyright (C) 2002 Diego Gonzalez
 *
 * Written by: Diego Gonzalez <diego@pemas.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gstdio.h>
#include <gio/gio.h>
#include <mateconf/mateconf-client.h>
#include <unistd.h>
#include <libebook/e-book.h>
#include <dbus/dbus-glib-bindings.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmateui/mate-desktop-thumbnail.h>

#include "e-image-chooser.h"
#include "mate-about-me-password.h"
#include "mate-about-me-fingerprint.h"
#include "marshal.h"

#include "capplet-util.h"

#define MAX_HEIGHT 150
#define MAX_WIDTH  150

#define EMAIL_SLOTS 4

typedef struct {
	EContact 	*contact;
	EBook    	*book;

	GtkBuilder 	*dialog;
	GtkWidget	*enable_fingerprint_button;
	GtkWidget	*disable_fingerprint_button;
	GtkWidget   *image_chooser;

	GdkScreen    	*screen;
	GtkIconTheme 	*theme;
	MateDesktopThumbnailFactory *thumbs;

	EContactAddress *addr1;
	EContactAddress *addr2;
	gchar           *email[EMAIL_SLOTS];
	const gchar     *email_types[EMAIL_SLOTS];

	gboolean      	 have_image;
	gboolean      	 image_changed;
	gboolean      	 create_self;

	gchar        	*person;
	gchar 		*login;
	gchar 		*username;

	guint	      	 commit_timeout_id;
} MateAboutMe;

static MateAboutMe *me = NULL;

struct WidToCid {
	gchar *wid;
	guint  cid;
};

enum {
	ADDRESS_STREET = 1,
	ADDRESS_POBOX,
	ADDRESS_LOCALITY,
	ADDRESS_CODE,
	ADDRESS_REGION,
	ADDRESS_COUNTRY
};

#define EMAIL_WORK              0
#define EMAIL_HOME              1
#define ADDRESS_HOME		21
#define ADDRESS_WORK		27

struct WidToCid ids[] = {

	{ "email-work-e",      0                             }, /* 00 */
	{ "email-home-e",      1                             }, /* 01 */

	{ "phone-home-e",      E_CONTACT_PHONE_HOME          }, /* 02 */
	{ "phone-mobile-e",    E_CONTACT_PHONE_MOBILE        }, /* 03 */
	{ "phone-work-e",      E_CONTACT_PHONE_BUSINESS      }, /* 04 */
	{ "phone-work-fax-e",  E_CONTACT_PHONE_BUSINESS_FAX  }, /* 05 */

	{ "im-jabber-e",       E_CONTACT_IM_JABBER_HOME_1    }, /* 06 */
	{ "im-msn-e",          E_CONTACT_IM_MSN_HOME_1       }, /* 07 */
	{ "im-icq-e",          E_CONTACT_IM_ICQ_HOME_1       }, /* 08 */
	{ "im-yahoo-e",        E_CONTACT_IM_YAHOO_HOME_1     }, /* 09 */
	{ "im-aim-e",          E_CONTACT_IM_AIM_HOME_1       }, /* 10 */
	{ "im-groupwise-e",    E_CONTACT_IM_GROUPWISE_HOME_1 }, /* 11 */

        { "web-homepage-e",    E_CONTACT_HOMEPAGE_URL        }, /* 12 */
        { "web-calendar-e",    E_CONTACT_CALENDAR_URI        }, /* 13 */
        { "web-weblog-e",      E_CONTACT_BLOG_URL            }, /* 14 */

        { "job-profession-e",  E_CONTACT_ROLE                }, /* 15 */
        { "job-title-e",       E_CONTACT_TITLE               }, /* 16 */
        { "job-dept-e",        E_CONTACT_ORG_UNIT            }, /* 17 */
        { "job-assistant-e",   E_CONTACT_ASSISTANT           }, /* 18 */
        { "job-company-e",     E_CONTACT_ORG                 }, /* 19 */
        { "job-manager-e",     E_CONTACT_MANAGER             }, /* 20 */

	{ "addr-street-1",     ADDRESS_STREET                }, /* 21 */
	{ "addr-po-1", 	       ADDRESS_POBOX                 }, /* 22 */
	{ "addr-locality-1",   ADDRESS_LOCALITY              }, /* 23 */
	{ "addr-code-1",       ADDRESS_CODE                  }, /* 24 */
	{ "addr-region-1",     ADDRESS_REGION                }, /* 25 */
	{ "addr-country-1",    ADDRESS_COUNTRY               }, /* 26 */

	{ "addr-street-2",     ADDRESS_STREET                }, /* 27 */
	{ "addr-po-2", 	       ADDRESS_POBOX                 }, /* 28 */
	{ "addr-locality-2",   ADDRESS_LOCALITY              }, /* 29 */
	{ "addr-code-2",       ADDRESS_CODE                  }, /* 30 */
	{ "addr-region-2",     ADDRESS_REGION                }, /* 31 */
	{ "addr-country-2",    ADDRESS_COUNTRY               }, /* 32 */

        { NULL,            0                             }
};

#define ATTRIBUTE_HOME "HOME"
#define ATTRIBUTE_WORK "WORK"
#define ATTRIBUTE_OTHER "OTHER"

static void about_me_set_address_field (EContactAddress *, guint, gchar *);


/*** Utility functions ***/
static void
about_me_error (GtkWindow *parent, gchar *str)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
				         GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
				         GTK_BUTTONS_OK, str);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

/********************/
static void
about_me_destroy (void)
{
	e_contact_address_free (me->addr1);
	e_contact_address_free (me->addr2);

	if (me->contact)
		g_object_unref (me->contact);
	if (me->book)
		g_object_unref (me->book);
	if (me->dialog)
		g_object_unref (me->dialog);

	g_free (me->email[0]);
	g_free (me->email[1]);
	g_free (me->email[2]);
	g_free (me->email[3]);

	g_free (me->person);
	g_free (me->login);
	g_free (me->username);
	g_free (me);
	me = NULL;
}

static void
about_me_update_email (MateAboutMe *me)
{
	GList *attrs = NULL;
	gint i;

	for (i = 0; i < EMAIL_SLOTS; ++i) {
		if (me->email[i] != NULL) {
			EVCardAttribute *attr;
			const gchar *type = me->email_types[i];

			attr = e_vcard_attribute_new (NULL, EVC_EMAIL);

			e_vcard_attribute_add_param_with_value (attr,
								e_vcard_attribute_param_new (EVC_TYPE),
								type ? type : ATTRIBUTE_OTHER);

			e_vcard_attribute_add_value (attr, me->email[i]);
			attrs = g_list_append (attrs, attr);
		}
	}

	e_contact_set_attributes (me->contact, E_CONTACT_EMAIL, attrs);

	g_list_foreach (attrs, (GFunc) e_vcard_attribute_free, NULL);
	g_list_free (attrs);
}

static void
about_me_commit (MateAboutMe *me)
{
	EContactName *name;

	char *strings[4], **stringptr;
	char *fileas;

	name = NULL;

	if (me->create_self) {
		if (me->username == NULL)
			fileas = g_strdup ("Myself");
		else {
			name = e_contact_name_from_string (me->username);

			stringptr = strings;
			if (name->family && *name->family)
				*(stringptr++) = name->family;
			if (name->given && *name->given)
				*(stringptr++) = name->given;
			*stringptr = NULL;
			fileas = g_strjoinv (", ", strings);
		}

		e_contact_set (me->contact, E_CONTACT_FILE_AS, fileas);
		e_contact_set (me->contact, E_CONTACT_NICKNAME, me->login);
		e_contact_set (me->contact, E_CONTACT_FULL_NAME, me->username);

		e_contact_name_free (name);
		g_free (fileas);
	}

	about_me_update_email (me);

	if (me->create_self) {
		e_book_add_contact (me->book, me->contact, NULL);
		e_book_set_self (me->book, me->contact, NULL);
	} else {
		if (!e_book_commit_contact (me->book, me->contact, NULL))
			g_warning ("Could not save contact information");
	}

	me->create_self = FALSE;
}

static gboolean
about_me_commit_from_timeout (MateAboutMe *me)
{
	about_me_commit (me);

	return FALSE;
}

static gboolean
about_me_focus_out (GtkWidget *widget, GdkEventFocus *event, MateAboutMe *unused)
{
	gchar *str;
	const gchar *wid;
	gint i;

	if (me == NULL)
		return FALSE;

	wid = gtk_buildable_get_name (GTK_BUILDABLE (widget));

	if (wid == NULL)
		return FALSE;

	for (i = 0; ids[i].wid != NULL; i++)
		if (strcmp (ids[i].wid, wid) == 0)
			break;

	if (GTK_IS_ENTRY (widget)) {
		str = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
	} else if (GTK_IS_TEXT_VIEW (widget)) {
		GtkTextBuffer   *buffer;
		GtkTextIter      iter_start;
		GtkTextIter	 iter_end;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		gtk_text_buffer_get_start_iter (buffer, &iter_start);
		iter_end = iter_start;
		gtk_text_iter_forward_to_end (&iter_end);
		str = gtk_text_iter_get_text (&iter_start, &iter_end);
	} else {
		return FALSE;
	}

	if (i == EMAIL_HOME || i == EMAIL_WORK) {

		g_free (me->email[ids[i].cid]);
		if (str[0] == '\0')
			me->email[ids[i].cid] = NULL;
		else
			me->email[ids[i].cid] = g_strdup (str);
		me->email_types[ids[i].cid] = (i == EMAIL_HOME) ? ATTRIBUTE_HOME : ATTRIBUTE_WORK;
	/* FIXME: i'm getting an empty address field in evolution */
	} else if (i >= ADDRESS_HOME && i < ADDRESS_WORK) {
		about_me_set_address_field (me->addr1, ids[i].cid, str);
		e_contact_set (me->contact, E_CONTACT_ADDRESS_HOME, me->addr1);
	} else if (i >= ADDRESS_WORK) {
		about_me_set_address_field (me->addr2, ids[i].cid, str);
		e_contact_set (me->contact, E_CONTACT_ADDRESS_WORK, me->addr2);
	} else {
		e_contact_set (me->contact, ids[i].cid, str);
	}

	g_free (str);

	if (me->commit_timeout_id) {
		g_source_remove (me->commit_timeout_id);
	}

	me->commit_timeout_id = g_timeout_add (600, (GSourceFunc) about_me_commit_from_timeout, me);

	return FALSE;
}

/*
 * Helpers
 */

static gchar *
about_me_get_address_field (EContactAddress *addr, guint cid)
{
	gchar *str;

	if (addr == NULL) {
		return NULL;
	}

	switch (cid) {
		case ADDRESS_STREET:
			str = addr->street;
			break;
		case ADDRESS_POBOX:
			str = addr->po;
			break;
		case ADDRESS_LOCALITY:
			str = addr->locality;
			break;
		case ADDRESS_CODE:
			str = addr->code;
			break;
		case ADDRESS_REGION:
			str = addr->region;
			break;
		case ADDRESS_COUNTRY:
			str = addr->country;
			break;
		default:
			str = NULL;
			break;
	}

	return str;
}

static void
about_me_set_address_field (EContactAddress *addr, guint cid, gchar *str)
{
	switch (cid) {
		case ADDRESS_STREET:
			g_free (addr->street);
			addr->street = g_strdup (str);
			break;
		case ADDRESS_POBOX:
			g_free (addr->po);
			addr->po = g_strdup (str);
			break;
		case ADDRESS_LOCALITY:
			g_free (addr->locality);
			addr->locality = g_strdup (str);
			break;
		case ADDRESS_CODE:
			g_free (addr->code);
			addr->code = g_strdup (str);
			break;
		case ADDRESS_REGION:
			g_free (addr->region);
			addr->region = g_strdup (str);
			break;
		case ADDRESS_COUNTRY:
			g_free (addr->country);
			addr->country = g_strdup (str);
			break;
	}
}

static void
about_me_setup_email (MateAboutMe *me)
{
	GList *attrs, *la;
	gboolean has_home = FALSE, has_work = FALSE;
	guint i;

	attrs = e_contact_get_attributes (me->contact, E_CONTACT_EMAIL);

	for (la = attrs, i = 0; la; la = la->next, ++i) {
		EVCardAttribute *a = la->data;

		me->email[i] = e_vcard_attribute_get_value (a);
		if (e_vcard_attribute_has_type (a, ATTRIBUTE_HOME)) {
			me->email_types[i] = ATTRIBUTE_HOME;
			if (!has_home) {
				ids[EMAIL_HOME].cid = i;
				has_home = TRUE;
			}
		} else if (e_vcard_attribute_has_type (a, ATTRIBUTE_WORK)) {
			me->email_types[i] = ATTRIBUTE_WORK;
			if (!has_work) {
				ids[EMAIL_WORK].cid = i;
				has_work = TRUE;
			}
		} else {
			me->email_types[i] = ATTRIBUTE_OTHER;
		}

		e_vcard_attribute_free (a);
	}

	g_list_free (attrs);

	if (ids[EMAIL_HOME].cid == ids[EMAIL_WORK].cid) {
		if (has_home)
			ids[EMAIL_WORK].cid = 1;
		else
			ids[EMAIL_HOME].cid = 0;
	}

	me->email_types[ids[EMAIL_WORK].cid] = ATTRIBUTE_WORK;
	me->email_types[ids[EMAIL_HOME].cid] = ATTRIBUTE_HOME;
}

/**
 * about_me_load_string_field:
 *
 * wid: UI widget name
 * cid: id of the field (EDS id)
 * aid: position in the array WidToCid
 **/

static void
about_me_load_string_field (MateAboutMe *me, const gchar *wid, guint cid, guint aid)
{
	GtkWidget   *widget;
	GtkBuilder  *dialog;
	const gchar *str;

	dialog = me->dialog;

	widget = WID (wid);

	if (me->create_self == TRUE) {
		g_signal_connect (widget, "focus-out-event", G_CALLBACK (about_me_focus_out), me);
		return;
	}

	if (aid == EMAIL_HOME || aid == EMAIL_WORK) {
		str = e_contact_get_const (me->contact, E_CONTACT_EMAIL_1 + cid);
	} else if (aid >= ADDRESS_HOME && aid < ADDRESS_WORK) {
		str = about_me_get_address_field (me->addr1, cid);
	} else if (aid >= ADDRESS_WORK) {
		str = about_me_get_address_field (me->addr2, cid);
	} else {
		str = e_contact_get_const (me->contact, cid);
	}

	str = str ? str : "";

	if (GTK_IS_ENTRY (widget)) {
		gtk_entry_set_text (GTK_ENTRY (widget), str);
	} else if (GTK_IS_TEXT_VIEW (widget)) {
		GtkTextBuffer *buffer;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
		gtk_text_buffer_set_text (buffer, str, -1);
	}

	g_signal_connect (widget, "focus-out-event", G_CALLBACK (about_me_focus_out), me);
}

static void
about_me_load_photo (MateAboutMe *me, EContact *contact)
{
	GtkBuilder    *dialog;
	EContactPhoto *photo;

	dialog = me->dialog;

	if (me->person)
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);

	photo = e_contact_get (contact, E_CONTACT_PHOTO);

	if (photo && photo->type == E_CONTACT_PHOTO_TYPE_INLINED) {
		me->have_image = TRUE;
		e_image_chooser_set_image_data (E_IMAGE_CHOOSER (me->image_chooser),
						(char *) photo->data.inlined.data, photo->data.inlined.length);
		e_contact_photo_free (photo);
	} else {
		me->have_image = FALSE;
	}
}

static void
about_me_update_photo (MateAboutMe *me)
{
	GtkBuilder    *dialog;
	EContactPhoto *photo;
	gchar         *file;
	GError        *error;

	guchar 	      *data;
	gsize 	       length;

	dialog = me->dialog;


	if (me->image_changed && me->have_image) {
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
		GdkPixbuf *pixbuf, *scaled;
		int height, width;
		gboolean do_scale = FALSE;
		float scale;

		e_image_chooser_get_image_data (E_IMAGE_CHOOSER (me->image_chooser), (char **) &data, &length);

		/* Before updating the image in EDS scale it to a reasonable size
		   so that the user doesn't get an application that does not respond
		   or that takes 100% CPU */
		gdk_pixbuf_loader_write (loader, data, length, NULL);
		gdk_pixbuf_loader_close (loader, NULL);

		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

		if (pixbuf)
			g_object_ref (pixbuf);

		g_object_unref (loader);

		height = gdk_pixbuf_get_height (pixbuf);
		width = gdk_pixbuf_get_width (pixbuf);

		if (height >= width && height > MAX_HEIGHT) {
			scale = (float)MAX_HEIGHT/height;
			do_scale = TRUE;
		} else if (width > height && width > MAX_WIDTH) {
			scale = (float)MAX_WIDTH/width;
			do_scale = TRUE;
		}

		if (do_scale) {
			char *scaled_data = NULL;
			gsize scaled_length;

			scaled = gdk_pixbuf_scale_simple (pixbuf, width*scale, height*scale, GDK_INTERP_BILINEAR);
			gdk_pixbuf_save_to_buffer (scaled, &scaled_data, &scaled_length, "png", NULL,
						   "compression", "9", NULL);

			g_free (data);
			data = (guchar *) scaled_data;
			length = scaled_length;
		}

		photo = g_new0 (EContactPhoto, 1);
		photo->type = E_CONTACT_PHOTO_TYPE_INLINED;
		photo->data.inlined.data = data;
		photo->data.inlined.length = length;
		e_contact_set (me->contact, E_CONTACT_PHOTO, photo);

		/* Save the image for MDM */
		/* FIXME: I would have to read the default used by the mdmgreeter program */
		error = NULL;
		file = g_build_filename (g_get_home_dir (), ".face", NULL);
		if (g_file_set_contents (file,
					 (gchar *) photo->data.inlined.data,
					 photo->data.inlined.length,
					 &error) != FALSE) {
			g_chmod (file, 0644);
		} else {
			g_warning ("Could not create %s: %s", file, error->message);
			g_error_free (error);
		}

		g_free (file);

		e_contact_photo_free (photo);

	} else if (me->image_changed && !me->have_image) {
		/* Update the image in the card */
		e_contact_set (me->contact, E_CONTACT_PHOTO, NULL);

		file = g_build_filename (g_get_home_dir (), ".face", NULL);

		g_unlink (file);

		g_free (file);
	}

	about_me_commit (me);
}

static void
about_me_load_info (MateAboutMe *me)
{
	gint i;

	if (me->create_self == FALSE) {
		me->addr1 = e_contact_get (me->contact, E_CONTACT_ADDRESS_HOME);
		if (me->addr1 == NULL)
			me->addr1 = g_new0 (EContactAddress, 1);
		me->addr2 = e_contact_get (me->contact, E_CONTACT_ADDRESS_WORK);
		if (me->addr2 == NULL)
			me->addr2 = g_new0 (EContactAddress, 1);
	} else {
		me->addr1 = g_new0 (EContactAddress, 1);
		me->addr2 = g_new0 (EContactAddress, 1);
	}

	for (i = 0; ids[i].wid != NULL; i++) {
		about_me_load_string_field (me, ids[i].wid, ids[i].cid, i);
	}

	set_fingerprint_label (me->enable_fingerprint_button,
			       me->disable_fingerprint_button);
}

static void
about_me_update_preview (GtkFileChooser *chooser,
			 MateAboutMe   *me)
{
	gchar *uri;

	uri = gtk_file_chooser_get_preview_uri (chooser);

	if (uri) {
		GtkWidget *image;
		GdkPixbuf *pixbuf = NULL;
		GFile *file;
		GFileInfo *file_info;

		if (!me->thumbs)
			me->thumbs = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

		file = g_file_new_for_uri (uri);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					       G_FILE_QUERY_INFO_NONE,
					       NULL, NULL);
		g_object_unref (file);

		if (file_info != NULL) {
			const gchar *content_type;

			content_type = g_file_info_get_content_type (file_info);
			if (content_type) {
				gchar *mime_type;

				mime_type = g_content_type_get_mime_type (content_type);

				pixbuf = mate_desktop_thumbnail_factory_generate_thumbnail (me->thumbs,
										     uri,
										     mime_type);
				g_free (mime_type);
			}
			g_object_unref (file_info);
		}

		image = gtk_file_chooser_get_preview_widget (chooser);

		if (pixbuf != NULL) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
			g_object_unref (pixbuf);
		} else {
			gtk_image_set_from_stock (GTK_IMAGE (image),
						  "gtk-dialog-question",
						  GTK_ICON_SIZE_DIALOG);
		}
	}
	gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
}

static void
about_me_image_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	GtkFileChooser *chooser_dialog;
	gint response;
	GtkBuilder *dialog;
	GtkWidget  *image;
	const gchar *chooser_dir = DATADIR"/pixmaps/faces";
	const gchar *pics_dir;
	GtkFileFilter *filter;

	dialog = me->dialog;

	chooser_dialog = GTK_FILE_CHOOSER (
			 gtk_file_chooser_dialog_new (_("Select Image"), GTK_WINDOW (WID ("about-me-dialog")),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							_("No Image"), GTK_RESPONSE_NO,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
							NULL));
	gtk_window_set_modal (GTK_WINDOW (chooser_dialog), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (chooser_dialog), GTK_RESPONSE_ACCEPT);

	gtk_file_chooser_add_shortcut_folder (chooser_dialog, chooser_dir, NULL);
	pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
	if (pics_dir != NULL)
		gtk_file_chooser_add_shortcut_folder (chooser_dialog, pics_dir, NULL);

	if (!g_file_test (chooser_dir, G_FILE_TEST_IS_DIR))
		chooser_dir = g_get_home_dir ();

	gtk_file_chooser_set_current_folder (chooser_dialog, chooser_dir);
	gtk_file_chooser_set_use_preview_label (chooser_dialog,	FALSE);

	image = gtk_image_new ();
	gtk_file_chooser_set_preview_widget (chooser_dialog, image);
	gtk_widget_set_size_request (image, 128, -1);

	gtk_widget_show (image);

	g_signal_connect (chooser_dialog, "update-preview",
			  G_CALLBACK (about_me_update_preview), me);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Images"));
	gtk_file_filter_add_pixbuf_formats (filter);
	gtk_file_chooser_add_filter (chooser_dialog, filter);
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter (chooser_dialog, filter);

	response = gtk_dialog_run (GTK_DIALOG (chooser_dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		gchar* filename;

		filename = gtk_file_chooser_get_filename (chooser_dialog);
		me->have_image = TRUE;
		me->image_changed = TRUE;

		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), filename);
		g_free (filename);
		about_me_update_photo (me);
	} else if (response == GTK_RESPONSE_NO) {
		me->have_image = FALSE;
		me->image_changed = TRUE;
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);
		about_me_update_photo (me);
	}

	gtk_widget_destroy (GTK_WIDGET (chooser_dialog));
}

static void
about_me_image_changed_cb (GtkWidget *widget, MateAboutMe *me)
{
	me->have_image = TRUE;
	me->image_changed = TRUE;
	about_me_update_photo (me);
}

/* About Me Dialog Callbacks */

static void
about_me_icon_theme_changed (GtkWindow    *window,
			     GtkIconTheme *theme)
{
	GtkIconInfo *icon;

	icon = gtk_icon_theme_lookup_icon (me->theme, "stock_person", 80, 0);
	if (icon == NULL) {
		g_debug ("Icon not found");
	}
	g_free (me->person);
	me->person = g_strdup (gtk_icon_info_get_filename (icon));

	gtk_icon_info_free (icon);

	if (me->have_image)
		e_image_chooser_set_from_file (E_IMAGE_CHOOSER (me->image_chooser), me->person);
}

static void
about_me_button_clicked_cb (GtkDialog *dialog, gint response_id, MateAboutMe *me)
{
	if (response_id == GTK_RESPONSE_HELP)
		g_print ("Help goes here");
	else {
		if (me->commit_timeout_id) {
			g_source_remove (me->commit_timeout_id);
			about_me_commit (me);
		}

		about_me_destroy ();
		gtk_main_quit ();
	}
}

static void
about_me_passwd_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	GtkBuilder *dialog;

	dialog = me->dialog;
	mate_about_me_password (GTK_WINDOW (WID ("about-me-dialog")));
}

static void
about_me_fingerprint_button_clicked_cb (GtkWidget *button, MateAboutMe *me)
{
	fingerprint_button_clicked (me->dialog,
				    me->enable_fingerprint_button,
				    me->disable_fingerprint_button);
}

static gint
about_me_setup_dialog (void)
{
	GtkWidget    *widget;
	GtkWidget    *main_dialog;
	GtkIconInfo  *icon;
	GtkBuilder   *dialog;
	GError 	     *error = NULL;
	GList        *chain;
	gchar        *str;

	me = g_new0 (MateAboutMe, 1);

	dialog = gtk_builder_new ();
	gtk_builder_add_from_file (dialog, MATECC_UI_DIR "/mate-about-me-dialog.ui", NULL);

	me->image_chooser = e_image_chooser_new ();
	gtk_container_add (GTK_CONTAINER (WID ("button-image")), me->image_chooser);

	if (dialog == NULL) {
		about_me_destroy ();
		return -1;
	}

	me->dialog = dialog;

	/* Connect the close button signal */
	main_dialog = WID ("about-me-dialog");
	g_signal_connect (main_dialog, "response",
			  G_CALLBACK (about_me_button_clicked_cb), me);

	gtk_window_set_resizable (GTK_WINDOW (main_dialog), FALSE);
	capplet_set_icon (main_dialog, "user-info");

	/* Setup theme details */
	me->screen = gtk_window_get_screen (GTK_WINDOW (main_dialog));
	me->theme = gtk_icon_theme_get_for_screen (me->screen);

	icon = gtk_icon_theme_lookup_icon (me->theme, "stock_person", 80, 0);
	if (icon != NULL) {
		me->person = g_strdup (gtk_icon_info_get_filename (icon));
		gtk_icon_info_free (icon);
	}

	g_signal_connect_object (me->theme, "changed",
				 G_CALLBACK (about_me_icon_theme_changed),
				 main_dialog,
				 G_CONNECT_SWAPPED);

	/* Get the self contact */
	if (!e_book_get_self (&me->contact, &me->book, &error)) {
		if (error->code == E_BOOK_ERROR_PROTOCOL_NOT_SUPPORTED) {
			about_me_error (NULL, _("There was an error while trying to get the addressbook information\n" \
						"Evolution Data Server can't handle the protocol"));
			g_clear_error (&error);
			about_me_destroy ();
			return -1;
		}

		g_clear_error (&error);

		me->create_self = TRUE;
		me->contact = e_contact_new ();

		if (me->book == NULL) {
			me->book = e_book_new_system_addressbook (&error);
			if (me->book == NULL || error != NULL) {
				g_error ("%s", error->message);
				g_clear_error (&error);
			}

			if (e_book_open (me->book, FALSE, NULL) == FALSE) {
				about_me_error (GTK_WINDOW (main_dialog),
						_("Unable to open address book"));
				g_clear_error (&error);
			}
		}
	} else {
		about_me_setup_email (me);
	}

	me->login = g_strdup (g_get_user_name ());
	me->username = g_strdup (g_get_real_name ());

	/* Contact Tab */
	about_me_load_photo (me, me->contact);

	widget = WID ("fullname");
	str = g_strdup_printf ("<b><span size=\"xx-large\">%s</span></b>", me->username);

	gtk_label_set_markup (GTK_LABEL (widget), str);
	g_free (str);

	widget = WID ("login");
	gtk_label_set_text (GTK_LABEL (widget), me->login);

	str = g_strdup_printf (_("About %s"), me->username);
	gtk_window_set_title (GTK_WINDOW (main_dialog), str);
	g_free (str);

	widget = WID ("password");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (about_me_passwd_clicked_cb), me);

	widget = WID ("button-image");
	g_signal_connect (widget, "clicked",
			  G_CALLBACK (about_me_image_clicked_cb), me);

	me->enable_fingerprint_button = WID ("enable_fingerprint_button");
	me->disable_fingerprint_button = WID ("disable_fingerprint_button");

	g_signal_connect (me->enable_fingerprint_button, "clicked",
			  G_CALLBACK (about_me_fingerprint_button_clicked_cb), me);
	g_signal_connect (me->disable_fingerprint_button, "clicked",
			  G_CALLBACK (about_me_fingerprint_button_clicked_cb), me);

	g_signal_connect (me->image_chooser, "changed",
			  G_CALLBACK (about_me_image_changed_cb), me);

	/* Address tab: set up the focus chains */
	chain = g_list_prepend (NULL, WID ("addr-country-1"));
	chain = g_list_prepend (chain, WID ("addr-po-1"));
	chain = g_list_prepend (chain, WID ("addr-region-1"));
	chain = g_list_prepend (chain, WID ("addr-code-1"));
	chain = g_list_prepend (chain, WID ("addr-locality-1"));
	chain = g_list_prepend (chain, WID ("addr-scrolledwindow-1"));
	widget = WID ("addr-table-1");
	gtk_container_set_focus_chain (GTK_CONTAINER (widget), chain);
	g_list_free (chain);

	chain = g_list_prepend (NULL, WID ("addr-country-2"));
	chain = g_list_prepend (chain, WID ("addr-po-2"));
	chain = g_list_prepend (chain, WID ("addr-region-2"));
	chain = g_list_prepend (chain, WID ("addr-code-2"));
	chain = g_list_prepend (chain, WID ("addr-locality-2"));
	chain = g_list_prepend (chain, WID ("addr-scrolledwindow-2"));
	widget = WID ("addr-table-2");
	gtk_container_set_focus_chain (GTK_CONTAINER (widget), chain);
	g_list_free (chain);

	about_me_load_info (me);

	gtk_widget_show_all (main_dialog);

	return 0;
}

int
main (int argc, char **argv)
{
	int rc = 0;

	capplet_init (NULL, &argc, &argv);

	if (!g_thread_supported ())
		g_thread_init (NULL);

	dbus_g_object_register_marshaller (fprintd_marshal_VOID__STRING_BOOLEAN,
					   G_TYPE_NONE, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INVALID);

	rc = about_me_setup_dialog ();

	if (rc != -1) {
		gtk_main ();
	}

	return rc;
}
