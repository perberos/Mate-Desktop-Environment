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

#ifndef __UTILS_DEBUG_H__
#define __UTILS_DEBUG_H__

#ifdef __GNUC__
// we have to mark this as a system header because otherwise GCC barfs 
// on variadic macros.
#pragma GCC system_header
#endif

#include <assert.h>
#include "base/macros.hpp"

namespace utils {

#ifdef DEBUG
#define DBG_OUT(x, ...) \
  ::utils::dbg_print(x,  __FUNCTION__, ## __VA_ARGS__)
#else
#define DBG_OUT(x, ...)   
#endif

#ifdef DEBUG
#define DBG_ASSERT(cond, reason)  \
  ::utils::dbg_assert(cond, #cond, __FILE__, __LINE__, reason)
#else
#define DBG_ASSERT(cond, reason)  \
  assert(cond)
#endif

#define ERR_OUT(x, ...) \
  ::utils::err_print(x,  __FUNCTION__, ## __VA_ARGS__)


  /** print debug messages. printf format.
   * NOOP if DEBUG is not defined.
   * Call with the DBG_OUT macro
   * @param fmt the formt string, printf style
   * @param func the func name
   */
  void dbg_print(const char* fmt, const char* func, ...)
    _PRINTF_FORMAT(1,3);

  /** assert 
   * @param condvalue the value of the assert, true, assert
   * @param cond the text of the condition
   * @param filen the file name __FILE__
   * @param linen the line number __LINE__
   * @param reason the reason of the assert
   */
  void dbg_assert(bool condvalue, const char* cond, const char* filen,
          int linen, const char* reason);

  /** print error message. printf format.
   * Call with the ERR_OUT macro.
   * @param fmt the formt string, printf style
   * @param func the func name
   */
  void err_print(const char *fmt, const char* func, ...)
    _PRINTF_FORMAT(1,3);

}

#endif
