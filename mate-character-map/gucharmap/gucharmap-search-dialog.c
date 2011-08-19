/*
 * Copyright © 2004 Noah Levitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#include <config.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "gucharmap-search-dialog.h"
#include "gucharmap-window.h"

#define GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), gucharmap_search_dialog_get_type (), GucharmapSearchDialogPrivate))

#define I_(string) g_intern_static_string (string)

enum
{
  SEARCH_START,
  SEARCH_FINISH,
  NUM_SIGNALS
};

static guint gucharmap_search_dialog_signals[NUM_SIGNALS];

enum
{
  GUCHARMAP_RESPONSE_PREVIOUS,
  GUCHARMAP_RESPONSE_NEXT
};

typedef struct _GucharmapSearchDialogPrivate GucharmapSearchDialogPrivate;
typedef struct _GucharmapSearchState GucharmapSearchState;

struct _GucharmapSearchState
{
  GucharmapCodepointList *list;
  gchar                  *search_string;
  gchar                  *search_string_nfd_temp;
  gchar                  *search_string_nfd;  /* points into search_string_nfd_temp */
  gint                    search_string_nfd_len;
  gint                    search_index_nfd;
  gchar                  *search_string_nfc;
  gint                    search_string_nfc_len;
  gint                    search_index_nfc;
  gint                    search_string_value;
  gint                    start_index;
  gint                    curr_index;
  GucharmapDirection      increment;
  gboolean                whole_word;
  gboolean                annotations;
  gint                    found_index;       /* index of the found character */
  /* true if there are known to be no matches, or there is known to be
   * exactly one match and it has been found */
  gboolean                dont_search;
  gboolean                did_before_checks;
  gpointer                saved_data;        /* holds some data to pass back to the caller */
  gint                    list_num_chars;    /* last_index + 1 */
  gboolean                searching;
  gint                    strings_checked;
};

struct _GucharmapSearchDialogPrivate
{
  GucharmapWindow       *guw;
  GtkWidget             *entry;
  GtkWidget             *whole_word_option;
  GtkWidget             *annotations_option;
  GucharmapSearchState  *search_state;
  GtkWidget             *prev_button;
  GtkWidget             *next_button;
};

static void gucharmap_search_dialog_class_init (GucharmapSearchDialogClass *klass);
static void gucharmap_search_dialog_init       (GucharmapSearchDialog *dialog);

G_DEFINE_TYPE (GucharmapSearchDialog, gucharmap_search_dialog, GTK_TYPE_DIALOG)

static const gchar *
utf8_strcasestr (const gchar *haystack, 
                 const gchar *needle,
		 const gboolean whole_word)
{
  gint needle_len = strlen (needle);
  gint haystack_len = strlen (haystack);
  const gchar *p, *q, *r;

  for (p = haystack;  p + needle_len <= haystack + haystack_len;  p = g_utf8_next_char (p))
    {
      if (whole_word && !(p == haystack || g_unichar_isspace(p[-1])))
	goto next;

      for (q = needle, r = p;  *q && *r;  q = g_utf8_next_char (q), r = g_utf8_next_char (r))
        {
          gunichar lc0 = g_unichar_tolower (g_utf8_get_char (r));
          gunichar lc1 = g_unichar_tolower (g_utf8_get_char (q));
          if (lc0 != lc1)
            goto next;
        }

      if (whole_word && !(r[0] == '\0' || g_unichar_isspace(r[0])))
	goto next;

      return p;

      next:
        ;
    }

  return NULL;
}

static gboolean
matches (GucharmapSearchDialog *search_dialog,
         gunichar               wc,
         const gchar           *search_string_nfd,
         const gboolean         annotations)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  const gchar *haystack; 
  const gchar **haystack_arr; 
  gchar *haystack_nfd;
  gboolean matched = FALSE;
  gint i;

  haystack = gucharmap_get_unicode_data_name (wc);
  if (haystack)
    {
      priv->search_state->strings_checked++;

      /* character names are ascii, so are nfd */
      haystack_nfd = (gchar *) haystack;
      matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
    }

  if (annotations)
    {

#if ENABLE_UNIHAN
      haystack = gucharmap_get_unicode_kDefinition (wc);
      if (haystack)
	{
	  priv->search_state->strings_checked++;

	  haystack_nfd = g_utf8_normalize (haystack, -1, G_NORMALIZE_NFD);
	  matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
	  g_free (haystack_nfd);
	}

      if (matched)
	return TRUE;
#endif

      haystack_arr = gucharmap_get_nameslist_equals (wc);
      if (haystack_arr)
	{
	  for (i = 0; haystack_arr[i] != NULL; i++)
	    {
	      priv->search_state->strings_checked++;

	      haystack_nfd = g_utf8_normalize (haystack_arr[i], -1, G_NORMALIZE_NFD);
	      matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
	      g_free (haystack_nfd);
	      if (matched)
		break;
	    }
	  g_free (haystack_arr);
	}

      if (matched)
	return TRUE;

      haystack_arr = gucharmap_get_nameslist_stars (wc);
      if (haystack_arr)
	{
	  for (i = 0; haystack_arr[i] != NULL; i++)
	    {
	      priv->search_state->strings_checked++;

	      haystack_nfd = g_utf8_normalize (haystack_arr[i], -1, G_NORMALIZE_NFD);
	      matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
	      g_free (haystack_nfd);
	      if (matched)
		break;
	    }
	  g_free (haystack_arr);
	}

      if (matched)
	return TRUE;

      haystack_arr = gucharmap_get_nameslist_colons (wc);
      if (haystack_arr)
	{
	  for (i = 0; haystack_arr[i] != NULL; i++)
	    {
	      priv->search_state->strings_checked++;

	      haystack_nfd = g_utf8_normalize (haystack_arr[i], -1, G_NORMALIZE_NFD);
	      matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
	      g_free (haystack_nfd);
	      if (matched)
		break;
	    }
	  g_free (haystack_arr);
	}

      if (matched)
	return TRUE;

      haystack_arr = gucharmap_get_nameslist_pounds (wc);
      if (haystack_arr)
	{
	  for (i = 0; haystack_arr[i] != NULL; i++)
	    {
	      priv->search_state->strings_checked++;

	      haystack_nfd = g_utf8_normalize (haystack_arr[i], -1, G_NORMALIZE_NFD);
	      matched = utf8_strcasestr (haystack_nfd, search_string_nfd, priv->search_state->whole_word) != NULL;
	      g_free (haystack_nfd);
	      if (matched)
		break;
	    }
	  g_free (haystack_arr);
	}

      if (matched)
	return TRUE;
    }

  /* XXX: other strings */

  return matched;
}

/* string should have no leading spaces */
static gint
check_for_explicit_codepoint (const GucharmapCodepointList *list,
                              const gchar                  *string)
{
  const gchar *nptr;
  gchar *endptr;
  gunichar wc;

  /* check for explicit decimal codepoint */
  nptr = string;
  if (g_ascii_strncasecmp (string, "&#", 2) == 0)
    nptr = string + 2;
  else if (*string == '#')
    nptr = string + 1;

  if (nptr != string)
    {
      wc = strtoul (nptr, &endptr, 10);
      if (endptr != nptr)
        {
          gint index = gucharmap_codepoint_list_get_index ((GucharmapCodepointList *) list, wc);
          if (index != -1)
            return index;
        }
    }

  /* check for explicit hex code point */
  nptr = string;
  if (g_ascii_strncasecmp (string, "&#x", 3) == 0)
    nptr = string + 3;
  else if (g_ascii_strncasecmp (string, "U+", 2) == 0 || g_ascii_strncasecmp (string, "0x", 2) == 0)
    nptr = string + 2;

  if (nptr != string)
    {
      wc = strtoul (nptr, &endptr, 16);
      if (endptr != nptr)
        {
          gint index = gucharmap_codepoint_list_get_index ((GucharmapCodepointList *) list, wc);
          if (index != -1)
            return index;
        }
    }

  /* check for hex codepoint without any prefix */
  /* as unicode standard assigns numerical codes to characters, its very usual
   * to search with character code without any prefix. so moved it to here.
   */
  wc = strtoul (string, &endptr, 16);
  if (endptr-3 >= string)
    {
      gint index = gucharmap_codepoint_list_get_index ((GucharmapCodepointList *) list, wc);
      if (index != -1)
	return index;
    }

  return -1;
}

static gboolean
quick_checks_before (GucharmapSearchState *search_state)
{
  if (search_state->dont_search)
    return TRUE;

  if (search_state->did_before_checks)
    return FALSE;
  search_state->did_before_checks = TRUE;

  g_return_val_if_fail (search_state->search_string_nfd != NULL, FALSE);
  g_return_val_if_fail (search_state->search_string_nfc != NULL, FALSE);

  /* caller should check for empty string */
  if (search_state->search_string_nfd[0] == '\0')
    {
      search_state->dont_search = TRUE;
      return TRUE;
    }

  if (!search_state->whole_word)
    {

      /* if NFD of the search string is a single character, jump to that */
      if (search_state->search_string_nfd_len == 1)
	{
	  if (search_state->search_index_nfd != -1)
	    {
	      search_state->found_index = search_state->curr_index = search_state->search_index_nfd;
	      search_state->dont_search = TRUE;
	      return TRUE;
	    }
	}

      /* if NFC of the search string is a single character, jump to that */
      if (search_state->search_string_nfc_len == 1)
	{
	  if (search_state->search_index_nfc != -1)
	    {
	      search_state->found_index = search_state->curr_index = search_state->search_index_nfc;
	      search_state->dont_search = TRUE;
	      return TRUE;
	    }
	}

    }

  return FALSE;
}

static gboolean
quick_checks_after (GucharmapSearchState *search_state)
{
  /* jump to the first nonspace character unless it’s plain ascii */
  if (!search_state->whole_word)
    if (search_state->search_string_nfd[0] < 0x20 || search_state->search_string_nfd[0] > 0x7e)
      {
	gint index = gucharmap_codepoint_list_get_index (search_state->list, g_utf8_get_char (search_state->search_string_nfd));
	if (index != -1)
	  {
	    search_state->found_index = index;
	    search_state->dont_search = TRUE;
	    return TRUE;
	  }
      }

  return FALSE;
}

static gboolean
idle_search (GucharmapSearchDialog *search_dialog)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  gunichar wc;
  GTimer *timer;

  /* search without leading and tailing spaces */
  /* with "match whole word" option, there's no need for leading and tailing spaces */

  if (quick_checks_before (priv->search_state))
    return FALSE;

  timer = g_timer_new ();

  do
    {
      priv->search_state->curr_index = (priv->search_state->curr_index + priv->search_state->increment + priv->search_state->list_num_chars) % priv->search_state->list_num_chars;
      wc = gucharmap_codepoint_list_get_char (priv->search_state->list, priv->search_state->curr_index);

      if (!gucharmap_unichar_validate (wc) || !gucharmap_unichar_isdefined (wc))
        continue;


      /* check for explicit codepoint */
      if (priv->search_state->search_string_value != -1 && priv->search_state->curr_index == priv->search_state->search_string_value)
	{
	  priv->search_state->found_index = priv->search_state->curr_index;
          g_timer_destroy (timer);
	  return FALSE;
	}

      /* check for other matches */
      if (matches (search_dialog, wc, priv->search_state->search_string_nfd, priv->search_state->annotations))
        {
          priv->search_state->found_index = priv->search_state->curr_index;
          g_timer_destroy (timer);
          return FALSE;
        }

      if (g_timer_elapsed (timer, NULL) > 0.050)
        {
          g_timer_destroy (timer);
          return TRUE;
        }
    }
  while (priv->search_state->curr_index != priv->search_state->start_index);

  g_timer_destroy (timer);

  if (quick_checks_after (priv->search_state))
    return FALSE;

  priv->search_state->dont_search = TRUE;

  return FALSE;
}

/**
 * gucharmap_search_state_get_found_char:
 * @search_state: 
 * Return value: 
 **/
static gunichar
gucharmap_search_state_get_found_char (GucharmapSearchState *search_state)
{
  if (search_state->found_index > 0)
    return gucharmap_codepoint_list_get_char (search_state->list, search_state->found_index);
  else
    return (gunichar)(-1);
}

/**
 * gucharmap_search_state_free:
 * @search_state: 
 **/
static void
gucharmap_search_state_free (GucharmapSearchState *search_state)
{
  g_object_unref (search_state->list);
  g_free (search_state->search_string_nfd_temp);
  g_free (search_state->search_string_nfc);
  g_slice_free (GucharmapSearchState, search_state);
}

/**
 * gucharmap_search_state_new:
 * @list: a #GucharmapCodepointList to be searched
 * @search_string: the text to search for
 * @start_index: the starting point within @list
 * @direction: forward or backward
 * @whole_word: %TRUE if it should match whole words
 * @annotations: %TRUE if it should search in character's annotations
 *
 * Initializes a #GucharmapSearchState to search for the next character in
 * the codepoint list that matches @search_string. Assumes input is valid.
 *
 * Return value: the new #GucharmapSearchState.
 **/
static GucharmapSearchState * 
gucharmap_search_state_new (GucharmapCodepointList       *list,
                            const gchar                  *search_string, 
                            gint                          start_index, 
                            GucharmapDirection            direction, 
                            gboolean                      whole_word,
                            gboolean                      annotations)
{
  GucharmapSearchState *search_state;
  gchar *p, *q, *r;

  g_assert (direction == GUCHARMAP_DIRECTION_BACKWARD || direction == GUCHARMAP_DIRECTION_FORWARD);

  search_state = g_slice_new (GucharmapSearchState);

  search_state->list = g_object_ref (list);
  search_state->list_num_chars = gucharmap_codepoint_list_get_last_index (search_state->list) + 1;

  search_state->search_string = g_strdup (search_string);
  search_state->search_string_nfd_temp = g_utf8_normalize (search_string, -1, G_NORMALIZE_NFD);

  search_state->increment = direction;
  search_state->whole_word = whole_word;
  search_state->annotations = annotations;
  search_state->did_before_checks = FALSE;

  search_state->found_index = -1;
  search_state->dont_search = FALSE;

  search_state->start_index = start_index;
  search_state->curr_index = start_index;

  /* set end of search string to last non-space character */
  for (p = q = r = search_state->search_string_nfd_temp;
       p[0] != '\0';
       q = p, p = g_utf8_next_char (p))
    if (g_unichar_isspace (g_utf8_get_char (p)) && !g_unichar_isspace (g_utf8_get_char (q)))
	r = p;
  if (!g_unichar_isspace (g_utf8_get_char (q)))
      r = p;
  r[0] = '\0';

  /* caller should check not to search for empty string */
  g_return_val_if_fail (r != search_state->search_string_nfd_temp, FALSE);

  /* NFD */
  /* set pointer to first non-space character in the search string */
  for (search_state->search_string_nfd = search_state->search_string_nfd_temp;
       *search_state->search_string_nfd != '\0'
       && g_unichar_isspace (g_utf8_get_char (search_state->search_string_nfd));
       search_state->search_string_nfd = g_utf8_next_char (search_state->search_string_nfd))
    ;
  search_state->search_string_nfd_len = g_utf8_strlen (search_state->search_string_nfd, -1);
  if (search_state->search_string_nfd_len == 1)
    search_state->search_index_nfd = gucharmap_codepoint_list_get_index (search_state->list, g_utf8_get_char (search_state->search_string_nfd));
  else
    search_state->search_index_nfd = -1;

  /* NFC */
  search_state->search_string_nfc = g_utf8_normalize (search_state->search_string_nfd, -1, G_NORMALIZE_NFC);
  search_state->search_string_nfc_len = g_utf8_strlen (search_state->search_string_nfc, -1);
  if (search_state->search_string_nfc_len == 1)
    search_state->search_index_nfc = gucharmap_codepoint_list_get_index (search_state->list, g_utf8_get_char (search_state->search_string_nfc));
  else
    search_state->search_index_nfc = -1;

  /* INDEX */
  search_state->search_string_value = check_for_explicit_codepoint (search_state->list, search_state->search_string_nfd);

  search_state->searching = FALSE;
  return search_state;
}

static void
information_dialog (GucharmapSearchDialog *search_dialog,
                    const gchar           *message)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  GtkWidget *dialog;

#if GTK_CHECK_VERSION (2,18,0)
  dialog = gtk_message_dialog_new (gtk_widget_get_visible (GTK_WIDGET (search_dialog)) ?
#else
  dialog = gtk_message_dialog_new (GTK_WIDGET_VISIBLE (search_dialog) ?
#endif
                                     GTK_WINDOW (search_dialog) :
                                     GTK_WINDOW (priv->guw),
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Information"));
  gtk_window_set_icon_name (GTK_WINDOW (dialog), GUCHARMAP_ICON_NAME);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
search_completed (GucharmapSearchDialog *search_dialog)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  gunichar found_char = gucharmap_search_state_get_found_char (priv->search_state);

  priv->search_state->searching = FALSE;

  g_signal_emit (search_dialog, gucharmap_search_dialog_signals[SEARCH_FINISH], 0, found_char);

  if (found_char == (gunichar)(-1))
    {
      information_dialog (search_dialog, _("Not found."));
      gtk_widget_set_sensitive (priv->prev_button, FALSE);
      gtk_widget_set_sensitive (priv->next_button, FALSE);
    }

  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (search_dialog)), NULL);
}

static gboolean
_entry_is_empty (GtkEntry *entry)
{
  const gchar *text = gtk_entry_get_text (entry);
  const gchar *p;	/* points into text */

  for (p = text;
       p[0] != '\0' && g_unichar_isspace (g_utf8_get_char (p));
       p = g_utf8_next_char (p))
    ;
  return p[0] == '\0';
}

static void
_gucharmap_search_dialog_fire_search (GucharmapSearchDialog *search_dialog,
                                      GucharmapDirection     direction)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  GucharmapCodepointList *list;
  gunichar start_char;
  gint start_index;

  if (priv->search_state && priv->search_state->searching) /* Already searching */
    return;

  GdkCursor *cursor = _gucharmap_window_progress_cursor ();
  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (search_dialog)), cursor);
  gdk_cursor_unref (cursor);

  list = gucharmap_charmap_get_book_codepoint_list (priv->guw->charmap);
  if (!list)
    return;

  if (priv->search_state == NULL
      || list != priv->search_state->list
      || strcmp (priv->search_state->search_string, gtk_entry_get_text (GTK_ENTRY (priv->entry))) != 0
      || priv->search_state->whole_word != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->whole_word_option))
      || priv->search_state->annotations != gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->annotations_option)) )
    {
      if (priv->search_state)
        gucharmap_search_state_free (priv->search_state);

      start_char = gucharmap_charmap_get_active_character (priv->guw->charmap);
      start_index = gucharmap_codepoint_list_get_index (list, start_char);
      priv->search_state = gucharmap_search_state_new (list, gtk_entry_get_text (GTK_ENTRY (priv->entry)),
			      start_index, direction,
			      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->whole_word_option)),
			      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->annotations_option)) );
    }
  else
    {
      start_char = gucharmap_charmap_get_active_character (priv->guw->charmap);
      priv->search_state->start_index = gucharmap_codepoint_list_get_index (list, start_char);
      priv->search_state->curr_index = priv->search_state->start_index;
      priv->search_state->increment = direction;
    }

  priv->search_state->searching = TRUE;
  priv->search_state->strings_checked = 0;

  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, (GSourceFunc) idle_search, search_dialog, (GDestroyNotify) search_completed);
  g_signal_emit (search_dialog, gucharmap_search_dialog_signals[SEARCH_START], 0);

  g_object_unref (list);
}

void
gucharmap_search_dialog_start_search (GucharmapSearchDialog *search_dialog,
                                      GucharmapDirection     direction)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);

  if (priv->search_state != NULL && !_entry_is_empty (GTK_ENTRY (priv->entry)))
    _gucharmap_search_dialog_fire_search (search_dialog, direction);
  else
    gtk_window_present (GTK_WINDOW (search_dialog));
}

static void
search_find_response (GtkDialog *dialog,
                      gint       response)
{
  GucharmapSearchDialog *search_dialog = GUCHARMAP_SEARCH_DIALOG (dialog);
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);

  switch (response)
    {
      case GUCHARMAP_RESPONSE_PREVIOUS:
        _gucharmap_search_dialog_fire_search (search_dialog, GUCHARMAP_DIRECTION_BACKWARD);
        break;

      case GUCHARMAP_RESPONSE_NEXT:
        _gucharmap_search_dialog_fire_search (search_dialog, GUCHARMAP_DIRECTION_FORWARD);
        break;

      default:
        gtk_widget_hide (GTK_WIDGET (search_dialog));
        break;
    }

  gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);
}

static void
entry_changed (GtkObject             *object,
               GucharmapSearchDialog *search_dialog)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  gboolean is_empty;

  is_empty = _entry_is_empty (GTK_ENTRY (priv->entry));
      
  gtk_widget_set_sensitive (priv->prev_button, !is_empty);
  gtk_widget_set_sensitive (priv->next_button, !is_empty);
}

static void 
set_button_stock_image_and_label (GtkButton *button,
                                  gchar     *stock_id,
                                  gchar     *mnemonic)
{
  GtkWidget *image;

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (button, image);
  gtk_button_set_label (button, mnemonic);
  gtk_button_set_use_underline (button, TRUE);
}

static void
gucharmap_search_dialog_init (GucharmapSearchDialog *search_dialog)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);
  GtkWidget *hbox, *label, *content_area;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (search_dialog));

  /* follow hig guidelines */
  gtk_window_set_title (GTK_WINDOW (search_dialog), _("Find"));
  gtk_container_set_border_width (GTK_CONTAINER (search_dialog), 6);
  gtk_dialog_set_has_separator (GTK_DIALOG (search_dialog), FALSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (search_dialog), TRUE);
  gtk_box_set_spacing (GTK_BOX (content_area), 12);
  gtk_window_set_resizable (GTK_WINDOW (search_dialog), FALSE);

  g_signal_connect (search_dialog, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  /* add buttons */
  gtk_dialog_add_button (GTK_DIALOG (search_dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  priv->prev_button = gtk_button_new ();
#if GTK_CHECK_VERSION (2,18,0)
  gtk_widget_set_can_default (priv->prev_button, TRUE);
#else
  GTK_WIDGET_SET_FLAGS (priv->prev_button, GTK_CAN_DEFAULT);
#endif
  set_button_stock_image_and_label (GTK_BUTTON (priv->prev_button), GTK_STOCK_GO_BACK, _("_Previous"));
  gtk_dialog_add_action_widget (GTK_DIALOG (search_dialog), priv->prev_button, GUCHARMAP_RESPONSE_PREVIOUS);
  gtk_widget_show (priv->prev_button);

  priv->next_button = gtk_button_new ();
#if GTK_CHECK_VERSION (2,18,0)
  gtk_widget_set_can_default (priv->prev_button, TRUE);
#else
  GTK_WIDGET_SET_FLAGS (priv->prev_button, GTK_CAN_DEFAULT);
#endif
  gtk_widget_show (priv->next_button);
  set_button_stock_image_and_label (GTK_BUTTON (priv->next_button), GTK_STOCK_GO_FORWARD, _("_Next"));
  gtk_dialog_add_action_widget (GTK_DIALOG (search_dialog), priv->next_button, GUCHARMAP_RESPONSE_NEXT);

  gtk_dialog_set_default_response (GTK_DIALOG (search_dialog), GUCHARMAP_RESPONSE_NEXT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (search_dialog),
                                           GUCHARMAP_RESPONSE_PREVIOUS,
                                           GUCHARMAP_RESPONSE_NEXT,
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  priv->entry = gtk_entry_new ();
  gtk_widget_show (priv->entry);
  gtk_entry_set_activates_default (GTK_ENTRY (priv->entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), priv->entry, TRUE, TRUE, 0);
  g_signal_connect (priv->entry, "changed", G_CALLBACK (entry_changed), search_dialog);

  priv->whole_word_option = gtk_check_button_new_with_mnemonic (_("Match _whole word"));
  gtk_widget_show (priv->whole_word_option);
  gtk_box_pack_start (GTK_BOX (content_area), priv->whole_word_option, FALSE, FALSE, 0);
  g_signal_connect (priv->whole_word_option, "toggled", G_CALLBACK (entry_changed), search_dialog);

  priv->annotations_option = gtk_check_button_new_with_mnemonic (_("Search in character _details"));
  gtk_widget_show (priv->annotations_option);
  gtk_box_pack_start (GTK_BOX (content_area), priv->annotations_option, FALSE, FALSE, 0);
  g_signal_connect (priv->annotations_option, "toggled", G_CALLBACK (entry_changed), search_dialog);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->entry);

  /* since the entry is empty */
  gtk_widget_set_sensitive (priv->prev_button, FALSE);
  gtk_widget_set_sensitive (priv->next_button, FALSE);

  priv->search_state = NULL;
  priv->guw = NULL;

  g_signal_connect (GTK_DIALOG (search_dialog), "response", G_CALLBACK (search_find_response), NULL);
}

static void 
gucharmap_search_dialog_finalize (GObject *object)
{
  GucharmapSearchDialog *search_dialog = GUCHARMAP_SEARCH_DIALOG (object);
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);

  if (priv->search_state)
    gucharmap_search_state_free (priv->search_state);

  G_OBJECT_CLASS (gucharmap_search_dialog_parent_class)->finalize (object);
}

static void
gucharmap_search_dialog_class_init (GucharmapSearchDialogClass *clazz)
{
  g_type_class_add_private (clazz, sizeof (GucharmapSearchDialogPrivate));

  G_OBJECT_CLASS (clazz)->finalize = gucharmap_search_dialog_finalize;

  clazz->search_start = NULL;
  clazz->search_finish = NULL;

  gucharmap_search_dialog_signals[SEARCH_START] =
      g_signal_new (I_("search-start"), gucharmap_search_dialog_get_type (), G_SIGNAL_RUN_FIRST,
                    G_STRUCT_OFFSET (GucharmapSearchDialogClass, search_start), NULL, NULL, 
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  gucharmap_search_dialog_signals[SEARCH_FINISH] =
      g_signal_new (I_("search-finish"), gucharmap_search_dialog_get_type (), G_SIGNAL_RUN_FIRST, 
                    G_STRUCT_OFFSET (GucharmapSearchDialogClass, search_finish), NULL, NULL, 
                    g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
}

GtkWidget *
gucharmap_search_dialog_new (GucharmapWindow *guw)
{
  GucharmapSearchDialog *search_dialog = g_object_new (gucharmap_search_dialog_get_type (), NULL);
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);

  priv->guw = guw;

  gtk_window_set_transient_for (GTK_WINDOW (search_dialog), GTK_WINDOW (guw));

  if (guw)
    gtk_window_set_icon (GTK_WINDOW (search_dialog), gtk_window_get_icon (GTK_WINDOW (guw)));

  return GTK_WIDGET (search_dialog);
}

void
gucharmap_search_dialog_present (GucharmapSearchDialog *search_dialog)
{
  gtk_widget_grab_focus (GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog)->entry);
  gtk_window_present (GTK_WINDOW (search_dialog));
}

gdouble
gucharmap_search_dialog_get_completed (GucharmapSearchDialog *search_dialog)
{
  GucharmapSearchDialogPrivate *priv = GUCHARMAP_SEARCH_DIALOG_GET_PRIVATE (search_dialog);

  if (priv->search_state == NULL || !priv->search_state->searching)
    return -1.0;
  else
    {
#if ENABLE_UNIHAN
      gdouble total = gucharmap_get_unicode_data_name_count () + gucharmap_get_unihan_count ();
#else
      gdouble total = gucharmap_get_unicode_data_name_count ();
#endif
      return (gdouble) priv->search_state->strings_checked / total;
    }
}
