/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-drive.h
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#if !defined (__GDU_GTK_INSIDE_GDU_GTK_H) && !defined (GDU_GTK_COMPILATION)
#error "Only <gdu-gtk/gdu-gtk.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_GTK_TYPES_H
#define __GDU_GTK_TYPES_H

#include <glib-object.h>
#include <gdu/gdu.h>
#include <gtk/gtk.h>
#include <gdu-gtk/gdu-gtk-enums.h>

G_BEGIN_DECLS

typedef struct GduSample                      GduSample;
typedef struct GduColor                       GduColor;
typedef struct GduCurve                       GduCurve;
typedef struct GduGraph                       GduGraph;
typedef struct GduTimeLabel                   GduTimeLabel;
typedef struct GduAtaSmartDialog              GduAtaSmartDialog;
typedef struct GduSpinner                     GduSpinner;
typedef struct GduPoolTreeModel               GduPoolTreeModel;
typedef struct GduPoolTreeView                GduPoolTreeView;
typedef struct GduCreateLinuxMdDialog         GduCreateLinuxMdDialog;
typedef struct GduSizeWidget                  GduSizeWidget;
typedef struct GduVolumeGrid                  GduVolumeGrid;
typedef struct GduDetailsTable                GduDetailsTable;
typedef struct GduDetailsElement              GduDetailsElement;
typedef struct GduErrorDialog                 GduErrorDialog;
typedef struct GduConfirmationDialog          GduConfirmationDialog;
typedef struct GduButtonElement               GduButtonElement;
typedef struct GduButtonTable                 GduButtonTable;
typedef struct GduDialog                      GduDialog;
typedef struct GduEditPartitionDialog         GduEditPartitionDialog;
typedef struct GduFormatDialog                GduFormatDialog;
typedef struct GduPartitionDialog             GduPartitionDialog;
typedef struct GduCreatePartitionDialog       GduCreatePartitionDialog;
typedef struct GduCreateLinuxLvm2VolumeDialog GduCreateLinuxLvm2VolumeDialog;
typedef struct GduEditNameDialog              GduEditNameDialog;
typedef struct GduDiskSelectionWidget         GduDiskSelectionWidget;
typedef struct GduAddComponentLinuxMdDialog   GduAddComponentLinuxMdDialog;
typedef struct GduEditLinuxMdDialog           GduEditLinuxMdDialog;
typedef struct GduDriveBenchmarkDialog        GduDriveBenchmarkDialog;
typedef struct GduConnectToServerDialog       GduConnectToServerDialog;
typedef struct GduEditLinuxLvm2Dialog         GduEditLinuxLvm2Dialog;
typedef struct GduAddPvLinuxLvm2Dialog        GduAddPvLinuxLvm2Dialog;

G_END_DECLS

#endif /* __GDU_GTK_TYPES_H */
