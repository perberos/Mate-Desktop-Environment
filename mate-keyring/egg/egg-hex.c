/* 
 * mate-keyring
 * 
 * Copyright (C) 2008 Stefan Walter
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU Lesser General  License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General  License for more details.
 *  
 * You should have received a copy of the GNU Lesser General 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.  
 */

#include "config.h"

#include "egg-hex.h"

#include <string.h>

static const char HEXC_UPPER[] = "0123456789ABCDEF";
static const char HEXC_LOWER[] = "0123456789abcdef";

guchar*
egg_hex_decode (const gchar *data, gssize n_data, gsize *n_decoded)
{
	guchar *result;
	guchar *decoded;
	gushort j;
	gint state = 0;
	const gchar* pos;
    
	g_return_val_if_fail (data || !n_data, NULL);
	g_return_val_if_fail (n_decoded, NULL);
	
	if (n_data == -1)
		n_data = strlen (data);

	decoded = result = g_malloc0 ((n_data / 2) + 1);
	*n_decoded = 0;

	while (n_data > 0) {
    		if (!g_ascii_isspace (*data)) {
    			
	        	/* Find the position */
			pos = strchr (HEXC_UPPER, g_ascii_toupper (*data));
			if (pos == 0)
				break;

			j = pos - HEXC_UPPER;
			if(!state) {
				*decoded = (j & 0xf) << 4;
				state = 1;
			} else {      
				*decoded |= (j & 0xf);
				(*n_decoded)++;
				decoded++;
				state = 0;
			}
    		}
      
      		++data;
      		--n_data;
	}

	/* Parsing error */
	if (state != 0) {
		g_free (result);
		result = NULL;
	}

	return result;
}

gchar* 
egg_hex_encode (const guchar *data, gsize n_data)
{
	return egg_hex_encode_full (data, n_data, TRUE, '\0', 0);
}

gchar*
egg_hex_encode_full (const guchar *data, gsize n_data,
                     gboolean upper_case, gchar delim, guint group)
{
	GString *result;
	const char *hexc;
	gsize bytes;
	guchar j;
	
	g_return_val_if_fail (data || !n_data, NULL);
	
	hexc = upper_case ? HEXC_UPPER : HEXC_LOWER;

	result = g_string_sized_new (n_data * 2 + 1);
	bytes = 0;
	
	while (n_data > 0) {
		
		if (group && bytes && (bytes % group) == 0)
			g_string_append_c (result, delim);

		j = *(data) >> 4 & 0xf;
		g_string_append_c (result, hexc[j]);
		
		j = *(data++) & 0xf;
		g_string_append_c (result, hexc[j]);
    
		++bytes;
		--n_data;
	}

	/* Make sure still null terminated */
	return g_string_free (result, FALSE);
}

