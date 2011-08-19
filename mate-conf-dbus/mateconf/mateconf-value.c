/* MateConf
 * Copyright (C) 1999, 2000, 2002 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "mateconf-value.h"
#include "mateconf-error.h"
#include "mateconf-schema.h"
#include "mateconf-internals.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
  MateConfValueType type;
  union {
    gchar* string_data;
    gint int_data;
    gboolean bool_data;
    gdouble float_data;
    MateConfSchema* schema_data;
    struct {
      MateConfValueType type;
      GSList* list;
    } list_data;
    struct {
      MateConfValue* car;
      MateConfValue* cdr;
    } pair_data;
  } d;
} MateConfRealValue;

#define REAL_VALUE(x) ((MateConfRealValue*)(x))

static void
set_string(gchar** dest, const gchar* src)
{
  if (*dest != NULL)
    g_free(*dest);

  *dest = src ? g_strdup(src) : NULL;
}

/*
 * Values
 */

MateConfValue* 
mateconf_value_new(MateConfValueType type)
{
  MateConfValue* value;
  static gboolean initted = FALSE;
  
  g_return_val_if_fail(MATECONF_VALUE_TYPE_VALID(type), NULL);

  if (!initted)
    {
      _mateconf_init_i18n ();
      initted = TRUE;
    }
  
  value = (MateConfValue*) g_slice_new0 (MateConfRealValue);

  value->type = type;

  /* the g_new0() is important: sets list type to invalid, NULLs all
   * pointers
   */
  
  return value;
}

MateConfValue* 
mateconf_value_new_from_string(MateConfValueType type, const gchar* value_str,
                             GError** err)
{
  MateConfValue* value;

  value = mateconf_value_new(type);

  switch (type)
    {
    case MATECONF_VALUE_INT:
      {
        char* endptr = NULL;
        glong result;

        errno = 0;
        result = strtol(value_str, &endptr, 10);

        if (endptr == value_str)
          {
            if (err)
              *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                                      _("Didn't understand `%s' (expected integer)"),
                                      value_str);
            
            mateconf_value_free(value);
            value = NULL;
          }
        else if (errno == ERANGE)
          {
            if (err)
              *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                                      _("Integer `%s' is too large or small"),
                                      value_str);
            mateconf_value_free(value);
            value = NULL;
          }
        else
          mateconf_value_set_int(value, result);
      }
      break;
    case MATECONF_VALUE_FLOAT:
      {
        double num;

        if (mateconf_string_to_double(value_str, &num))
          {
            mateconf_value_set_float(value, num);
          }
        else
          {
            if (err)
              *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                                      _("Didn't understand `%s' (expected real number)"),
                                     value_str);
            
            mateconf_value_free(value);
            value = NULL;
          }
      }
      break;
    case MATECONF_VALUE_STRING:      
      if (!g_utf8_validate (value_str, -1, NULL))
        {
          g_set_error (err, MATECONF_ERROR,
                       MATECONF_ERROR_PARSE_ERROR,
                       _("Text contains invalid UTF-8"));
          mateconf_value_free(value);
          value = NULL;
        }
      else
        {
          mateconf_value_set_string(value, value_str);
        }
      break;
    case MATECONF_VALUE_BOOL:
      switch (*value_str)
        {
        case 't':
        case 'T':
        case '1':
        case 'y':
        case 'Y':
          mateconf_value_set_bool(value, TRUE);
          break;

        case 'f':
        case 'F':
        case '0':
        case 'n':
        case 'N':
          mateconf_value_set_bool(value, FALSE);
          break;
          
        default:
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                                   _("Didn't understand `%s' (expected true or false)"),
                                   value_str);
          
          mateconf_value_free(value);
          value = NULL;
          break;
        }
      break;
    case MATECONF_VALUE_LIST:
    case MATECONF_VALUE_PAIR:
    default:
      g_assert_not_reached();
      break;
    }

  return value;
}

static char *
escape_string(const char *str, const char *escaped_chars)
{
  gint i, j, len;
  gchar* ret;

  len = 0;
  for (i = 0; str[i] != '\0'; i++)
    {
      if (strchr(escaped_chars, str[i]) != NULL ||
	  str[i] == '\\')
	len++;
      len++;
    }
  
  ret = g_malloc(len + 1);

  j = 0;
  for (i = 0; str[i] != '\0'; i++)
    {
      if (strchr(escaped_chars, str[i]) != NULL ||
	  str[i] == '\\')
	{
	  ret[j++] = '\\';
	}
      ret[j++] = str[i];
    }
  ret[j++] = '\0';

  return ret;
}

MateConfValue*
mateconf_value_new_list_from_string(MateConfValueType list_type,
                                  const gchar* str,
				  GError** err)
{
  int i, len;
  gboolean escaped, pending_chars;
  GString *string;
  MateConfValue* value;
  GSList *list;

  g_return_val_if_fail(list_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_PAIR, NULL);

  if (!g_utf8_validate (str, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_PARSE_ERROR,
                   _("Text contains invalid UTF-8"));
      return NULL;
    }
  
  if (str[0] != '[')
    {
      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (list must start with a '[')"),
			       str);
      return NULL;
    }

  len = strlen(str);

  /* Note: by now len is sure to be 1 or larger, so len-1 will never be
   * negative */
  if (str[len-1] != ']')
    {
      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (list must end with a ']')"),
			       str);
      return NULL;
    }

  if (strstr(str, "[]"))
    {
      value = mateconf_value_new(MATECONF_VALUE_LIST);
      mateconf_value_set_list_type(value, list_type);

      return value;
    }

  escaped = FALSE;
  pending_chars = FALSE;
  list = NULL;
  string = g_string_new(NULL);

  for (i = 1; str[i] != '\0'; i++)
    {
      if ( ! escaped &&
	  (str[i] == ',' ||
	   str[i] == ']'))
	{
	  MateConfValue* val;
	  val = mateconf_value_new_from_string(list_type, string->str, err);

	  if (err && *err != NULL)
	    {
	      /* Free values so far */
	      g_slist_foreach(list, (GFunc)mateconf_value_free, NULL);
	      g_slist_free(list);

	      g_string_free(string, TRUE);

	      return NULL;
	    }

	  g_string_assign(string, "");
	  list = g_slist_prepend(list, val);
	  if (str[i] == ']' &&
	      i != len-1)
	    {
	      /* Free values so far */
	      g_slist_foreach(list, (GFunc)mateconf_value_free, NULL);
	      g_slist_free(list);

	      g_string_free(string, TRUE);

	      if (err)
		*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
				       _("Didn't understand `%s' (extra unescaped ']' found inside list)"),
				       str);
	      return NULL;
	    }
	  pending_chars = FALSE;
	}
      else if ( ! escaped && str[i] == '\\')
	{
	  escaped = TRUE;
	  pending_chars = TRUE;
	}
      else
	{
	  g_string_append_c(string, str[i]);
	  escaped = FALSE;
	  pending_chars = TRUE;
	}
    }

  g_string_free(string, TRUE);

  if (pending_chars)
    {
      /* Free values so far */
      g_slist_foreach(list, (GFunc)mateconf_value_free, NULL);
      g_slist_free(list);

      g_string_free(string, TRUE);

      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (extra trailing characters)"),
			       str);
      return NULL;
    }

  /* inverse list as we were prepending to it */
  list = g_slist_reverse(list);

  value = mateconf_value_new(MATECONF_VALUE_LIST);
  mateconf_value_set_list_type(value, list_type);

  mateconf_value_set_list_nocopy(value, list);

  return value;
}

MateConfValue*
mateconf_value_new_pair_from_string(MateConfValueType car_type,
                                  MateConfValueType cdr_type,
                                  const gchar* str,
				  GError** err)
{
  int i, len;
  int elem;
  gboolean escaped, pending_chars;
  GString *string;
  MateConfValue* value;
  MateConfValue* car;
  MateConfValue* cdr;

  g_return_val_if_fail(car_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(car_type != MATECONF_VALUE_PAIR, NULL);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_PAIR, NULL);

  if (!g_utf8_validate (str, -1, NULL))
    {
      g_set_error (err, MATECONF_ERROR,
                   MATECONF_ERROR_PARSE_ERROR,
                   _("Text contains invalid UTF-8"));
      return NULL;
    }
  
  if (str[0] != '(')
    {
      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (pair must start with a '(')"),
			       str);
      return NULL;
    }

  len = strlen(str);

  /* Note: by now len is sure to be 1 or larger, so len-1 will never be
   * negative */
  if (str[len-1] != ')')
    {
      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (pair must end with a ')')"),
			       str);
      return NULL;
    }

  escaped = FALSE;
  pending_chars = FALSE;
  car = cdr = NULL;
  string = g_string_new(NULL);
  elem = 0;

  for (i = 1; str[i] != '\0'; i++)
    {
      if ( ! escaped &&
	  (str[i] == ',' ||
	   str[i] == ')'))
	{
	  if ((str[i] == ')' && elem != 1) ||
	      (elem > 1))
	    {
	      /* Free values so far */
	      if (car)
	        mateconf_value_free(car);
	      if (cdr)
	        mateconf_value_free(cdr);

	      g_string_free(string, TRUE);

	      if (err)
		*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
				       _("Didn't understand `%s' (wrong number of elements)"),
				       str);
	      return NULL;
	    }

	  if (elem == 0)
	    car = mateconf_value_new_from_string(car_type, string->str, err);
	  else if (elem == 1)
	    cdr = mateconf_value_new_from_string(cdr_type, string->str, err);

	  elem ++;

	  if (err && *err != NULL)
	    {
	      /* Free values so far */
	      if (car)
	        mateconf_value_free(car);
	      if (cdr)
	        mateconf_value_free(cdr);

	      g_string_free(string, TRUE);

	      return NULL;
	    }

	  g_string_assign(string, "");

	  if (str[i] == ')' &&
	      i != len-1)
	    {
	      /* Free values so far */
	      if (car)
	        mateconf_value_free(car);
	      if (cdr)
	        mateconf_value_free(cdr);

	      g_string_free(string, TRUE);

	      if (err)
		*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
				       _("Didn't understand `%s' (extra unescaped ')' found inside pair)"),
				       str);
	      return NULL;
	    }
	  pending_chars = FALSE;
	}
      else if ( ! escaped && str[i] == '\\')
	{
	  escaped = TRUE;
	  pending_chars = TRUE;
	}
      else
	{
	  g_string_append_c(string, str[i]);
	  escaped = FALSE;
	  pending_chars = TRUE;
	}
    }

  g_string_free(string, TRUE);

  if (pending_chars)
    {
      /* Free values so far */
      if (car)
	mateconf_value_free(car);
      if (cdr)
	mateconf_value_free(cdr);

      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (extra trailing characters)"),
			       str);
      return NULL;
    }

  if (elem != 2)
    {
      /* Free values so far */
      if (car)
	mateconf_value_free(car);
      if (cdr)
	mateconf_value_free(cdr);

      if (err)
	*err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
			       _("Didn't understand `%s' (wrong number of elements)"),
			       str);
      return NULL;
    }

  value = mateconf_value_new(MATECONF_VALUE_PAIR);
  mateconf_value_set_car_nocopy(value, car);
  mateconf_value_set_cdr_nocopy(value, cdr);

  return value;
}

gchar*
mateconf_value_to_string(const MateConfValue* value)
{
  /* These strings shouldn't be translated; they're primarily 
     intended for machines to read, not humans, though I do
     use them in some debug spew
  */
  gchar* retval = NULL;

  switch (value->type)
    {
    case MATECONF_VALUE_INT:
      retval = g_strdup_printf("%d", mateconf_value_get_int(value));
      break;
    case MATECONF_VALUE_FLOAT:
      retval = mateconf_double_to_string(mateconf_value_get_float(value));
      break;
    case MATECONF_VALUE_STRING:
      retval = g_strdup(mateconf_value_get_string(value));
      break;
    case MATECONF_VALUE_BOOL:
      retval = mateconf_value_get_bool(value) ? g_strdup("true") : g_strdup("false");
      break;
    case MATECONF_VALUE_LIST:
      {
        GSList* list;

        list = mateconf_value_get_list(value);

        if (list == NULL)
          retval = g_strdup("[]");
        else
          {
            gchar* buf = NULL;
            guint bufsize = 64;
            guint cur = 0;

            g_assert(list != NULL);
            
            buf = g_malloc(bufsize+3); /* my +3 superstition */
            
            buf[0] = '[';
            ++cur;

            g_assert(cur < bufsize);
            
            while (list != NULL)
              {
                gchar* tmp;
                gchar* elem;
                guint len;
                
                tmp = mateconf_value_to_string((MateConfValue*)list->data);

                g_assert(tmp != NULL);

		elem = escape_string(tmp, ",]");

		g_free(tmp);

                len = strlen(elem);

                if ((cur + len + 2) >= bufsize) /* +2 for '\0' and comma */
                  {
                    bufsize = MAX(bufsize*2, bufsize+len+4); 
                    buf = g_realloc(buf, bufsize+3);
                  }

                g_assert(cur < bufsize);
                
                strcpy(&buf[cur], elem);
                cur += len;

                g_assert(cur < bufsize);
                
                g_free(elem);

                buf[cur] = ',';
                ++cur;

                g_assert(cur < bufsize);
                
                list = g_slist_next(list);
              }

            g_assert(cur < bufsize);
            
            buf[cur-1] = ']'; /* overwrites last comma */
            buf[cur] = '\0';

            retval = buf;
          }
      }
      break;
    case MATECONF_VALUE_PAIR:
      {
        gchar* tmp;
        gchar* car;
        gchar* cdr;

        if (mateconf_value_get_car (value))
          tmp = mateconf_value_to_string(mateconf_value_get_car(value));
        else
          tmp = g_strdup ("nil");
	car = escape_string(tmp, ",)");
	g_free(tmp);

        if (mateconf_value_get_cdr (value))
          tmp = mateconf_value_to_string(mateconf_value_get_cdr(value));
        else
          tmp = g_strdup ("nil");
	cdr = escape_string(tmp, ",)");
	g_free(tmp);
        retval = g_strdup_printf("(%s,%s)", car, cdr);
        g_free(car);
        g_free(cdr);
      }
      break;
      /* These remaining shouldn't really be used outside of debug spew... */
    case MATECONF_VALUE_INVALID:
      retval = g_strdup("Invalid");
      break;
    case MATECONF_VALUE_SCHEMA:
      {
        const gchar* locale;
        const gchar* type;
        const gchar* list_type;
        const gchar* car_type;
        const gchar* cdr_type;
        
        locale = mateconf_schema_get_locale(mateconf_value_get_schema(value));
        type = mateconf_value_type_to_string(mateconf_schema_get_type(mateconf_value_get_schema(value)));
        list_type = mateconf_value_type_to_string(mateconf_schema_get_list_type(mateconf_value_get_schema(value)));
        car_type = mateconf_value_type_to_string(mateconf_schema_get_car_type(mateconf_value_get_schema(value)));
        cdr_type = mateconf_value_type_to_string(mateconf_schema_get_cdr_type(mateconf_value_get_schema(value)));
        
        retval = g_strdup_printf("Schema (type: `%s' list_type: '%s' "
				 "car_type: '%s' cdr_type: '%s' locale: `%s')",
                                 type, list_type, car_type, cdr_type,
				 locale ? locale : "(null)");
      }
      break;
    default:
      g_assert_not_reached();
      break;
    }

  return retval;
}

static GSList*
copy_value_list(GSList* list)
{
  GSList* copy = NULL;
  GSList* tmp = list;
  
  while (tmp != NULL)
    {
      copy = g_slist_prepend(copy, mateconf_value_copy(tmp->data));
      
      tmp = g_slist_next(tmp);
    }
  
  copy = g_slist_reverse(copy);

  return copy;
}

MateConfValue* 
mateconf_value_copy (const MateConfValue* src)
{
  MateConfRealValue *dest;
  MateConfRealValue *real;
  
  g_return_val_if_fail(src != NULL, NULL);

  real = REAL_VALUE (src);
  dest = REAL_VALUE (mateconf_value_new (src->type));

  switch (real->type)
    {
    case MATECONF_VALUE_INT:
    case MATECONF_VALUE_FLOAT:
    case MATECONF_VALUE_BOOL:
    case MATECONF_VALUE_INVALID:
      dest->d = real->d;
      break;
    case MATECONF_VALUE_STRING:
      set_string(&dest->d.string_data, real->d.string_data);
      break;
    case MATECONF_VALUE_SCHEMA:
      if (real->d.schema_data)
        dest->d.schema_data = mateconf_schema_copy(real->d.schema_data);
      else
        dest->d.schema_data = NULL;
      break;
      
    case MATECONF_VALUE_LIST:
      {
        GSList* copy;

        copy = copy_value_list(real->d.list_data.list);

        dest->d.list_data.list = copy;
        dest->d.list_data.type = real->d.list_data.type;
      }
      break;
      
    case MATECONF_VALUE_PAIR:

      if (real->d.pair_data.car)
        dest->d.pair_data.car = mateconf_value_copy(real->d.pair_data.car);
      else
        dest->d.pair_data.car = NULL;

      if (real->d.pair_data.cdr)
        dest->d.pair_data.cdr = mateconf_value_copy(real->d.pair_data.cdr);
      else
        dest->d.pair_data.cdr = NULL;

      break;
      
    default:
      g_assert_not_reached();
    }
  
  return (MateConfValue*) dest;
}

static void
mateconf_value_free_list(MateConfValue* value)
{
  GSList* tmp;
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_LIST);

  real = REAL_VALUE (value);
  
  tmp = real->d.list_data.list;

  while (tmp != NULL)
    {
      mateconf_value_free(tmp->data);
      
      tmp = g_slist_next(tmp);
    }
  g_slist_free(real->d.list_data.list);

  real->d.list_data.list = NULL;
}

void 
mateconf_value_free(MateConfValue* value)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);

  real = REAL_VALUE (value);
  
  switch (real->type)
    {
    case MATECONF_VALUE_STRING:
      if (real->d.string_data != NULL)
        g_free(real->d.string_data);
      break;
    case MATECONF_VALUE_SCHEMA:
      if (real->d.schema_data != NULL)
        mateconf_schema_free(real->d.schema_data);
      break;
    case MATECONF_VALUE_LIST:
      mateconf_value_free_list(value);
      break;
    case MATECONF_VALUE_PAIR:
      if (real->d.pair_data.car != NULL)
        mateconf_value_free(real->d.pair_data.car);

      if (real->d.pair_data.cdr != NULL)
        mateconf_value_free(real->d.pair_data.cdr);
      break;
    default:
      break;
    }
  
  g_slice_free(MateConfRealValue, real);
}

const char*
mateconf_value_get_string (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_STRING, NULL);
  
  return REAL_VALUE (value)->d.string_data;
}

char*
mateconf_value_steal_string (MateConfValue *value)
{
  char *string;
  MateConfRealValue *real;
  
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_STRING, NULL);

  real = REAL_VALUE (value);

  string = real->d.string_data;
  real->d.string_data = NULL;

  return string;
}

int
mateconf_value_get_int (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->type == MATECONF_VALUE_INT, 0);
  
  return REAL_VALUE (value)->d.int_data;
}

double
mateconf_value_get_float (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, 0.0);
  g_return_val_if_fail (value->type == MATECONF_VALUE_FLOAT, 0.0);
  
  return REAL_VALUE (value)->d.float_data;
}

MateConfValueType
mateconf_value_get_list_type (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, MATECONF_VALUE_INVALID);
  g_return_val_if_fail (value->type == MATECONF_VALUE_LIST, MATECONF_VALUE_INVALID);
  
  return REAL_VALUE (value)->d.list_data.type;
}

GSList*
mateconf_value_get_list (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_LIST, NULL);

  return REAL_VALUE (value)->d.list_data.list;
}

GSList*
mateconf_value_steal_list (MateConfValue *value)
{
  GSList *list;
  MateConfRealValue *real;
  
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_LIST, NULL);

  real = REAL_VALUE (value);

  list = real->d.list_data.list;
  real->d.list_data.list = NULL;
  return list;
}

MateConfValue*
mateconf_value_get_car (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_PAIR, NULL);
  
  return REAL_VALUE (value)->d.pair_data.car;
}

MateConfValue*
mateconf_value_get_cdr (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_PAIR, NULL);
  
  return REAL_VALUE (value)->d.pair_data.cdr;
}


gboolean
mateconf_value_get_bool (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, FALSE);
  g_return_val_if_fail (value->type == MATECONF_VALUE_BOOL, FALSE);
  
  return REAL_VALUE (value)->d.bool_data;
}

MateConfSchema*
mateconf_value_get_schema (const MateConfValue *value)
{
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_SCHEMA, NULL);
  
  return REAL_VALUE (value)->d.schema_data;
}

MateConfSchema*
mateconf_value_steal_schema (MateConfValue *value)
{
  MateConfSchema *schema;
  MateConfRealValue *real;
  
  g_return_val_if_fail (value != NULL, NULL);
  g_return_val_if_fail (value->type == MATECONF_VALUE_SCHEMA, NULL);

  real = REAL_VALUE (value);

  schema = real->d.schema_data;
  real->d.schema_data = NULL;

  return schema;
}

void        
mateconf_value_set_int(MateConfValue* value, gint the_int)
{
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_INT);

  REAL_VALUE (value)->d.int_data = the_int;
}

void        
mateconf_value_set_string(MateConfValue* value, const gchar* the_str)
{  
  mateconf_value_set_string_nocopy (value,
                                 g_strdup (the_str));
}

void
mateconf_value_set_string_nocopy (MateConfValue *value,
                               char       *str)
{
  MateConfRealValue *real;

  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_STRING);

  real = REAL_VALUE (value);

  g_free (real->d.string_data);
  real->d.string_data = str;
}

void        
mateconf_value_set_float(MateConfValue* value, gdouble the_float)
{
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_FLOAT);

  REAL_VALUE (value)->d.float_data = the_float;
}

void        
mateconf_value_set_bool(MateConfValue* value, gboolean the_bool)
{
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_BOOL);

  REAL_VALUE (value)->d.bool_data = the_bool;
}

void       
mateconf_value_set_schema(MateConfValue* value, const MateConfSchema* sc)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_SCHEMA);

  real = REAL_VALUE (value);
  
  if (real->d.schema_data != NULL)
    mateconf_schema_free (real->d.schema_data);

  real->d.schema_data = mateconf_schema_copy (sc);
}

void        
mateconf_value_set_schema_nocopy(MateConfValue* value, MateConfSchema* sc)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_SCHEMA);
  g_return_if_fail(sc != NULL);

  real = REAL_VALUE (value);
  
  if (real->d.schema_data != NULL)
    mateconf_schema_free (real->d.schema_data);

  real->d.schema_data = sc;
}

void
mateconf_value_set_car(MateConfValue* value, const MateConfValue* car)
{
  mateconf_value_set_car_nocopy(value, mateconf_value_copy(car));
}

void
mateconf_value_set_car_nocopy(MateConfValue* value, MateConfValue* car)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_PAIR);

  real = REAL_VALUE (value);
  
  if (real->d.pair_data.car != NULL)
    mateconf_value_free (real->d.pair_data.car);

  real->d.pair_data.car = car;
}

void
mateconf_value_set_cdr(MateConfValue* value, const MateConfValue* cdr)
{
  mateconf_value_set_cdr_nocopy(value, mateconf_value_copy(cdr));
}

void
mateconf_value_set_cdr_nocopy(MateConfValue* value, MateConfValue* cdr)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_PAIR);

  real = REAL_VALUE (value);
  
  if (real->d.pair_data.cdr != NULL)
    mateconf_value_free (real->d.pair_data.cdr);

  real->d.pair_data.cdr = cdr;
}

void
mateconf_value_set_list_type (MateConfValue    *value,
                           MateConfValueType type)
{
  MateConfRealValue *real;
  
  g_return_if_fail(value != NULL);
  g_return_if_fail(value->type == MATECONF_VALUE_LIST);
  g_return_if_fail(type != MATECONF_VALUE_LIST);
  g_return_if_fail(type != MATECONF_VALUE_PAIR);

  real = REAL_VALUE (value);
  
  /* If the list is non-NULL either we already have the right
   * type, or we shouldn't be changing it without deleting
   * the list first.
   */
  g_return_if_fail (real->d.list_data.list == NULL);

  real->d.list_data.type = type;
}

void
mateconf_value_set_list_nocopy (MateConfValue* value,
                             GSList* list)
{
  MateConfRealValue *real;
  
  g_return_if_fail (value != NULL);
  g_return_if_fail (value->type == MATECONF_VALUE_LIST);

  real = REAL_VALUE (value);
  
  g_return_if_fail (real->d.list_data.type != MATECONF_VALUE_INVALID);
  
  if (real->d.list_data.list)
    mateconf_value_free_list (value);

  real->d.list_data.list = list;
}

void
mateconf_value_set_list       (MateConfValue* value,
                            GSList* list)
{
  MateConfRealValue *real;
  
  g_return_if_fail (value != NULL);
  g_return_if_fail (value->type == MATECONF_VALUE_LIST);

  real = REAL_VALUE (value);

  g_return_if_fail (real->d.list_data.type != MATECONF_VALUE_INVALID);
  g_return_if_fail ((list == NULL) ||
                    ((list->data != NULL) &&
                     (((MateConfValue*)list->data)->type == real->d.list_data.type)));
  
  if (real->d.list_data.list)
    mateconf_value_free_list (value);

  real->d.list_data.list = copy_value_list (list);
}


static int
null_safe_strcmp (const char *lhs,
                  const char *rhs)
{
  if (lhs == NULL && rhs == NULL)
    return 0;
  else if (lhs == NULL)
    return -1;
  else if (rhs == NULL)
    return 1;
  else
    return strcmp (lhs, rhs);
}

int
mateconf_value_compare (const MateConfValue *value_a,
                     const MateConfValue *value_b)
{
  g_return_val_if_fail (value_a != NULL, 0);
  g_return_val_if_fail (value_b != NULL, 0);

  /* Impose arbitrary type ordering, just to keep the
   * sort invariants stable.
   */
  if (value_a->type < value_b->type)
    return -1;
  else if (value_a->type > value_b->type)
    return 1;
  
  switch (value_a->type)
    {
    case MATECONF_VALUE_INT:
      if (mateconf_value_get_int (value_a) < mateconf_value_get_int (value_b))
        return -1;
      else if (mateconf_value_get_int (value_a) > mateconf_value_get_int (value_b))
        return 1;
      else
        return 0;
    case MATECONF_VALUE_FLOAT:
      if (mateconf_value_get_float (value_a) < mateconf_value_get_float (value_b))
        return -1;
      else if (mateconf_value_get_float (value_a) > mateconf_value_get_float (value_b))
        return 1;
      else
        return 0;
    case MATECONF_VALUE_STRING:
      return strcmp (mateconf_value_get_string (value_a),
                     mateconf_value_get_string (value_b));
    case MATECONF_VALUE_BOOL:
      if (mateconf_value_get_bool (value_a) == mateconf_value_get_bool (value_b))
        return 0;
      /* make TRUE > FALSE to maintain sort invariants */
      else if (mateconf_value_get_bool (value_a))
        return 1;
      else
        return -1;
    case MATECONF_VALUE_LIST:
      {
        GSList *list_a;
        GSList *list_b;

        list_a = mateconf_value_get_list (value_a);
        list_b = mateconf_value_get_list (value_b);
        
        while (list_a != NULL && list_b != NULL)
          {
            int result;

            result = mateconf_value_compare (list_a->data, list_b->data);

            if (result != 0)
              return result;
            
            list_a = g_slist_next (list_a);
            list_b = g_slist_next (list_b);
          }
        
        if (list_a)
          return 1; /* list_a is longer so "greater" */
        else if (list_b)
          return -1;
        else
          return 0;
      }
    case MATECONF_VALUE_PAIR:
      {
        MateConfValue *a_car, *b_car, *a_cdr, *b_cdr;
        int result;
        
        a_car = mateconf_value_get_car (value_a);
        b_car = mateconf_value_get_car (value_b);
        a_cdr = mateconf_value_get_cdr (value_a);
        b_cdr = mateconf_value_get_cdr (value_b);

        if (a_car == NULL && b_car != NULL)
          return -1;
        else if (a_car != NULL && b_car == NULL)
          return 1;
        else if (a_car != NULL && b_car != NULL)
          {
            result = mateconf_value_compare (a_car, b_car);

            if (result != 0)
              return result;
          }

        if (a_cdr == NULL && b_cdr != NULL)
          return -1;
        else if (a_cdr != NULL && b_cdr == NULL)
          return 1;
        else if (a_cdr != NULL && b_cdr != NULL)
          {
            result = mateconf_value_compare (a_cdr, b_cdr);

            if (result != 0)
              return result;
          }

        return 0;
      }
    case MATECONF_VALUE_INVALID:
      return 0;
    case MATECONF_VALUE_SCHEMA:
      {
        const char *locale_a, *locale_b;
        MateConfValueType type_a, type_b;
        MateConfValueType list_type_a, list_type_b;
        MateConfValueType car_type_a, car_type_b;
        MateConfValueType cdr_type_a, cdr_type_b;
        const char *short_desc_a, *short_desc_b;
        const char *long_desc_a, *long_desc_b;
        int result;
        
        type_a = mateconf_schema_get_type (mateconf_value_get_schema (value_a));
        type_b = mateconf_schema_get_type (mateconf_value_get_schema (value_b));

        if (type_a < type_b)
          return -1;
        else if (type_a > type_b)
          return 1;

        short_desc_a = mateconf_schema_get_short_desc (mateconf_value_get_schema (value_a));
        short_desc_b = mateconf_schema_get_short_desc (mateconf_value_get_schema (value_b));

        result = null_safe_strcmp (short_desc_a, short_desc_b);
        if (result != 0)
          return result;
        
        long_desc_a = mateconf_schema_get_long_desc (mateconf_value_get_schema (value_a));


        long_desc_b = mateconf_schema_get_long_desc (mateconf_value_get_schema (value_b));

        result = null_safe_strcmp (long_desc_a, long_desc_b);
        if (result != 0)
          return result;
        
        locale_a = mateconf_schema_get_locale (mateconf_value_get_schema (value_a));
        locale_b = mateconf_schema_get_locale (mateconf_value_get_schema (value_b));

        result = null_safe_strcmp (locale_a, locale_b);
        if (result != 0)
          return result;        

        if (type_a == MATECONF_VALUE_LIST)
          {
            list_type_a = mateconf_schema_get_list_type (mateconf_value_get_schema (value_a));
            list_type_b = mateconf_schema_get_list_type (mateconf_value_get_schema (value_b));
            
            if (list_type_a < list_type_b)
              return -1;
            else if (list_type_a > list_type_b)
              return 1;
          }

        if (type_a == MATECONF_VALUE_PAIR)
          {
            car_type_a = mateconf_schema_get_car_type (mateconf_value_get_schema (value_a));
            car_type_b = mateconf_schema_get_car_type (mateconf_value_get_schema (value_b));
            
            if (car_type_a < car_type_b)
              return -1;
            else if (car_type_a > car_type_b)
              return 1;
            
            cdr_type_a = mateconf_schema_get_cdr_type (mateconf_value_get_schema (value_a));
            cdr_type_b = mateconf_schema_get_cdr_type (mateconf_value_get_schema (value_b));
            
            if (cdr_type_a < cdr_type_b)
              return -1;
            else if (cdr_type_a > cdr_type_b)
              return 1;
          }

        return 0;
      }
    }

  g_assert_not_reached ();

  return 0;
}

/*
 * MateConfMetaInfo
 */

MateConfMetaInfo*
mateconf_meta_info_new(void)
{
  MateConfMetaInfo* gcmi;

  gcmi = g_new0(MateConfMetaInfo, 1);

  /* pointers and time are NULL/0 */
  
  return gcmi;
}

void
mateconf_meta_info_free(MateConfMetaInfo* gcmi)
{
  g_free(gcmi->schema);
  g_free(gcmi->mod_user);
  g_free(gcmi);
}

const char*
mateconf_meta_info_get_schema (MateConfMetaInfo *gcmi)
{
  g_return_val_if_fail (gcmi != NULL, NULL);

  return gcmi->schema;
}

const char*
mateconf_meta_info_get_mod_user (MateConfMetaInfo *gcmi)
{
  g_return_val_if_fail (gcmi != NULL, NULL);

  return gcmi->mod_user;
}

GTime
mateconf_meta_info_mod_time (MateConfMetaInfo *gcmi)
{
  g_return_val_if_fail (gcmi != NULL, 0);
  
  return gcmi->mod_time;
}

void
mateconf_meta_info_set_schema  (MateConfMetaInfo* gcmi,
                              const gchar* schema_name)
{
  set_string(&gcmi->schema, schema_name);
}

void
mateconf_meta_info_set_mod_user(MateConfMetaInfo* gcmi,
                              const gchar* mod_user)
{
  set_string(&gcmi->mod_user, mod_user);
}

void
mateconf_meta_info_set_mod_time(MateConfMetaInfo* gcmi,
                              GTime mod_time)
{
  gcmi->mod_time = mod_time;
}

/*
 * MateConfEntry
 */

typedef struct {
  char *key;
  MateConfValue *value;
  char *schema_name;
  int refcount;
  guint is_default : 1;
  guint is_writable : 1;
} MateConfRealEntry;

#define REAL_ENTRY(x) ((MateConfRealEntry*)(x))

MateConfEntry*
mateconf_entry_new (const char *key,
                 const MateConfValue  *val)
{
  return mateconf_entry_new_nocopy (g_strdup (key),
                                 val ? mateconf_value_copy (val) : NULL);

}

MateConfEntry* 
mateconf_entry_new_nocopy (char* key, MateConfValue* val)
{
  MateConfRealEntry* real;

  real = g_slice_new (MateConfRealEntry);

  real->key   = key;
  real->value = val;
  real->schema_name = NULL;
  real->is_default = FALSE;
  real->is_writable = TRUE;
  real->refcount = 1;
  
  return (MateConfEntry*) real;
}

void
mateconf_entry_ref (MateConfEntry *entry)
{
  g_return_if_fail (entry != NULL);
  
  REAL_ENTRY (entry)->refcount += 1;
}

void
mateconf_entry_unref (MateConfEntry *entry)
{
  MateConfRealEntry *real;
  
  g_return_if_fail (entry != NULL);
  g_return_if_fail (REAL_ENTRY (entry)->refcount > 0);

  real = REAL_ENTRY (entry);
  
  real->refcount -= 1;

  if (real->refcount == 0)
    {
      g_free (real->key);
      if (real->value)
        mateconf_value_free (real->value);
      if (real->schema_name)
        g_free (real->schema_name);
      g_slice_free (MateConfRealEntry, real);
    }
}

void
mateconf_entry_free (MateConfEntry *entry)
{
  mateconf_entry_unref (entry);
}

MateConfEntry*
mateconf_entry_copy (const MateConfEntry *src)
{
  MateConfEntry *entry;
  MateConfRealEntry *real;
  
  entry = mateconf_entry_new (REAL_ENTRY (src)->key,
                           REAL_ENTRY (src)->value);
  real = REAL_ENTRY (entry);
  
  real->schema_name = g_strdup (REAL_ENTRY (src)->schema_name);
  real->is_default = REAL_ENTRY (src)->is_default;
  real->is_writable = REAL_ENTRY (src)->is_writable;

  return entry;
}

gboolean
mateconf_entry_equal (const MateConfEntry *a,
                   const MateConfEntry *b)
{
  MateConfRealEntry *real_a;
  MateConfRealEntry *real_b;
  
  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);

  real_a = REAL_ENTRY (a);
  real_b = REAL_ENTRY (b);
  
  /* do the cheap checks first, why not */
  if (real_a->value && !real_b->value)
    return FALSE;
  else if (!real_a->value && real_b->value)
    return FALSE;
  else if (real_a->is_default != real_b->is_default)
    return FALSE;
  else if (real_a->is_writable != real_b->is_writable)
    return FALSE;
  else if (strcmp (real_a->key, real_b->key) != 0)
    return FALSE;
  else if (real_a->schema_name && !real_b->schema_name)
    return FALSE;
  else if (!real_a->schema_name && real_b->schema_name)
    return FALSE;
  else if (real_a->schema_name && real_b->schema_name &&
           strcmp (real_a->schema_name, real_b->schema_name) != 0)
    return FALSE;
  else if (real_a->value && real_b->value &&
           mateconf_value_compare (real_a->value, real_b->value) != 0)
    return FALSE;
  else
    return TRUE;
}

MateConfValue*
mateconf_entry_steal_value (MateConfEntry* entry)
{
  MateConfValue* val = REAL_ENTRY (entry)->value;
  REAL_ENTRY (entry)->value = NULL;
  return val;
}

const char*
mateconf_entry_get_key (const MateConfEntry *entry)
{
  g_return_val_if_fail (entry != NULL, NULL);

  return REAL_ENTRY (entry)->key;
}

MateConfValue*
mateconf_entry_get_value (const MateConfEntry *entry)
{
  g_return_val_if_fail (entry != NULL, NULL);

  return REAL_ENTRY (entry)->value;
}

const char*
mateconf_entry_get_schema_name (const MateConfEntry *entry)
{
  g_return_val_if_fail (entry != NULL, NULL);

  return REAL_ENTRY (entry)->schema_name;
}

gboolean
mateconf_entry_get_is_default  (const MateConfEntry *entry)
{
  g_return_val_if_fail (entry != NULL, FALSE);

  return REAL_ENTRY (entry)->is_default;
}

gboolean
mateconf_entry_get_is_writable (const MateConfEntry *entry)
{
  g_return_val_if_fail (entry != NULL, FALSE);

  return REAL_ENTRY (entry)->is_writable;
}


void
mateconf_entry_set_value (MateConfEntry  *entry,
                       const MateConfValue  *val)
{
  mateconf_entry_set_value_nocopy (entry,
                                val ? mateconf_value_copy (val) : NULL);
}

void
mateconf_entry_set_value_nocopy(MateConfEntry* entry,
                             MateConfValue* val)
{
  if (REAL_ENTRY (entry)->value)
    mateconf_value_free (REAL_ENTRY (entry)->value);

  REAL_ENTRY (entry)->value = val;
}

void
mateconf_entry_set_schema_name(MateConfEntry* entry,
                            const gchar* name)
{
  if (REAL_ENTRY (entry)->schema_name)
    g_free (REAL_ENTRY (entry)->schema_name);

  REAL_ENTRY (entry)->schema_name = name ? g_strdup(name) : NULL;
}

void
mateconf_entry_set_is_default (MateConfEntry* entry,
                            gboolean is_default)
{
  REAL_ENTRY (entry)->is_default = is_default;
}

void
mateconf_entry_set_is_writable (MateConfEntry  *entry,
                             gboolean     is_writable)
{
  REAL_ENTRY (entry)->is_writable = is_writable;
}


gboolean
mateconf_value_validate (const MateConfValue *value,
                      GError          **err)
{
  MateConfRealValue *real;

  g_return_val_if_fail (value != NULL, FALSE);

  real = REAL_VALUE (value);
  
  switch (value->type)
    {
    case MATECONF_VALUE_STRING:
      if (real->d.string_data &&
          !g_utf8_validate (real->d.string_data, -1, NULL))
        {
          g_set_error (err, MATECONF_ERROR,
                       MATECONF_ERROR_FAILED,
                       _("Text contains invalid UTF-8"));
          return FALSE;
        }
      break;

    case MATECONF_VALUE_SCHEMA:
      if (real->d.schema_data)
        return mateconf_schema_validate (real->d.schema_data,
                                      err);
      break;

    default:
      break;
    }

  return TRUE;
}
