/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2005 John (J5) Palmieri <johnp@redhat.com>
 * Copyright (C) 2010 Red Hat, Inc.
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

#ifndef _ENGINES_H_
#define _ENGINES_H_

#include <gtk/gtk.h>

typedef void    (*UrlClickedCb) (GtkWindow * nw, const char *url);

GtkWindow      *theme_create_notification        (UrlClickedCb url_clicked_cb);
void            theme_destroy_notification       (GtkWindow   *nw);
void            theme_show_notification          (GtkWindow   *nw);
void            theme_hide_notification          (GtkWindow   *nw);
void            theme_set_notification_hints     (GtkWindow   *nw,
                                                  GHashTable  *hints);
void            theme_set_notification_timeout   (GtkWindow   *nw,
                                                  glong        timeout);
void            theme_notification_tick          (GtkWindow   *nw,
                                                  glong        remaining);
void            theme_set_notification_text      (GtkWindow   *nw,
                                                  const char  *summary,
                                                  const char  *body);
void            theme_set_notification_icon      (GtkWindow   *nw,
                                                  GdkPixbuf   *pixbuf);
void            theme_set_notification_arrow     (GtkWindow   *nw,
                                                  gboolean     visible,
                                                  int          x,
                                                  int          y);
void            theme_add_notification_action    (GtkWindow   *nw,
                                                  const char  *label,
                                                  const char  *key,
                                                  GCallback    cb);
void            theme_clear_notification_actions (GtkWindow   *nw);
void            theme_move_notification          (GtkWindow   *nw,
                                                  int          x,
                                                  int          y);
gboolean        theme_get_always_stack           (GtkWindow   *nw);

#endif /* _ENGINES_H_ */
