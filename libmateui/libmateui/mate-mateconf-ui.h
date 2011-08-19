/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* MATE GUI Library - mate-mateconf.h
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Jonathan Blandford  <jrb@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef MATE_MATECONF_UI_H
#define MATE_MATECONF_UI_H

/*
 * This is a private libmateui headerfile
 */

#include <libmate/mate-program.h>
#include <libmate/mate-mateconf.h>

/* MATE MateConf UI module; basically what this builds on
   the MateConf module from libmate, adding a GUI error box,
   etc..
*/

#define mate_mateconf_ui_module_info_get _mate_mateconf_ui_module_info_get
const MateModuleInfo * _mate_mateconf_ui_module_info_get	(void) G_GNUC_CONST;

#define mateui_mateconf_lazy_init _mateui_mateconf_lazy_init
void			_mateui_mateconf_lazy_init	(void);

#define mate_mateconf_get_bool _mate_mateconf_get_bool
gboolean		_mate_mateconf_get_bool		(const char *key);

#endif /* MATE_MATECONF_UI_H */
