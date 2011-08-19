/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#include <string.h>

#include "gst-filter.h"

typedef enum {
  IP_UNK,
  IP_V4,
  IP_V6
} IpVersion;

/* gets the value of a numeric IP address section */
static gint
get_address_section_value (const gchar *text, gint start, gint len)
{
  gchar *c = (gchar *) &text[start];
  gchar *str = g_strndup (c, len);
  gint value = g_strtod (str, NULL);
  gint i;

  for (i = 0; i < len; i++)
    {
      if ((str[i] < '0') || (str [i] > '9'))
        {
          g_free (str);
          return 256;
        }
    }

  g_free (str);
  return value;
}

/* I don't expect this function to be understood, but
 * it works with IPv4, IPv6 and IPv4 embedded in IPv6
 */
GstAddressRet
gst_filter_check_ip_address (const gchar *text)
{
  gint      i, len, numsep, section_val, nsegments;
  gboolean  has_double_colon, segment_has_alpha;
  IpVersion ver;
  gchar     c;

  if (!text)
    return GST_ADDRESS_INCOMPLETE;

  ver = IP_UNK;
  len = 0;
  numsep = 0;
  nsegments = 0;
  has_double_colon = FALSE;
  segment_has_alpha = FALSE;

  for (i = 0; text[i]; i++)
    {
      c = text[i];

      if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
        {
          len++;

          if ((ver == IP_V4) ||
              ((ver == IP_V6) && (i == 1) && (text[0] == ':')) ||
              ((ver == IP_V6) && (len > 4)))
            return GST_ADDRESS_ERROR;

          if (ver == IP_UNK)
            ver = IP_V6;

          segment_has_alpha = TRUE;
        }
      else if (c >='0' && c <='9')
        {
	len++;
	section_val = get_address_section_value (text, i - len + 1, len);

	if (((ver == IP_V4) && ((len > 3) || (section_val > 255))) ||
	    ((ver == IP_V6) && (i == 1) && (text[0] == ':')) ||
	    ((ver == IP_V6) && (len > 4)) ||
	    ((ver == IP_UNK) && (len > 4)))
	  return GST_ADDRESS_ERROR;

	if ((ver == IP_UNK) && ((len == 4) || (section_val > 255)))
	  ver = IP_V6;
        }
      else if (c == '.')
        {
	section_val = get_address_section_value (text, i - len, len);
	numsep++;

	if ((len == 0) ||
	    ((ver == IP_V4) && (numsep > 3)) ||
	    ((ver == IP_V6) && ((numsep > 6) || (len > 3) || (len == 0) || (section_val > 255))))
	  return GST_ADDRESS_ERROR;

	if ((ver == IP_V6) && (len >= 1) && (len <= 3) && (!segment_has_alpha) && (section_val <= 255))
	  {
	    ver = IP_V4;
	    numsep = 1;
	  }

	if ((ver == IP_UNK) && (section_val <= 255))
	  ver = IP_V4;

	len = 0;
        }
      else if (c == ':')
        {
          numsep++;

          if ((ver == IP_V4) ||
              (numsep >= 8) ||
              ((len == 0) && (has_double_colon)))
            return GST_ADDRESS_ERROR;

          if ((numsep > 1) && (len == 0) && (!has_double_colon))
            has_double_colon = TRUE;

          if (ver == IP_UNK)
            ver = IP_V6;

          len = 0;
          segment_has_alpha = FALSE;
        }
      else 
        return GST_ADDRESS_ERROR;
    }

  if ((ver == IP_V4) && (numsep == 3) && (len > 0))
    return GST_ADDRESS_IPV4;
  else if ((ver == IP_V6) && (len > 0) && ((numsep == 8) || has_double_colon))
    return GST_ADDRESS_IPV6;

  if (ver == IP_V4)
    return GST_ADDRESS_IPV4_INCOMPLETE;
  else if (ver == IP_V6)
    return GST_ADDRESS_IPV6_INCOMPLETE;

  return GST_ADDRESS_INCOMPLETE;
}

static gboolean
check_string (gint filter, const gchar *str)
{
  gboolean success = FALSE;
  gint ret;

  if ((filter == GST_FILTER_IP) ||
      (filter == GST_FILTER_IPV4) ||
      (filter == GST_FILTER_IPV6))
    {
      ret = gst_filter_check_ip_address (str);

      if (filter == GST_FILTER_IP)
        success = ((ret == GST_ADDRESS_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV4_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV4) ||
                   (ret == GST_ADDRESS_IPV6_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV6));
      else if (filter == GST_FILTER_IPV4)
        success = ((ret == GST_ADDRESS_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV4_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV4));
      else if (filter == GST_FILTER_IPV6)
        success = ((ret == GST_ADDRESS_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV6_INCOMPLETE) ||
                   (ret == GST_ADDRESS_IPV6));
    }
  else if (filter == GST_FILTER_PHONE)
    success = (strspn (str, "0123456789abcdABCD,#*") == strlen (str));

  return success;
}

static void
insert_filter (GtkEditable *editable, const gchar *text, gint length, gint *pos, gpointer data)
{
  gint   filter;
  gchar *str, *pre, *post;

  filter = GPOINTER_TO_INT (data);
  pre  = gtk_editable_get_chars (editable, 0, *pos);
  post = gtk_editable_get_chars (editable, *pos, -1);

  str = g_strconcat (pre, text, post, NULL);

  if (!check_string (filter, str))
    g_signal_stop_emission_by_name (G_OBJECT (editable), "insert-text");

  g_free (pre);
  g_free (post);
  g_free (str);
}

static void
delete_filter (GtkEditable *editable, gint start, gint end, gpointer data)
{
  gint   filter;
  gchar *str, *pre, *post;

  filter = GPOINTER_TO_INT (data);
  pre  = gtk_editable_get_chars (editable, 0, start);
  post = gtk_editable_get_chars (editable, end, -1);

  str = g_strconcat (pre, post, NULL);

  if (!check_string (filter, str))
    g_signal_stop_emission_by_name (G_OBJECT (editable), "delete-text");

  g_free (pre);
  g_free (post);
  g_free (str);
}

void
gst_filter_init (GtkEntry *entry, gint filter)
{
  g_signal_connect (G_OBJECT (entry), "insert-text",
                    G_CALLBACK (insert_filter), GINT_TO_POINTER (filter));

  g_signal_connect (G_OBJECT (entry), "delete-text",
                    G_CALLBACK (delete_filter), GINT_TO_POINTER (filter));
}
