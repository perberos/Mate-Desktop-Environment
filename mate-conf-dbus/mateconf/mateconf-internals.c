
/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
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

#include <config.h>
#include "mateconf-internals.h"
#include "mateconf-backend.h"
#include "mateconf-schema.h"
#include "mateconf.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>
#include <math.h>

#ifdef G_OS_WIN32
#include <windows.h>
#include <share.h>
#endif

gboolean mateconf_log_debug_messages = FALSE;

static gboolean mateconf_daemon_mode = FALSE;
static gchar* daemon_ior = NULL;

void
mateconf_set_daemon_mode(gboolean setting)
{
  mateconf_daemon_mode = setting;
}

gboolean
mateconf_in_daemon_mode(void)
{
  return mateconf_daemon_mode;
}

void
mateconf_set_daemon_ior(const gchar* ior)
{
  if (daemon_ior != NULL)
    {
      g_free(daemon_ior);
      daemon_ior = NULL;
    }
      
  if (ior != NULL)
    daemon_ior = g_strdup(ior);
}

const gchar*
mateconf_get_daemon_ior(void)
{
  return daemon_ior;
}

gchar*
mateconf_key_directory  (const gchar* key)
{
  const gchar* end;
  gchar* retval;
  int len;

  end = strrchr(key, '/');

  if (end == NULL)
    {
      mateconf_log(GCL_ERR, _("No '/' in key \"%s\""), key);
      return NULL;
    }

  len = end-key+1;

  if (len == 1)
    {
      /* Root directory */
      retval = g_strdup("/");
    }
  else 
    {
      retval = g_malloc(len);
      
      strncpy(retval, key, len);
      
      retval[len-1] = '\0';
    }

  return retval;
}

const gchar*
mateconf_key_key        (const gchar* key)
{
  const gchar* end;
  
  end = strrchr(key, '/');

  ++end;

  return end;
}

/*
 *  Random stuff 
 */

gboolean
mateconf_file_exists (const gchar* filename)
{
  struct stat s;
  
  g_return_val_if_fail (filename != NULL,FALSE);
  
  return g_stat (filename, &s) == 0;
}

gboolean
mateconf_file_test(const gchar* filename, int test)
{
  struct stat s;
  if(g_stat (filename, &s) != 0)
    return FALSE;
  if(!(test & MATECONF_FILE_ISFILE) && S_ISREG(s.st_mode))
    return FALSE;
  if(!(test & MATECONF_FILE_ISLINK) && S_ISLNK(s.st_mode))
    return FALSE;
  if(!(test & MATECONF_FILE_ISDIR) && S_ISDIR(s.st_mode))
    return FALSE;
  return TRUE;
}

#if HAVE_CORBA
MateConfValue* 
mateconf_value_from_corba_value(const ConfigValue* value)
{
  MateConfValue* gval;
  MateConfValueType type = MATECONF_VALUE_INVALID;
  
  switch (value->_d)
    {
    case InvalidVal:
      return NULL;
    case IntVal:
      type = MATECONF_VALUE_INT;
      break;
    case StringVal:
      type = MATECONF_VALUE_STRING;
      break;
    case FloatVal:
      type = MATECONF_VALUE_FLOAT;
      break;
    case BoolVal:
      type = MATECONF_VALUE_BOOL;
      break;
    case SchemaVal:
      type = MATECONF_VALUE_SCHEMA;
      break;
    case ListVal:
      type = MATECONF_VALUE_LIST;
      break;
    case PairVal:
      type = MATECONF_VALUE_PAIR;
      break;
    default:
      mateconf_log(GCL_DEBUG, "Invalid type in %s", G_GNUC_FUNCTION);
      return NULL;
    }

  g_assert(MATECONF_VALUE_TYPE_VALID(type));
  
  gval = mateconf_value_new(type);

  switch (gval->type)
    {
    case MATECONF_VALUE_INT:
      mateconf_value_set_int(gval, value->_u.int_value);
      break;
    case MATECONF_VALUE_STRING:
      if (!g_utf8_validate (value->_u.string_value, -1, NULL))
        {
          mateconf_log (GCL_ERR, _("Invalid UTF-8 in string value in '%s'"),
                     value->_u.string_value); 
        }
      else
        {
          mateconf_value_set_string(gval, value->_u.string_value);
        }
      break;
    case MATECONF_VALUE_FLOAT:
      mateconf_value_set_float(gval, value->_u.float_value);
      break;
    case MATECONF_VALUE_BOOL:
      mateconf_value_set_bool(gval, value->_u.bool_value);
      break;
    case MATECONF_VALUE_SCHEMA:
      mateconf_value_set_schema_nocopy(gval, 
                                    mateconf_schema_from_corba_schema(&(value->_u.schema_value)));
      break;
    case MATECONF_VALUE_LIST:
      {
        GSList* list = NULL;
        guint i = 0;
        
        switch (value->_u.list_value.list_type)
          {
          case BIntVal:
            mateconf_value_set_list_type(gval, MATECONF_VALUE_INT);
            break;
          case BBoolVal:
            mateconf_value_set_list_type(gval, MATECONF_VALUE_BOOL);
            break;
          case BFloatVal:
            mateconf_value_set_list_type(gval, MATECONF_VALUE_FLOAT);
            break;
          case BStringVal:
            mateconf_value_set_list_type(gval, MATECONF_VALUE_STRING);
            break;
          case BInvalidVal:
            break;
          default:
            g_warning("Bizarre list type in %s", G_GNUC_FUNCTION);
            break;
          }

        if (mateconf_value_get_list_type(gval) != MATECONF_VALUE_INVALID)
          {
            i = 0;
            while (i < value->_u.list_value.seq._length)
              {
                MateConfValue* val;
                
                /* This is a bit dubious; we cast a ConfigBasicValue to ConfigValue
                   because they have the same initial members, but by the time
                   the CORBA and C specs kick in, not sure we are guaranteed
                   to be able to do this.
                */
                val = mateconf_value_from_corba_value((ConfigValue*)&value->_u.list_value.seq._buffer[i]);
                
                if (val == NULL)
                  mateconf_log(GCL_ERR, _("Couldn't interpret CORBA value for list element"));
                else if (val->type != mateconf_value_get_list_type(gval))
                  mateconf_log(GCL_ERR, _("Incorrect type for list element in %s"), G_GNUC_FUNCTION);
                else
                  list = g_slist_prepend(list, val);
                
                ++i;
              }
        
            list = g_slist_reverse(list);
            
            mateconf_value_set_list_nocopy(gval, list);
          }
        else
          {
            mateconf_log(GCL_ERR, _("Received list from mateconfd with a bad list type"));
          }
      }
      break;
    case MATECONF_VALUE_PAIR:
      {
        g_return_val_if_fail(value->_u.pair_value._length == 2, gval);
        
        mateconf_value_set_car_nocopy(gval,
                                   mateconf_value_from_corba_value((ConfigValue*)&value->_u.list_value.seq._buffer[0]));

        mateconf_value_set_cdr_nocopy(gval,
                                   mateconf_value_from_corba_value((ConfigValue*)&value->_u.list_value.seq._buffer[1]));
      }
      break;
    default:
      g_assert_not_reached();
      break;
    }
  
  return gval;
}

void          
mateconf_fill_corba_value_from_mateconf_value(const MateConfValue *value, 
                                        ConfigValue      *cv)
{
  if (value == NULL)
    {
      cv->_d = InvalidVal;
      return;
    }

  switch (value->type)
    {
    case MATECONF_VALUE_INT:
      cv->_d = IntVal;
      cv->_u.int_value = mateconf_value_get_int(value);
      break;
    case MATECONF_VALUE_STRING:
      cv->_d = StringVal;
      cv->_u.string_value = CORBA_string_dup((char*)mateconf_value_get_string(value));
      break;
    case MATECONF_VALUE_FLOAT:
      cv->_d = FloatVal;
      cv->_u.float_value = mateconf_value_get_float(value);
      break;
    case MATECONF_VALUE_BOOL:
      cv->_d = BoolVal;
      cv->_u.bool_value = mateconf_value_get_bool(value);
      break;
    case MATECONF_VALUE_SCHEMA:
      cv->_d = SchemaVal;
      mateconf_fill_corba_schema_from_mateconf_schema (mateconf_value_get_schema(value),
                                                 &cv->_u.schema_value);
      break;
    case MATECONF_VALUE_LIST:
      {
        guint n, i;
        GSList* list;
        
        cv->_d = ListVal;

        list = mateconf_value_get_list(value);

        n = g_slist_length(list);

        cv->_u.list_value.seq._buffer =
          CORBA_sequence_ConfigBasicValue_allocbuf(n);
        cv->_u.list_value.seq._length = n;
        cv->_u.list_value.seq._maximum = n;
        CORBA_sequence_set_release(&cv->_u.list_value.seq, TRUE);
        
        switch (mateconf_value_get_list_type(value))
          {
          case MATECONF_VALUE_INT:
            cv->_u.list_value.list_type = BIntVal;
            break;

          case MATECONF_VALUE_BOOL:
            cv->_u.list_value.list_type = BBoolVal;
            break;
            
          case MATECONF_VALUE_STRING:
            cv->_u.list_value.list_type = BStringVal;
            break;

          case MATECONF_VALUE_FLOAT:
            cv->_u.list_value.list_type = BFloatVal;
            break;

          case MATECONF_VALUE_SCHEMA:
            cv->_u.list_value.list_type = BSchemaVal;
            break;
            
          default:
            cv->_u.list_value.list_type = BInvalidVal;
            mateconf_log(GCL_DEBUG, "Invalid list type in %s", G_GNUC_FUNCTION);
            break;
          }
        
        i= 0;
        while (list != NULL)
          {
            /* That dubious ConfigBasicValue->ConfigValue cast again */
            mateconf_fill_corba_value_from_mateconf_value((MateConfValue*)list->data,
                                                    (ConfigValue*)&cv->_u.list_value.seq._buffer[i]);

            list = g_slist_next(list);
            ++i;
          }
      }
      break;
    case MATECONF_VALUE_PAIR:
      {
        cv->_d = PairVal;

        cv->_u.pair_value._buffer =
          CORBA_sequence_ConfigBasicValue_allocbuf(2);
        cv->_u.pair_value._length = 2;
        cv->_u.pair_value._maximum = 2;
        CORBA_sequence_set_release(&cv->_u.pair_value, TRUE);
        
        /* dubious cast */
        mateconf_fill_corba_value_from_mateconf_value (mateconf_value_get_car(value),
                                                 (ConfigValue*)&cv->_u.pair_value._buffer[0]);
        mateconf_fill_corba_value_from_mateconf_value(mateconf_value_get_cdr(value),
                                                (ConfigValue*)&cv->_u.pair_value._buffer[1]);
      }
      break;
      
    case MATECONF_VALUE_INVALID:
      cv->_d = InvalidVal;
      break;
    default:
      cv->_d = InvalidVal;
      mateconf_log(GCL_DEBUG, "Unknown type in %s", G_GNUC_FUNCTION);
      break;
    }
}

ConfigValue*  
mateconf_corba_value_from_mateconf_value (const MateConfValue* value)
{
  ConfigValue* cv;

  cv = ConfigValue__alloc();

  mateconf_fill_corba_value_from_mateconf_value(value, cv);

  return cv;
}

ConfigValue*  
mateconf_invalid_corba_value (void)
{
  ConfigValue* cv;

  cv = ConfigValue__alloc();

  cv->_d = InvalidVal;

  return cv;
}

gchar*
mateconf_object_to_string (CORBA_Object obj,
                        GError **err)
{
  CORBA_Environment ev;
  gchar *ior;
  gchar *retval;
  
  CORBA_exception_init (&ev);

  ior = CORBA_ORB_object_to_string (mateconf_orb_get (), obj, &ev);

  if (ior == NULL)
    {
      mateconf_set_error (err,
                       MATECONF_ERROR_FAILED,
                       _("Failed to convert object to IOR"));

      return NULL;
    }

  retval = g_strdup (ior);

  CORBA_free (ior);

  return retval;
}

static ConfigValueType
corba_type_from_mateconf_type(MateConfValueType type)
{
  switch (type)
    {
    case MATECONF_VALUE_INT:
      return IntVal;
    case MATECONF_VALUE_BOOL:
      return BoolVal;
    case MATECONF_VALUE_FLOAT:
      return FloatVal;
    case MATECONF_VALUE_INVALID:
      return InvalidVal;
    case MATECONF_VALUE_STRING:
      return StringVal;
    case MATECONF_VALUE_SCHEMA:
      return SchemaVal;
    case MATECONF_VALUE_LIST:
      return ListVal;
    case MATECONF_VALUE_PAIR:
      return PairVal;
    default:
      g_assert_not_reached();
      return InvalidVal;
    }
}

static MateConfValueType
mateconf_type_from_corba_type(ConfigValueType type)
{
  switch (type)
    {
    case InvalidVal:
      return MATECONF_VALUE_INVALID;
    case StringVal:
      return MATECONF_VALUE_STRING;
    case IntVal:
      return MATECONF_VALUE_INT;
    case FloatVal:
      return MATECONF_VALUE_FLOAT;
    case SchemaVal:
      return MATECONF_VALUE_SCHEMA;
    case BoolVal:
      return MATECONF_VALUE_BOOL;
    case ListVal:
      return MATECONF_VALUE_LIST;
    case PairVal:
      return MATECONF_VALUE_PAIR;
    default:
      g_assert_not_reached();
      return MATECONF_VALUE_INVALID;
    }
}

void          
mateconf_fill_corba_schema_from_mateconf_schema(const MateConfSchema *sc, 
                                          ConfigSchema      *cs)
{
  cs->value_type = corba_type_from_mateconf_type (mateconf_schema_get_type (sc));
  cs->value_list_type = corba_type_from_mateconf_type (mateconf_schema_get_list_type (sc));
  cs->value_car_type = corba_type_from_mateconf_type (mateconf_schema_get_car_type (sc));
  cs->value_cdr_type = corba_type_from_mateconf_type (mateconf_schema_get_cdr_type (sc));

  cs->locale = CORBA_string_dup (mateconf_schema_get_locale (sc) ? mateconf_schema_get_locale (sc) : "");
  cs->short_desc = CORBA_string_dup (mateconf_schema_get_short_desc (sc) ? mateconf_schema_get_short_desc (sc) : "");
  cs->long_desc = CORBA_string_dup (mateconf_schema_get_long_desc (sc) ? mateconf_schema_get_long_desc (sc) : "");
  cs->owner = CORBA_string_dup (mateconf_schema_get_owner (sc) ? mateconf_schema_get_owner (sc) : "");

  {
    gchar* encoded;
    MateConfValue* default_val;

    default_val = mateconf_schema_get_default_value (sc);

    if (default_val)
      {
        encoded = mateconf_value_encode (default_val);

        g_assert (encoded != NULL);

        cs->encoded_default_value = CORBA_string_dup (encoded);

        g_free (encoded);
      }
    else
      cs->encoded_default_value = CORBA_string_dup ("");
  }
}

ConfigSchema* 
mateconf_corba_schema_from_mateconf_schema (const MateConfSchema* sc)
{
  ConfigSchema* cs;

  cs = ConfigSchema__alloc ();

  mateconf_fill_corba_schema_from_mateconf_schema (sc, cs);

  return cs;
}

MateConfSchema*  
mateconf_schema_from_corba_schema(const ConfigSchema* cs)
{
  MateConfSchema* sc;
  MateConfValueType type = MATECONF_VALUE_INVALID;
  MateConfValueType list_type = MATECONF_VALUE_INVALID;
  MateConfValueType car_type = MATECONF_VALUE_INVALID;
  MateConfValueType cdr_type = MATECONF_VALUE_INVALID;

  type = mateconf_type_from_corba_type(cs->value_type);
  list_type = mateconf_type_from_corba_type(cs->value_list_type);
  car_type = mateconf_type_from_corba_type(cs->value_car_type);
  cdr_type = mateconf_type_from_corba_type(cs->value_cdr_type);

  sc = mateconf_schema_new();

  mateconf_schema_set_type(sc, type);
  mateconf_schema_set_list_type(sc, list_type);
  mateconf_schema_set_car_type(sc, car_type);
  mateconf_schema_set_cdr_type(sc, cdr_type);

  if (*cs->locale != '\0')
    {
      if (!g_utf8_validate (cs->locale, -1, NULL))
        mateconf_log (GCL_ERR, _("Invalid UTF-8 in locale for schema"));
      else
        mateconf_schema_set_locale(sc, cs->locale);
    }

  if (*cs->short_desc != '\0')
    {
      if (!g_utf8_validate (cs->short_desc, -1, NULL))
        mateconf_log (GCL_ERR, _("Invalid UTF-8 in short description for schema"));
      else
        mateconf_schema_set_short_desc(sc, cs->short_desc);
    }

  if (*cs->long_desc != '\0')
    {
      if (!g_utf8_validate (cs->long_desc, -1, NULL))
        mateconf_log (GCL_ERR, _("Invalid UTF-8 in long description for schema"));
      else
        mateconf_schema_set_long_desc(sc, cs->long_desc);
    }

  if (*cs->owner != '\0')
    {
      if (!g_utf8_validate (cs->owner, -1, NULL))
        mateconf_log (GCL_ERR, _("Invalid UTF-8 in owner for schema"));
      else
        mateconf_schema_set_owner(sc, cs->owner);
    }
      
  {
    MateConfValue* val;

    val = mateconf_value_decode(cs->encoded_default_value);

    if (val)
      mateconf_schema_set_default_value_nocopy(sc, val);
  }
  
  return sc;
}
#endif /* HAVE_CORBA */

const gchar* 
mateconf_value_type_to_string(MateConfValueType type)
{
  switch (type)
    {
    case MATECONF_VALUE_INT:
      return "int";
    case MATECONF_VALUE_STRING:
      return "string";
    case MATECONF_VALUE_FLOAT:
      return "float";
    case MATECONF_VALUE_BOOL:
      return "bool";
    case MATECONF_VALUE_SCHEMA:
      return "schema";
    case MATECONF_VALUE_LIST:
      return "list";
    case MATECONF_VALUE_PAIR:
      return "pair";
    case MATECONF_VALUE_INVALID:
      return "*invalid*";
    default:
      g_assert_not_reached();
      return NULL; /* for warnings */
    }
}

MateConfValueType 
mateconf_value_type_from_string(const gchar* type_str)
{
  if (strcmp(type_str, "int") == 0)
    return MATECONF_VALUE_INT;
  else if (strcmp(type_str, "float") == 0)
    return MATECONF_VALUE_FLOAT;
  else if (strcmp(type_str, "string") == 0)
    return MATECONF_VALUE_STRING;
  else if (strcmp(type_str, "bool") == 0)
    return MATECONF_VALUE_BOOL;
  else if (strcmp(type_str, "schema") == 0)
    return MATECONF_VALUE_SCHEMA;
  else if (strcmp(type_str, "list") == 0)
    return MATECONF_VALUE_LIST;
  else if (strcmp(type_str, "pair") == 0)
    return MATECONF_VALUE_PAIR;
  else
    return MATECONF_VALUE_INVALID;
}

/*
 * Config files (yikes! we can't store our config in MateConf!)
 */

static gchar*
unquote_string(gchar* s)
{
  gchar* end;

  /* Strip whitespace and first quote from front of string */
  while (*s && (g_ascii_isspace(*s) || (*s == '"')))
    ++s;

  end = s;
  while (*end)
    ++end;

  --end; /* one back from '\0' */

  /* Strip whitespace and last quote from end of string */
  while ((end > s) && (g_ascii_isspace(*end) || (*end == '"')))
    {
      *end = '\0';
      --end;
    }

  return s;
}

#ifdef G_OS_WIN32

/* Return the home directory with forward slashes instead of backslashes */

const char*
_mateconf_win32_get_home_dir (void)
{
  static GQuark quark = 0;
  char *home_copy, *p;

  if (quark != 0)
    return g_quark_to_string (quark);
  
  home_copy = g_strdup (g_get_home_dir ());

  /* Replace backslashes with forward slashes */
  for (p = home_copy; *p; p++)
    if (*p == '\\')
      *p = '/';
  
  quark = g_quark_from_string (home_copy);
  g_free (home_copy);

  return g_quark_to_string (quark);
}

#endif

static const gchar*
get_variable(const gchar* varname)
{
  /* These first two DO NOT use environment variables, which
     makes things a bit more "secure" in some sense
  */
  if (strcmp(varname, "HOME") == 0)
    {
#ifndef G_OS_WIN32
      return g_get_home_dir ();
#else
      return _mateconf_win32_get_home_dir ();
#endif
    }
  else if (strcmp(varname, "USER") == 0)
    {
      return g_get_user_name();
    }
  else if (varname[0] == 'E' &&
           varname[1] == 'N' &&
           varname[2] == 'V' &&
           varname[3] == '_')
    {
      /* This is magic: if a variable called ENV_FOO is used,
         then the environment variable FOO is checked */
      const gchar* envvar = g_getenv(&varname[4]);

      if (envvar)
        return envvar;
      else
        return "";
    }
  else
    return "";
}

static gchar*
subst_variables(const gchar* src)
{
  const gchar* iter;
  gchar* retval;
  guint retval_len;
  guint pos;
  
  g_return_val_if_fail(src != NULL, NULL);

  retval_len = strlen(src) + 1;
  pos = 0;
  
  retval = g_malloc0(retval_len+3); /* add 3 just to avoid off-by-one
                                       segvs - yeah I know it bugs
                                       you, but C sucks */
  
  iter = src;

  while (*iter)
    {
      gboolean performed_subst = FALSE;
      
      if (pos >= retval_len)
        {
          retval_len *= 2;
          retval = g_realloc(retval, retval_len+3); /* add 3 for luck */
        }
      
      if (*iter == '$' && *(iter+1) == '(')
        {
          const gchar* varstart = iter + 2;
          const gchar* varend   = strchr(varstart, ')');

          if (varend != NULL)
            {
              char* varname;
              const gchar* varval;
              guint varval_len;

              performed_subst = TRUE;

              varname = g_strndup(varstart, varend - varstart);
              
              varval = get_variable(varname);
              g_free(varname);

              varval_len = strlen(varval);

              if ((retval_len - pos) < varval_len)
                {
                  retval_len = pos + varval_len;
                  retval = g_realloc(retval, retval_len+3);
                }
              
              strcpy(&retval[pos], varval);
              pos += varval_len;

              iter = varend + 1;
            }
        }

      if (!performed_subst)
        {
          retval[pos] = *iter;
          ++pos;
          ++iter;
        }
    }
  retval[pos] = '\0';

  return retval;
}

GSList *
mateconf_load_source_path(const gchar* filename, GError** err)
{
  FILE* f;
  GSList *l = NULL;
  gchar buf[512];

  f = g_fopen(filename, "r");

  if (f == NULL)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_FAILED,
                               _("Couldn't open path file `%s': %s\n"), 
                               filename, 
                               g_strerror(errno));
      return NULL;
    }

  while (fgets(buf, 512, f) != NULL)
    {
      gchar* s = buf;
      
      while (*s && g_ascii_isspace(*s))
        ++s;

      if (*s == '#')
        {
          /* Allow comments, why not */
        }
      else if (*s == '\0')
        {
          /* Blank line */
        }
      else if (strncmp("include", s, 7) == 0)
        {
          gchar* unq;
          GSList* included;
          gchar *varsubst;
          
          s += 7;
          while (g_ascii_isspace(*s))
            s++;
          unq = unquote_string(s);

          varsubst = subst_variables (unq);
#ifdef G_OS_WIN32
	  {
	    gchar *tem = varsubst;
	    varsubst = _mateconf_win32_replace_prefix (varsubst);
	    g_free (tem);
	  }
#endif
          included = mateconf_load_source_path (varsubst, NULL);
          g_free (varsubst);
          
          if (included != NULL)
            l = g_slist_concat (l, included);
        }
      else 
        {
          gchar* unq;
          gchar* varsubst;
          
          unq = unquote_string(buf);
          varsubst = subst_variables(unq);
          
          if (*varsubst != '\0') /* Drop lines with just two quote marks or something */
            {
              mateconf_log(GCL_DEBUG, _("Adding source `%s'\n"), varsubst);
              l = g_slist_append (l, varsubst);
            }
	  else
	    {
	      g_free (varsubst);
	    }
        }
    }

  if (ferror(f))
    {
      /* This should basically never happen */
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_FAILED,
                               _("Read error on file `%s': %s\n"), 
                               filename,
                               g_strerror(errno));
      /* don't return, we want to go ahead and return any 
         addresses we already loaded. */
    }

  fclose(f);  

  return l;
}

char *
mateconf_address_list_get_persistent_name (GSList *addresses)
{
  GSList  *tmp;
  GString *str = NULL;

  if (!addresses)
    {
      return g_strdup ("empty");
    }

  tmp = addresses;
  while (tmp != NULL)
    {
      const char *address = tmp->data;

      if (str == NULL)
	{
	  str = g_string_new (address);
	}
      else
        {
          g_string_append_c (str, MATECONF_DATABASE_LIST_DELIM);
          g_string_append (str, address);
        }

      tmp = tmp->next;
    }

  return g_string_free (str, FALSE);
}

GSList *
mateconf_persistent_name_get_address_list (const char *persistent_name)
{
  char   delim [2] = { MATECONF_DATABASE_LIST_DELIM, '\0' };
  char **address_vector;

  address_vector = g_strsplit (persistent_name, delim, -1);
  if (address_vector != NULL)
    {
      GSList  *retval = NULL;
      int      i;

      i = 0;
      while (address_vector [i] != NULL)
        {
          retval = g_slist_append (retval, g_strdup (address_vector [i]));
          ++i;
        }

      g_strfreev (address_vector);

      return retval;
    }
  else
    {
      return g_slist_append (NULL, g_strdup (persistent_name));
    }
}

void
mateconf_address_list_free (GSList *addresses)
{
  GSList *tmp;

  tmp = addresses;
  while (tmp != NULL)
    {
      g_free (tmp->data);
      tmp = tmp->next;
    }

  g_slist_free (addresses);
}

/* This should also support concatting filesystem dirs and keys, 
   or dir and subdir.
*/
gchar*        
mateconf_concat_dir_and_key(const gchar* dir, const gchar* key)
{
  guint dirlen;
  guint keylen;
  gchar* retval;

  g_return_val_if_fail(dir != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(*dir == '/', NULL);

  dirlen = strlen(dir);
  keylen = strlen(key);

  retval = g_malloc0(dirlen+keylen+3); /* auto-null-terminate */

  strcpy(retval, dir);

  if (dir[dirlen-1] == '/')
    {
      /* dir ends in slash, strip key slash if needed */
      if (*key == '/')
        ++key;

      strcpy((retval+dirlen), key);
    }
  else 
    {
      /* Dir doesn't end in slash, add slash if key lacks one. */
      gchar* dest = retval + dirlen;

      if (*key != '/')
        {
          *dest = '/';
          ++dest;
        }
      
      strcpy(dest, key);
    }
  
  return retval;
}

gulong
mateconf_string_to_gulong(const gchar* str)
{
  gulong retval;
  gchar *end;
  errno = 0;
  retval = strtoul(str, &end, 10);
  if (end == str || errno != 0)
    retval = 0;

  return retval;
}

gboolean
mateconf_string_to_double(const gchar* str,
                       gdouble*     retloc)
{
  char *end;

  errno = 0;
  *retloc = g_ascii_strtod (str, &end);
  if (end == str || errno != 0)
    {
      *retloc = 0.0;
      return FALSE;
    }
  else
    return TRUE;
}

gchar*
mateconf_double_to_string (gdouble val)
{
  char str[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, val);
  
  return g_strdup (str);
}

const gchar*
mateconf_current_locale(void)
{
#ifdef HAVE_LC_MESSAGES
  return setlocale(LC_MESSAGES, NULL);
#else
  return setlocale(LC_CTYPE, NULL);
#endif
}

/*
 * Log
 */

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

void
mateconf_log(MateConfLogPriority pri, const gchar* fmt, ...)
{
  gchar* msg;
  va_list args;
#ifdef HAVE_SYSLOG_H
  gchar* convmsg;
  int syslog_pri = LOG_DEBUG;
#endif

  if (!mateconf_log_debug_messages && 
      pri == GCL_DEBUG)
    return;
  
  va_start (args, fmt);
  msg = g_strdup_vprintf(fmt, args);
  va_end (args);

#ifdef HAVE_SYSLOG_H
  if (mateconf_daemon_mode)
    {
      switch (pri)
        {
        case GCL_EMERG:
          syslog_pri = LOG_EMERG;
          break;
      
        case GCL_ALERT:
          syslog_pri = LOG_ALERT;
          break;
      
        case GCL_CRIT:
          syslog_pri = LOG_CRIT;
          break;
      
        case GCL_ERR:
          syslog_pri = LOG_ERR;
          break;
      
        case GCL_WARNING:
          syslog_pri = LOG_WARNING;
          break;
      
        case GCL_NOTICE:
          syslog_pri = LOG_NOTICE;
          break;
      
        case GCL_INFO:
          syslog_pri = LOG_INFO;
          break;
      
        case GCL_DEBUG:
          syslog_pri = LOG_DEBUG;
          break;

        default:
          g_assert_not_reached();
          break;
        }

      convmsg = g_locale_from_utf8 (msg, -1, NULL, NULL, NULL);
      if (!convmsg)
        syslog (syslog_pri, "%s", msg);
      else
        {
	  syslog (syslog_pri, "%s", convmsg);
	  g_free (convmsg);
	}
    }
  else
#endif
    {
      switch (pri)
        {
        case GCL_EMERG:
        case GCL_ALERT:
        case GCL_CRIT:
        case GCL_ERR:
        case GCL_WARNING:
          g_printerr ("%s\n", msg);
          break;
      
        case GCL_NOTICE:
        case GCL_INFO:
        case GCL_DEBUG:
          g_print ("%s\n", msg);
          break;

        default:
          g_assert_not_reached();
          break;
        }
    }
  
  g_free(msg);
}

/*
 * List/pair conversion
 */

MateConfValue*
mateconf_value_list_from_primitive_list(MateConfValueType list_type, GSList* list,
                                     GError **err)
{
  GSList* value_list;
  GSList* tmp;

  g_return_val_if_fail(list_type != MATECONF_VALUE_INVALID, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_PAIR, NULL);
  
  value_list = NULL;

  tmp = list;

  while (tmp != NULL)
    {
      MateConfValue* val;

      val = mateconf_value_new(list_type);

      switch (list_type)
        {
        case MATECONF_VALUE_INT:
          mateconf_value_set_int(val, GPOINTER_TO_INT(tmp->data));
          break;

        case MATECONF_VALUE_BOOL:
          mateconf_value_set_bool(val, GPOINTER_TO_INT(tmp->data));
          break;

        case MATECONF_VALUE_FLOAT:
          mateconf_value_set_float(val, *((gdouble*)tmp->data));
          break;

        case MATECONF_VALUE_STRING:
          if (!g_utf8_validate (tmp->data, -1, NULL))
            {
              g_set_error (err, MATECONF_ERROR,
                           MATECONF_ERROR_FAILED,
                           _("Text contains invalid UTF-8"));
              goto error;
            }
                     
          mateconf_value_set_string(val, tmp->data);
          break;

        case MATECONF_VALUE_SCHEMA:
          if (!mateconf_schema_validate (tmp->data, err))
            goto error;
          mateconf_value_set_schema(val, tmp->data);
          break;
          
        default:
          g_assert_not_reached();
          break;
        }

      value_list = g_slist_prepend(value_list, val);

      tmp = g_slist_next(tmp);
    }

  /* Get it in the right order. */
  value_list = g_slist_reverse(value_list);

  {
    MateConfValue* value_with_list;
    
    value_with_list = mateconf_value_new(MATECONF_VALUE_LIST);
    mateconf_value_set_list_type(value_with_list, list_type);
    mateconf_value_set_list_nocopy(value_with_list, value_list);

    return value_with_list;
  }

 error:
  g_slist_foreach (value_list, (GFunc)mateconf_value_free, NULL);
  g_slist_free (value_list);
  return NULL;
}


static MateConfValue*
from_primitive(MateConfValueType type, gconstpointer address,
               GError **err)
{
  MateConfValue* val;

  val = mateconf_value_new(type);

  switch (type)
    {
    case MATECONF_VALUE_INT:
      mateconf_value_set_int(val, *((const gint*)address));
      break;

    case MATECONF_VALUE_BOOL:
      mateconf_value_set_bool(val, *((const gboolean*)address));
      break;

    case MATECONF_VALUE_STRING:
      if (!g_utf8_validate (*((const gchar**)address), -1, NULL))
        {
          g_set_error (err, MATECONF_ERROR,
                       MATECONF_ERROR_FAILED,
                       _("Text contains invalid UTF-8"));
          mateconf_value_free (val);
          return NULL;
        }

      mateconf_value_set_string(val, *((const gchar**)address));
      break;

    case MATECONF_VALUE_FLOAT:
      mateconf_value_set_float(val, *((const gdouble*)address));
      break;

    case MATECONF_VALUE_SCHEMA:
      if (!mateconf_schema_validate (*((MateConfSchema**)address), err))
        {
          mateconf_value_free (val);
          return NULL;
        }
      
      mateconf_value_set_schema(val, *((MateConfSchema**)address));
      break;
      
    default:
      g_assert_not_reached();
      break;
    }

  return val;
}

MateConfValue*
mateconf_value_pair_from_primitive_pair(MateConfValueType car_type,
                                     MateConfValueType cdr_type,
                                     gconstpointer address_of_car,
                                     gconstpointer address_of_cdr,
                                     GError       **err)
{
  MateConfValue* pair;
  MateConfValue* car;
  MateConfValue* cdr;
  
  g_return_val_if_fail(car_type != MATECONF_VALUE_INVALID, NULL);
  g_return_val_if_fail(car_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(car_type != MATECONF_VALUE_PAIR, NULL);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_INVALID, NULL);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_PAIR, NULL);
  g_return_val_if_fail(address_of_car != NULL, NULL);
  g_return_val_if_fail(address_of_cdr != NULL, NULL);
  
  car = from_primitive(car_type, address_of_car, err);
  if (car == NULL)
    return NULL;
  cdr = from_primitive(cdr_type, address_of_cdr, err);
  if (cdr == NULL)
    {
      mateconf_value_free (car);
      return NULL;
    }
  
  pair = mateconf_value_new(MATECONF_VALUE_PAIR);
  mateconf_value_set_car_nocopy(pair, car);
  mateconf_value_set_cdr_nocopy(pair, cdr);

  return pair;
}


GSList*
mateconf_value_list_to_primitive_list_destructive(MateConfValue* val,
                                               MateConfValueType list_type,
                                               GError** err)
{
  GSList* retval;

  g_return_val_if_fail(val != NULL, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_INVALID, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_PAIR, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  if (val->type != MATECONF_VALUE_LIST)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH,
                               _("Expected list, got %s"),
                               mateconf_value_type_to_string(val->type));
      mateconf_value_free(val);
      return NULL;
    }

  if (mateconf_value_get_list_type(val) != list_type)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH,
                               _("Expected list of %s, got list of %s"),
                               mateconf_value_type_to_string(list_type),
                               mateconf_value_type_to_string(mateconf_value_get_list_type (val)));
      mateconf_value_free(val);
      return NULL;
    }

  g_assert(mateconf_value_get_list_type(val) == list_type);
      
  retval = mateconf_value_steal_list (val);
      
  mateconf_value_free (val);
  val = NULL;
      
  {
    /* map (typeChange, retval) */
    GSList* tmp;

    tmp = retval;

    while (tmp != NULL)
      {
        MateConfValue* elem = tmp->data;

        g_assert(elem != NULL);
        g_assert(elem->type == list_type);
            
        switch (list_type)
          {
          case MATECONF_VALUE_INT:
            tmp->data = GINT_TO_POINTER(mateconf_value_get_int(elem));
            break;

          case MATECONF_VALUE_BOOL:
            tmp->data = GINT_TO_POINTER(mateconf_value_get_bool(elem));
            break;
                
          case MATECONF_VALUE_FLOAT:
            {
              gdouble* d = g_new(gdouble, 1);
              *d = mateconf_value_get_float(elem);
              tmp->data = d;
            }
            break;

          case MATECONF_VALUE_STRING:
            tmp->data = mateconf_value_steal_string (elem);
            break;

          case MATECONF_VALUE_SCHEMA:
            tmp->data = mateconf_value_steal_schema (elem);
            break;
                
          default:
            g_assert_not_reached();
            break;
          }

        /* Clean up the value */
        mateconf_value_free(elem);
            
        tmp = g_slist_next(tmp);
      }
  } /* list conversion block */
      
  return retval;
}


static void
primitive_value(gpointer retloc, MateConfValue* val)
{
  switch (val->type)
    {
    case MATECONF_VALUE_INT:
      *((gint*)retloc) = mateconf_value_get_int(val);
      break;

    case MATECONF_VALUE_FLOAT:
      *((gdouble*)retloc) = mateconf_value_get_float(val);
      break;

    case MATECONF_VALUE_STRING:
      {
        *((gchar**)retloc) = mateconf_value_steal_string (val);
      }
      break;

    case MATECONF_VALUE_BOOL:
      *((gboolean*)retloc) = mateconf_value_get_bool(val);
      break;

    case MATECONF_VALUE_SCHEMA:
      *((MateConfSchema**)retloc) = mateconf_value_steal_schema(val);
      break;
      
    default:
      g_assert_not_reached();
      break;
    }
}

gboolean
mateconf_value_pair_to_primitive_pair_destructive(MateConfValue* val,
                                               MateConfValueType car_type,
                                               MateConfValueType cdr_type,
                                               gpointer car_retloc,
                                               gpointer cdr_retloc,
                                               GError** err)
{
  MateConfValue* car;
  MateConfValue* cdr;

  g_return_val_if_fail(val != NULL, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(car_retloc != NULL, FALSE);
  g_return_val_if_fail(cdr_retloc != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);  
      
  if (val->type != MATECONF_VALUE_PAIR)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH,
                               _("Expected pair, got %s"),
                               mateconf_value_type_to_string(val->type));
      mateconf_value_free(val);
      return FALSE;
    }

  car = mateconf_value_get_car(val);
  cdr = mateconf_value_get_cdr(val);
      
  if (car == NULL ||
      cdr == NULL)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, 
                               _("Expected (%s,%s) pair, got a pair with one or both values missing"),
                               mateconf_value_type_to_string(car_type),
                               mateconf_value_type_to_string(cdr_type));

      mateconf_value_free(val);
      return FALSE;
    }

  g_assert(car != NULL);
  g_assert(cdr != NULL);
      
  if (car->type != car_type ||
      cdr->type != cdr_type)
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH,
                               _("Expected pair of type (%s,%s) got type (%s,%s)"),
                               mateconf_value_type_to_string(car_type),
                               mateconf_value_type_to_string(cdr_type),
                               mateconf_value_type_to_string(car->type),
                               mateconf_value_type_to_string(cdr->type));
      mateconf_value_free(val);
      return FALSE;
    }

  primitive_value(car_retloc, car);
  primitive_value(cdr_retloc, cdr);

  mateconf_value_free(val);

  return TRUE;
}



/*
 * Encode/decode
 */

gchar*
mateconf_quote_string   (const gchar* src)
{
  gchar* dest;
  const gchar* s;
  gchar* d;

  g_return_val_if_fail(src != NULL, NULL);
  
  /* waste memory! woo-hoo! */
  dest = g_malloc0(strlen(src)*2+4);
  
  d = dest;

  *d = '"';
  ++d;
  
  s = src;
  while (*s)
    {
      switch (*s)
        {
        case '"':
          {
            *d = '\\';
            ++d;
            *d = '"';
            ++d;
          }
          break;
          
        case '\\':
          {
            *d = '\\';
            ++d;
            *d = '\\';
            ++d;
          }
          break;
          
        default:
          {
            *d = *s;
            ++d;
          }
          break;
        }
      ++s;
    }

  /* End with quote mark and NULL */
  *d = '"';
  ++d;
  *d = '\0';
  
  return dest;
}

gchar*
mateconf_unquote_string (const gchar* str, const gchar** end, GError** err)
{
  gchar* unq;
  gchar* unq_end = NULL;

  g_return_val_if_fail(end != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  g_return_val_if_fail(str != NULL, NULL);
  
  unq = g_strdup(str);

  mateconf_unquote_string_inplace(unq, &unq_end, err);

  *end = (str + (unq_end - unq));

  return unq;
}

void
mateconf_unquote_string_inplace (gchar* str, gchar** end, GError** err)
{
  gchar* dest;
  gchar* s;

  g_return_if_fail(end != NULL);
  g_return_if_fail(err == NULL || *err == NULL);
  g_return_if_fail(str != NULL);
  
  dest = s = str;

  if (*s != '"')
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                               _("Quoted string doesn't begin with a quotation mark"));
      *end = str;
      return;
    }

  /* Skip the initial quote mark */
  ++s;
  
  while (*s)
    {
      g_assert(s > dest); /* loop invariant */
      
      switch (*s)
        {
        case '"':
          /* End of the string, return now */
          *dest = '\0';
          ++s;
          *end = s;
          return;

        case '\\':
          /* Possible escaped quote or \ */
          ++s;
          if (*s == '"')
            {
              *dest = *s;
              ++s;
              ++dest;
            }
          else if (*s == '\\')
            {
              *dest = *s;
              ++s;
              ++dest;
            }
          else
            {
              /* not an escaped char */
              *dest = '\\';
              ++dest;
              /* ++s already done. */
            }
          break;

        default:
          *dest = *s;
          ++dest;
          ++s;
          break;
        }

      g_assert(s > dest); /* loop invariant */
    }
  
  /* If we reach here this means the close quote was never encountered */

  *dest = '\0';
  
  if (err)
    *err = mateconf_error_new(MATECONF_ERROR_PARSE_ERROR,
                           _("Quoted string doesn't end with a quotation mark"));
  *end = s;
  return;
}

/* The encoding format

   The first byte of the encoded string is the type of the value:

    i  int
    b  bool
    f  float
    s  string
    c  schema
    p  pair
    l  list
    v  invalid

    For int, the rest of the encoded value is the integer to be parsed with atoi()
    For bool, the rest is 't' or 'f'
    For float, the rest is a float to parse with g_strtod()
    For string, the rest is the string (not quoted)
    For schema, the encoding is complicated; see below.
    For pair, the rest is two primitive encodings (ibcfs), quoted, separated by a comma,
              car before cdr
    For list, first character is type, the rest is primitive encodings, quoted,
              separated by commas

    Schema:

    After the 'c' indicating schema, the second character is a byte indicating
    the type the schema expects. Then a comma, and the quoted locale, or "" for none.
    comma, and quoted short description; comma, quoted long description; comma, default
    value in the encoded format given above, quoted.
*/

static gchar type_byte(MateConfValueType type)
{
  switch (type)
    {
    case MATECONF_VALUE_INT:
      return 'i';
        
    case MATECONF_VALUE_BOOL:
      return 'b';

    case MATECONF_VALUE_FLOAT:
      return 'f';

    case MATECONF_VALUE_STRING:
      return 's';

    case MATECONF_VALUE_SCHEMA:
      return 'c';

    case MATECONF_VALUE_LIST:
      return 'l';

    case MATECONF_VALUE_PAIR:
      return 'p';

    case MATECONF_VALUE_INVALID:
      return 'v';
      
    default:
      g_assert_not_reached();
      return '\0';
    }
}

static MateConfValueType
byte_type(gchar byte)
{
  switch (byte)
    {
    case 'i':
      return MATECONF_VALUE_INT;

    case 'b':
      return MATECONF_VALUE_BOOL;

    case 's':
      return MATECONF_VALUE_STRING;

    case 'c':
      return MATECONF_VALUE_SCHEMA;

    case 'f':
      return MATECONF_VALUE_FLOAT;

    case 'l':
      return MATECONF_VALUE_LIST;

    case 'p':
      return MATECONF_VALUE_PAIR;
      
    case 'v':
      return MATECONF_VALUE_INVALID;

    default:
      return MATECONF_VALUE_INVALID;
    }
}

MateConfValue*
mateconf_value_decode (const gchar* encoded)
{
  MateConfValueType type;
  MateConfValue* val;
  const gchar* s;
  
  type = byte_type(*encoded);

  if (type == MATECONF_VALUE_INVALID)
    return NULL;

  if (!g_utf8_validate (encoded, -1, NULL))
    {
      mateconf_log (GCL_ERR, _("Encoded value is not valid UTF-8"));
      return NULL;
    }
  
  val = mateconf_value_new(type);

  s = encoded + 1;
  
  switch (val->type)
    {
    case MATECONF_VALUE_INT:
      mateconf_value_set_int(val, atoi(s));
      break;
        
    case MATECONF_VALUE_BOOL:
      mateconf_value_set_bool(val, *s == 't' ? TRUE : FALSE);
      break;

    case MATECONF_VALUE_FLOAT:
      {
        double d;
        gchar* endptr = NULL;
        
        d = g_strtod(s, &endptr);
        if (endptr == s)
          g_warning("Failure converting string to double in %s", G_GNUC_FUNCTION);
        mateconf_value_set_float(val, d);
      }
      break;

    case MATECONF_VALUE_STRING:
      {
        mateconf_value_set_string(val, s);
      }
      break;

    case MATECONF_VALUE_SCHEMA:
      {
        MateConfSchema* sc = mateconf_schema_new();
        const gchar* end = NULL;
        gchar* unquoted;
        
        mateconf_value_set_schema_nocopy(val, sc);

        mateconf_schema_set_type(sc, byte_type(*s));
        ++s;
        mateconf_schema_set_list_type(sc, byte_type(*s));
        ++s;
        mateconf_schema_set_car_type(sc, byte_type(*s));
        ++s;
        mateconf_schema_set_cdr_type(sc, byte_type(*s));
        ++s;

        if (*s != ',')
          g_warning("no comma after types in schema");

	++s;

        /* locale */
        unquoted = mateconf_unquote_string(s, &end, NULL);

        mateconf_schema_set_locale(sc, unquoted);

        g_free(unquoted);
        
        if (*end != ',')
          g_warning("no comma after locale in schema");

        ++end;
        s = end;

        /* short */
        unquoted = mateconf_unquote_string(s, &end, NULL);

        mateconf_schema_set_short_desc(sc, unquoted);

        g_free(unquoted);
        
        if (*end != ',')
          g_warning("no comma after short desc in schema");

        ++end;
        s = end;

        /* long */
        unquoted = mateconf_unquote_string(s, &end, NULL);

        mateconf_schema_set_long_desc(sc, unquoted);

        g_free(unquoted);
        
        if (*end != ',')
          g_warning("no comma after long desc in schema");

        ++end;
        s = end;
        
        /* default value */
        unquoted = mateconf_unquote_string(s, &end, NULL);

        mateconf_schema_set_default_value_nocopy(sc, mateconf_value_decode(unquoted));

        g_free(unquoted);
        
        if (*end != '\0')
          g_warning("trailing junk after encoded schema");
      }
      break;

    case MATECONF_VALUE_LIST:
      {
        GSList* value_list = NULL;

        mateconf_value_set_list_type(val, byte_type(*s));
	++s;

        while (*s)
          {
            gchar* unquoted;
            const gchar* end;
            
            MateConfValue* elem;
            
            unquoted = mateconf_unquote_string(s, &end, NULL);            

            elem = mateconf_value_decode(unquoted);

            g_free(unquoted);
            
            if (elem)
              value_list = g_slist_prepend(value_list, elem);
            
            s = end;
            if (*s == ',')
              ++s;
            else if (*s != '\0')
              {
                g_warning("weird character in encoded list");
                break; /* error */
              }
          }

        value_list = g_slist_reverse(value_list);

        mateconf_value_set_list_nocopy(val, value_list);
      }
      break;

    case MATECONF_VALUE_PAIR:
      {
        gchar* unquoted;
        const gchar* end;
        
        MateConfValue* car;
        MateConfValue* cdr;
        
        unquoted = mateconf_unquote_string(s, &end, NULL);            
        
        car = mateconf_value_decode(unquoted);

        g_free(unquoted);
        
        s = end;
        if (*s == ',')
          ++s;
        else
          {
            g_warning("weird character in encoded pair");
          }
        
        unquoted = mateconf_unquote_string(s, &end, NULL);
        
        cdr = mateconf_value_decode(unquoted);
        g_free(unquoted);


        mateconf_value_set_car_nocopy(val, car);
        mateconf_value_set_cdr_nocopy(val, cdr);
      }
      break;

    default:
      g_assert_not_reached();
      break;
    }

  return val;
}

gchar*
mateconf_value_encode (MateConfValue* val)
{
  gchar* retval = NULL;
  
  g_return_val_if_fail(val != NULL, NULL);

  switch (val->type)
    {
    case MATECONF_VALUE_INT:
      retval = g_strdup_printf("i%d", mateconf_value_get_int(val));
      break;
        
    case MATECONF_VALUE_BOOL:
      retval = g_strdup_printf("b%c", mateconf_value_get_bool(val) ? 't' : 'f');
      break;

    case MATECONF_VALUE_FLOAT:
      retval = g_strdup_printf("f%g", mateconf_value_get_float(val));
      break;

    case MATECONF_VALUE_STRING:
      retval = g_strdup_printf("s%s", mateconf_value_get_string(val));
      break;

    case MATECONF_VALUE_SCHEMA:
      {
        gchar* tmp;
        gchar* quoted;
        gchar* encoded;
        MateConfSchema* sc;

        sc = mateconf_value_get_schema(val);
        
        tmp = g_strdup_printf("c%c%c%c%c,",
			      type_byte(mateconf_schema_get_type(sc)),
			      type_byte(mateconf_schema_get_list_type(sc)),
			      type_byte(mateconf_schema_get_car_type(sc)),
			      type_byte(mateconf_schema_get_cdr_type(sc)));

        quoted = mateconf_quote_string(mateconf_schema_get_locale(sc) ?
                                    mateconf_schema_get_locale(sc) : "");
        retval = g_strconcat(tmp, quoted, ",", NULL);

        g_free(tmp);
        g_free(quoted);

        tmp = retval;
        quoted = mateconf_quote_string(mateconf_schema_get_short_desc(sc) ?
                                    mateconf_schema_get_short_desc(sc) : "");

        retval = g_strconcat(tmp, quoted, ",", NULL);

        g_free(tmp);
        g_free(quoted);


        tmp = retval;
        quoted = mateconf_quote_string(mateconf_schema_get_long_desc(sc) ?
                                    mateconf_schema_get_long_desc(sc) : "");

        retval = g_strconcat(tmp, quoted, ",", NULL);

        g_free(tmp);
        g_free(quoted);
        

        if (mateconf_schema_get_default_value(sc) != NULL)
          encoded = mateconf_value_encode(mateconf_schema_get_default_value(sc));
        else
          encoded = g_strdup("");

        tmp = retval;
          
        quoted = mateconf_quote_string(encoded);

        retval = g_strconcat(tmp, quoted, NULL);

        g_free(tmp);
        g_free(quoted);
        g_free(encoded);
      }
      break;

    case MATECONF_VALUE_LIST:
      {
        GSList* tmp;

        retval = g_strdup_printf("l%c", type_byte(mateconf_value_get_list_type(val)));
        
        tmp = mateconf_value_get_list(val);

        while (tmp != NULL)
          {
            MateConfValue* elem = tmp->data;
            gchar* encoded;
            gchar* quoted;
            
            g_assert(elem != NULL);

            encoded = mateconf_value_encode(elem);

            quoted = mateconf_quote_string(encoded);

            g_free(encoded);

            {
              gchar* free_me;
              free_me = retval;
              
              retval = g_strconcat(retval, ",", quoted, NULL);
              
              g_free(quoted);
              g_free(free_me);
            }
            
            tmp = g_slist_next(tmp);
          }
      }
      break;

    case MATECONF_VALUE_PAIR:
      {
        gchar* car_encoded;
        gchar* cdr_encoded;
        gchar* car_quoted;
        gchar* cdr_quoted;

        car_encoded = mateconf_value_encode(mateconf_value_get_car(val));
        cdr_encoded = mateconf_value_encode(mateconf_value_get_cdr(val));

        car_quoted = mateconf_quote_string(car_encoded);
        cdr_quoted = mateconf_quote_string(cdr_encoded);

        retval = g_strconcat("p", car_quoted, ",", cdr_quoted, NULL);

        g_free(car_encoded);
        g_free(cdr_encoded);
        g_free(car_quoted);
        g_free(cdr_quoted);
      }
      break;

    default:
      g_assert_not_reached();
      break;
      
    }

  return retval;
}

#ifdef HAVE_CORBA

/*
 * Locks
 */

/*
 * Locks works as follows. We have a lock directory to hold the locking
 * mess, and we have an IOR file inside the lock directory with the
 * mateconfd IOR, and we have an fcntl() lock on the IOR file. The IOR
 * file is created atomically using a temporary file, then link()
 */

struct _MateConfLock {
  gchar *lock_directory;
  gchar *iorfile;
  int    lock_fd;
};

static void
mateconf_lock_destroy (MateConfLock* lock)
{
  if (lock->lock_fd >= 0)
    close (lock->lock_fd);
  g_free (lock->iorfile);
  g_free (lock->lock_directory);
  g_free (lock);
}

#ifndef G_OS_WIN32

static void
set_close_on_exec (int fd)
{
#if defined (F_GETFD) && defined (FD_CLOEXEC)
  int val;

  val = fcntl (fd, F_GETFD, 0);
  if (val < 0)
    {
      mateconf_log (GCL_DEBUG, "couldn't F_GETFD: %s\n", g_strerror (errno));
      return;
    }

  val |= FD_CLOEXEC;

  if (fcntl (fd, F_SETFD, val) < 0)
    mateconf_log (GCL_DEBUG, "couldn't F_SETFD: %s\n", g_strerror (errno));
#endif
}

#endif

#ifdef F_SETLK
/* Your basic Stevens cut-and-paste */
static int
lock_reg (int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
  struct flock lock;

  lock.l_type = type; /* F_RDLCK, F_WRLCK, F_UNLCK */
  lock.l_start = offset; /* byte offset relative to whence */
  lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
  lock.l_len = len; /* #bytes, 0 for eof */

  return fcntl (fd, cmd, &lock);
}
#endif

#ifdef F_SETLK
#define lock_entire_file(fd) \
  lock_reg ((fd), F_SETLK, F_WRLCK, 0, SEEK_SET, 0)
#define unlock_entire_file(fd) \
  lock_reg ((fd), F_SETLK, F_UNLCK, 0, SEEK_SET, 0)
#elif defined (G_OS_WIN32)
/* We don't use these macros */
#else
#warning Please implement proper locking
#define lock_entire_file(fd) 0
#define unlock_entire_file(fd) 0
#endif

static gboolean
file_locked_by_someone_else (int fd)
{
#ifdef F_SETLK
  struct flock lock;

  lock.l_type = F_WRLCK;
  lock.l_start = 0;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;

  if (fcntl (fd, F_GETLK, &lock) < 0)
    return TRUE; /* pretend it's locked */

  if (lock.l_type == F_UNLCK)
    return FALSE; /* we have the lock */
  else
    return TRUE; /* someone else has it */
#else
  return FALSE;
#endif
}

#ifndef G_OS_WIN32

static char*
unique_filename (const char *directory)
{
  char *guid;
  char *uniquefile;
  
  guid = mateconf_unique_key ();
  uniquefile = g_strconcat (directory, "/", guid, NULL);
  g_free (guid);

  return uniquefile;
}

#endif

static int
create_new_locked_file (const gchar *directory,
                        const gchar *filename,
                        GError     **err)
{
  int fd;
  gboolean got_lock = FALSE;

#ifndef G_OS_WIN32

  char *uniquefile = unique_filename (directory);

  fd = open (uniquefile, O_WRONLY | O_CREAT, 0700);

  /* Lock our temporary file, lock hopefully applies to the
   * inode and so also counts once we link it to the new name
   */
  if (lock_entire_file (fd) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_LOCK_FAILED,
                   _("Could not lock temporary file '%s': %s"),
                   uniquefile, g_strerror (errno));
      goto out;
    }
  
  /* Create lockfile as a link to unique file */
  if (link (uniquefile, filename) == 0)
    {
      /* filename didn't exist before, and open succeeded, and we have the lock */
      got_lock = TRUE;
      goto out;
    }
  else
    {
      /* see if the link really succeeded */
      struct stat sb;
      if (stat (uniquefile, &sb) == 0 &&
          sb.st_nlink == 2)
        {
          got_lock = TRUE;
          goto out;
        }
      else
        {
          g_set_error (err,
                       MATECONF_ERROR,
                       MATECONF_ERROR_LOCK_FAILED,
                       _("Could not create file '%s', probably because it already exists"),
                       filename);
          goto out;
        }
    }
  
 out:
  if (got_lock)
    set_close_on_exec (fd);
  
  unlink (uniquefile);
  g_free (uniquefile);

#else

  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      fd = _wsopen (wfilename, O_WRONLY|O_CREAT|O_EXCL, SH_DENYWR, 0700);
      g_free (wfilename);
    }
  else
    {
      char *cpfilename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      fd = _sopen (cpfilename, O_WRONLY|O_CREAT|O_EXCL, SH_DENYWR, 0700);
      g_free (cpfilename);
    }

  got_lock = (fd >= 0);

#endif

  if (!got_lock)
    {
      if (fd >= 0)
        close (fd);
      fd = -1;
    }
  
  return fd;
}

static int
open_empty_locked_file (const gchar *directory,
                        const gchar *filename,
                        GError     **err)
{
  int fd;

  fd = create_new_locked_file (directory, filename, NULL);

  if (fd >= 0)
    return fd;
  
  /* We failed to create the file, most likely because it already
   * existed; try to get the lock on the existing file, and if we can
   * get that lock, delete it, then start over.
   */

#ifndef G_OS_WIN32

  fd = open (filename, O_RDWR, 0700);
  if (fd < 0)
    {
      /* File has gone away? */
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_LOCK_FAILED,
                   _("Failed to create or open '%s'"),
                   filename);
      return -1;
    }

  if (lock_entire_file (fd) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_LOCK_FAILED,
                   _("Failed to lock '%s': probably another process has the lock, or your operating system has NFS file locking misconfigured (%s)"),
                   filename, g_strerror (errno));
      close (fd);
      return -1;
    }

  /* We have the lock on filename, so delete it */
  /* FIXME this leaves .nfs32423432 cruft */
  unlink (filename);
  close (fd);
  fd = -1;

#else
  /* Unlinking open files fail on Win32 */

  if (g_remove (filename) == -1)
    {
      g_set_error (err,
		   MATECONF_ERROR,
		   MATECONF_ERROR_LOCK_FAILED,
		   _("Failed to remove '%s': %s"),
		   filename, g_strerror (errno));
      return -1;
    }
#endif

  /* Now retry creating our file */
  fd = create_new_locked_file (directory, filename, err);
  
  return fd;
}

static ConfigServer
read_current_server_and_set_warning (const gchar *iorfile,
                                     GString     *warning)
{
  FILE *fp;
  
  fp = g_fopen (iorfile, "r");
          
  if (fp == NULL)
    {
      if (warning)
        g_string_append_printf (warning,
                                _("IOR file '%s' not opened successfully, no mateconfd located: %s"),
                                iorfile, g_strerror (errno));

      return CORBA_OBJECT_NIL;
    }
  else /* successfully opened IOR file */
    {
      char buf[2048] = { '\0' };
      const char *str = NULL;
      
      fgets (buf, sizeof (buf) - 2, fp);
      fclose (fp);

      /* The lockfile format is <pid>:<ior> for mateconfd
       * or <pid>:none for mateconftool
       */
      str = buf;
      while (isdigit ((unsigned char) *str))
        ++str;

      if (*str == ':')
        ++str;
          
      if (str[0] == 'n' &&
          str[1] == 'o' &&
          str[2] == 'n' &&
          str[3] == 'e')
        {
          if (warning)
            g_string_append_printf (warning,
                                    _("mateconftool or other non-mateconfd process has the lock file '%s'"),
                                    iorfile); 
        }
      else /* file contains daemon IOR */
        {
          CORBA_ORB orb;
          CORBA_Environment ev;
          ConfigServer server;
          
          CORBA_exception_init (&ev);
                  
          orb = mateconf_orb_get ();

          if (orb == NULL)
            {
              if (warning)
                g_string_append_printf (warning,
                                        _("couldn't contact ORB to resolve existing mateconfd object reference"));
              return CORBA_OBJECT_NIL;
            }
                  
          server = CORBA_ORB_string_to_object (orb, (char*) str, &ev);
          CORBA_exception_free (&ev);

          if (server == CORBA_OBJECT_NIL &&
              warning)
            g_string_append_printf (warning,
                                    _("Failed to convert IOR '%s' to an object reference"),
                                    str);
          
          return server;
        }

      return CORBA_OBJECT_NIL;
    }
}

static ConfigServer
read_current_server (const gchar *iorfile,
                     gboolean     warn_if_fail)
{
  GString *warning;
  ConfigServer server;
  
  if (warn_if_fail)
    warning = g_string_new (NULL);
  else
    warning = NULL;

  server = read_current_server_and_set_warning (iorfile, warning);

  if (warning)
    {
      if (warning->len > 0)
	mateconf_log (GCL_WARNING, "%s", warning->str);

      g_string_free (warning, TRUE);
    }

  return server;
}

MateConfLock*
mateconf_get_lock_or_current_holder (const gchar  *lock_directory,
                                  ConfigServer *current_server,
                                  GError      **err)
{
  MateConfLock* lock;
  
  g_return_val_if_fail(lock_directory != NULL, NULL);

  if (current_server)
    *current_server = CORBA_OBJECT_NIL;
  
  if (g_mkdir (lock_directory, 0700) < 0 &&
      errno != EEXIST)
    {
      mateconf_set_error (err,
                       MATECONF_ERROR_LOCK_FAILED,
                       _("couldn't create directory `%s': %s"),
                       lock_directory, g_strerror (errno));

      return NULL;
    }

  lock = g_new0 (MateConfLock, 1);

  lock->lock_directory = g_strdup (lock_directory);

  lock->iorfile = g_strconcat (lock->lock_directory, "/ior", NULL);

  /* Check the current IOR file and ping its daemon */
  
  lock->lock_fd = open_empty_locked_file (lock->lock_directory,
                                          lock->iorfile,
                                          err);
  
  if (lock->lock_fd < 0)
    {
      /* We didn't get the lock. Read the old server, and provide
       * it to the caller. Error is already set.
       */
      if (current_server)
        *current_server = read_current_server (lock->iorfile, TRUE);

      mateconf_lock_destroy (lock);
      
      return NULL;
    }
  else
    {
      /* Write IOR to lockfile */
      const gchar* ior;
      int retval;
      gchar* s;
      
      s = g_strdup_printf ("%u:", (guint) getpid ());
        
      retval = write (lock->lock_fd, s, strlen (s));

      g_free (s);
        
      if (retval >= 0)
        {
          ior = mateconf_get_daemon_ior();
            
          if (ior == NULL)
            retval = write (lock->lock_fd, "none", 4);
          else
            retval = write (lock->lock_fd, ior, strlen (ior));
        }

      if (retval < 0)
        {
          mateconf_set_error (err,
                           MATECONF_ERROR_LOCK_FAILED,
                           _("Can't write to file `%s': %s"),
                           lock->iorfile, g_strerror (errno));

          g_unlink (lock->iorfile);
          mateconf_lock_destroy (lock);

          return NULL;
        }
    }

  return lock;
}

MateConfLock*
mateconf_get_lock (const gchar *lock_directory,
                GError     **err)
{
  return mateconf_get_lock_or_current_holder (lock_directory, NULL, err);
}

gboolean
mateconf_release_lock (MateConfLock *lock,
                    GError   **err)
{
  gboolean retval;
  char *uniquefile;
  
  retval = FALSE;
  uniquefile = NULL;
  
  /* A paranoia check to avoid disaster if e.g.
   * some random client code opened and closed the
   * lockfile (maybe Nautilus checking its MIME type or
   * something)
   */
  if (lock->lock_fd < 0 ||
      file_locked_by_someone_else (lock->lock_fd))
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("We didn't have the lock on file `%s', but we should have"),
                   lock->iorfile);
      goto out;
    }

#ifndef G_OS_WIN32

  /* To avoid annoying .nfs3435314513453145 files on unlink, which keep us
   * from removing the lock directory, we don't want to hold the
   * lockfile open after removing all links to it. But we can't
   * close it then unlink, because then we would be unlinking without
   * holding the lock. So, we create a unique filename and link it too
   * the locked file, then unlink the locked file, then drop our locks
   * and close file descriptors, then unlink the unique filename
   */
  
  uniquefile = unique_filename (lock->lock_directory);

  if (link (lock->iorfile, uniquefile) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Failed to link '%s' to '%s': %s"),
                   uniquefile, lock->iorfile, g_strerror (errno));

      goto out;
    }
  
  /* Note that we unlink while still holding the lock to avoid races */
  if (unlink (lock->iorfile) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Failed to remove lock file `%s': %s"),
                   lock->iorfile,
                   g_strerror (errno));
      goto out;
    }

#endif

  /* Now drop our lock */
  if (lock->lock_fd >= 0)
    {
      close (lock->lock_fd);
      lock->lock_fd = -1;
    }

#ifndef G_OS_WIN32

  /* Now remove the temporary link we used to avoid .nfs351453 garbage */
  if (unlink (uniquefile) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Failed to clean up file '%s': %s"),
                   uniquefile, g_strerror (errno));

      goto out;
    }

#endif

  /* And finally clean up the directory - this would have failed if
   * we had .nfs323423423 junk
   */
  if (g_rmdir (lock->lock_directory) < 0)
    {
      g_set_error (err,
                   MATECONF_ERROR,
                   MATECONF_ERROR_FAILED,
                   _("Failed to remove lock directory `%s': %s"),
                   lock->lock_directory,
                   g_strerror (errno));
      goto out;
    }

  retval = TRUE;
  
 out:

  g_free (uniquefile);
  mateconf_lock_destroy (lock);
  return retval;
}

/* This function doesn't try to see if the lock is valid or anything
 * of the sort; it just reads it. It does do the object_to_string
 */
ConfigServer
mateconf_get_current_lock_holder  (const gchar *lock_directory,
                                GString     *failure_log)
{
  char *iorfile;
  ConfigServer server;

  iorfile = g_strconcat (lock_directory, "/ior", NULL);
  server = read_current_server_and_set_warning (iorfile, failure_log);
  g_free (iorfile);
  return server;
}

void
mateconf_daemon_blow_away_locks (void)
{
  char *lock_directory;
  char *iorfile;
  
  lock_directory = mateconf_get_lock_dir ();

  iorfile = g_strconcat (lock_directory, "/ior", NULL);

  if (g_unlink (iorfile) < 0)
    g_printerr (_("Failed to unlink lock file %s: %s\n"),
                iorfile, g_strerror (errno));

  g_free (iorfile);
  g_free (lock_directory);
}

static CORBA_ORB mateconf_orb = CORBA_OBJECT_NIL;      

CORBA_ORB
mateconf_orb_get (void)
{
  if (mateconf_orb == CORBA_OBJECT_NIL)
    {
      CORBA_Environment ev;
      int argc = 1;
      char *argv[] = { "mateconf", NULL };

      _mateconf_init_i18n ();
      
      CORBA_exception_init (&ev);
      
      mateconf_orb = CORBA_ORB_init (&argc, argv, "matecorba-local-orb", &ev);
      g_assert (ev._major == CORBA_NO_EXCEPTION);

      CORBA_exception_free (&ev);
    }

  return mateconf_orb;
}

int
mateconf_orb_release (void)
{
  int ret = 0;

  if (mateconf_orb != CORBA_OBJECT_NIL)
    {
      CORBA_ORB orb = mateconf_orb;
      CORBA_Environment ev;

      mateconf_orb = CORBA_OBJECT_NIL;

      CORBA_exception_init (&ev);

      CORBA_ORB_destroy (orb, &ev);
      CORBA_Object_release ((CORBA_Object)orb, &ev);

      if (ev._major != CORBA_NO_EXCEPTION)
        {
          ret = 1;
        }
      CORBA_exception_free (&ev);
    }

  return ret;
}

char*
mateconf_get_daemon_dir (void)
{  
  if (mateconf_use_local_locks ())
    {
      char *s;
      char *subdir;

      subdir = g_strconcat ("mateconfd-", g_get_user_name (), NULL);
      
      s = g_build_filename (g_get_tmp_dir (), subdir, NULL);

      g_free (subdir);

      return s;
    }
  else
    {
#ifndef G_OS_WIN32
      const char *home = g_get_home_dir ();
#else
      const char *home = _mateconf_win32_get_home_dir ();
#endif
      return g_strconcat (home, "/.mateconfd", NULL);
    }
}

char*
mateconf_get_lock_dir (void)
{
  char *mateconfd_dir;
  char *lock_dir;
  
  mateconfd_dir = mateconf_get_daemon_dir ();
  lock_dir = g_strconcat (mateconfd_dir, "/lock", NULL);

  g_free (mateconfd_dir);
  return lock_dir;
}

#if defined (F_SETFD) && defined (FD_CLOEXEC)

static void
set_cloexec (gint fd)
{
  fcntl (fd, F_SETFD, FD_CLOEXEC);
}

static void
close_fd_func (gpointer data)
{
  int *pipes = data;
  
  gint open_max;
  gint i;
  
  open_max = sysconf (_SC_OPEN_MAX);
  for (i = 3; i < open_max; i++)
    {
      /* don't close our write pipe */
      if (i != pipes[1])
        set_cloexec (i);
    }
}

#else

#define close_fd_func NULL

#endif

ConfigServer
mateconf_activate_server (gboolean  start_if_not_found,
                       GError  **error)
{
  ConfigServer server = CORBA_OBJECT_NIL;
  int p[2] = { -1, -1 };
  char buf[1];
  GError *tmp_err;
  char *argv[3];
  char *mateconfd_dir;
  char *lock_dir;
  GString *failure_log;
  struct stat statbuf;
  CORBA_Environment ev;
  gboolean dir_accessible;

  failure_log = g_string_new (NULL);
  
  mateconfd_dir = mateconf_get_daemon_dir ();
  
  dir_accessible = g_stat (mateconfd_dir, &statbuf) >= 0;

  if (!dir_accessible && errno != ENOENT)
    {
      server = CORBA_OBJECT_NIL;
      mateconf_log (GCL_WARNING, _("Failed to stat %s: %s"),
		 mateconfd_dir, g_strerror (errno));
    }
  else if (dir_accessible)
    {
      g_string_append (failure_log, " 1: ");
      lock_dir = mateconf_get_lock_dir ();
      server = mateconf_get_current_lock_holder (lock_dir, failure_log);
      g_free (lock_dir);

      /* Confirm server exists */
      CORBA_exception_init (&ev);

      if (!CORBA_Object_is_nil (server, &ev))
	{
	  ConfigServer_ping (server, &ev);
      
	  if (ev._major != CORBA_NO_EXCEPTION)
	    {
	      server = CORBA_OBJECT_NIL;

	      g_string_append_printf (failure_log,
				      _("Server ping error: %s"),
				      CORBA_exception_id (&ev));
	    }
	}

      CORBA_exception_free (&ev);
  
      if (server != CORBA_OBJECT_NIL)
	{
	  g_string_free (failure_log, TRUE);
	  g_free (mateconfd_dir);
	  return server;
	}
    }

  g_free (mateconfd_dir);

  if (start_if_not_found)
    {
      /* Spawn server */
      if (pipe (p) < 0)
        {
          g_set_error (error,
                       MATECONF_ERROR,
                       MATECONF_ERROR_NO_SERVER,
                       _("Failed to create pipe for communicating with spawned mateconf daemon: %s\n"),
                       g_strerror (errno));
          goto out;
        }

      argv[0] = g_build_filename (MATECONF_SERVERDIR, MATECONFD, NULL);
      argv[1] = g_strdup_printf ("%d", p[1]);
      argv[2] = NULL;
  
      tmp_err = NULL;
      if (!g_spawn_async (NULL,
                          argv,
                          NULL,
                          G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                          close_fd_func,
			  p,
                          NULL,
                          &tmp_err))
        {
          g_free (argv[0]);
          g_free (argv[1]);
          g_set_error (error,
                       MATECONF_ERROR,
                       MATECONF_ERROR_NO_SERVER,
                       _("Failed to launch configuration server: %s\n"),
                       tmp_err->message);
          g_error_free (tmp_err);
          goto out;
        }
      
      g_free (argv[0]);
      g_free (argv[1]);
  
      /* Block until server starts up */
      read (p[0], buf, 1);

      g_string_append (failure_log, " 2: ");
      lock_dir = mateconf_get_lock_dir ();
      server = mateconf_get_current_lock_holder (lock_dir, failure_log);
      g_free (lock_dir);
    }
  
 out:
  if (server == CORBA_OBJECT_NIL &&
      error &&
      *error == NULL)
    g_set_error (error,
                 MATECONF_ERROR,
                 MATECONF_ERROR_NO_SERVER,
                 _("Failed to contact configuration server; some possible causes are that you need to enable TCP/IP networking for MateCORBA, or you have stale NFS locks due to a system crash. See http://www.mate.org/projects/mateconf/ for information. (Details - %s)"),
                 failure_log->len > 0 ? failure_log->str : _("none"));

  g_string_free (failure_log, TRUE);
  
  if (p[0] != -1)
    close (p[0]);
  if (p[1] != -1)
    close (p[1]);
  
  return server;
}

gboolean
mateconf_CORBA_Object_equal (gconstpointer a, gconstpointer b)
{
  CORBA_Environment ev;
  CORBA_Object _obj_a = (gpointer)a;
  CORBA_Object _obj_b = (gpointer)b;
  gboolean retval;

  CORBA_exception_init (&ev);
  retval = CORBA_Object_is_equivalent(_obj_a, _obj_b, &ev);
  CORBA_exception_free (&ev);

  return retval;
}

guint
mateconf_CORBA_Object_hash (gconstpointer key)
{
  CORBA_Environment ev;
  CORBA_Object _obj = (gpointer)key;
  CORBA_unsigned_long retval;

  CORBA_exception_init (&ev);
  retval = CORBA_Object_hash(_obj, G_MAXUINT, &ev);
  CORBA_exception_free (&ev);

  return retval;
}

#endif /* HAVE_CORBA */

void
_mateconf_init_i18n (void)
{
  static gboolean done = FALSE;

  if (!done)
    {
      bindtextdomain (GETTEXT_PACKAGE, MATECONF_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
      bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
      done = TRUE;
    }
}

enum { UNKNOWN, LOCAL, NORMAL };

gboolean
mateconf_use_local_locks (void)
{
  static int local_locks = UNKNOWN;
  
  if (local_locks == UNKNOWN)
    {
      const char *l =
        g_getenv ("MATECONF_GLOBAL_LOCKS");

      if (l && atoi (l) == 1)
        local_locks = NORMAL;
      else
        local_locks = LOCAL;
    }

  return local_locks == LOCAL;
}

/* Fake implementations of those. */
MateConfLock*
mateconf_get_lock (const gchar *lock_directory,
                GError     **err)
{
  return (MateConfLock *) g_strdup ("fake");
}

gboolean
mateconf_release_lock (MateConfLock *lock,
                    GError   **err)
{
  g_free (lock);
  return TRUE;
}
