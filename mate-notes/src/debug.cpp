/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * niepce - utils/debug.cpp
 *
 * Copyright (C) 2007 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <pthread.h>

#if defined(NDEBUG)
#define _SAVENDEBUG NDEBUG
#undef NDEBUG
#endif
// we make sure assert is always defined.
#include <assert.h>
// but we need to save the state.
#if defined(_SAVENDEBUG)
#define NDEBUG _SAVENDEBUG
#endif

#include "debug.hpp"


namespace utils {

  static void _vprint(const char *prefix, const char *fmt, 
                     const char* func,  va_list marker);
  static void _print(const char *prefix, const char *fmt, 
             const char* func, ...);

  void dbg_print(const char *fmt, const char* func, ...)
  {
#ifdef DEBUG
#define DEBUG_MSG "DEBUG: "
    va_list marker;
    
    va_start(marker, func);
    // TODO make this atomic
    _vprint(DEBUG_MSG, fmt, func, marker);

    va_end(marker);
    
#undef DEBUG_MSG
#endif
  }


  /** assert 
   * 
   */
  void dbg_assert(bool condvalue, const char* cond, const char* filen,
          int linen, const char* reason)
  {
    if(!condvalue) {
      _print("ASSERT: ", "%s:%d %s", cond, filen, linen, reason);

    }
  }


  void err_print(const char *fmt, const char* func, ...)
  {
#define ERROR_MSG "ERROR: "
    va_list marker;
    
    va_start(marker, func);
    // TODO make this atomic
    _vprint(ERROR_MSG, fmt, func, marker);

    va_end(marker);
    
#undef ERROR_MSG
  }


  static void _print(const char *prefix, const char *fmt, 
              const char* func, ...)
  {
    va_list marker;
    
    va_start(marker, func);

    _vprint(prefix, fmt, func, marker);

    va_end(marker);
  }

  static void _vprint(const char *prefix, const char *fmt, 
              const char* func,  va_list marker)
  {
//    static boost::recursive_mutex mutex;
//    boost::recursive_mutex::scoped_lock lock(mutex);
    char buf[128];
    snprintf(buf, 128, "(%d) ", (int)pthread_self());
    fwrite(buf, 1, strlen(buf), stderr);
    fwrite(prefix, 1, strlen(prefix), stderr);

    if(func) {
      fwrite(func, 1, strlen(func), stderr);
      fwrite(" - ", 1, 3, stderr);
    }

    vfprintf(stderr, fmt, marker);
    fprintf(stderr, "\n");
  }

}
