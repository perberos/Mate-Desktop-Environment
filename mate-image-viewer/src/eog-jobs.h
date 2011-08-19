/* Eye Of Mate - Jobs
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-jobs.h) by:
 * 	- Martin Kretzschmar <martink@mate.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EOG_JOBS_H__
#define __EOG_JOBS_H__

#include "eog-list-store.h"
#include "eog-transform.h"
#include "eog-enums.h"

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __EOG_IMAGE_DECLR__
#define __EOG_IMAGE_DECLR__
  typedef struct _EogImage EogImage;
#endif

#ifndef __EOG_URI_CONVERTER_DECLR__
#define __EOG_URI_CONVERTER_DECLR__
typedef struct _EogURIConverter EogURIConverter;
#endif

#ifndef __EOG_JOB_DECLR__
#define __EOG_JOB_DECLR__
typedef struct _EogJob EogJob;
#endif
typedef struct _EogJobClass EogJobClass;

typedef struct _EogJobThumbnail EogJobThumbnail;
typedef struct _EogJobThumbnailClass EogJobThumbnailClass;

typedef struct _EogJobLoad EogJobLoad;
typedef struct _EogJobLoadClass EogJobLoadClass;

typedef struct _EogJobModel EogJobModel;
typedef struct _EogJobModelClass EogJobModelClass;

typedef struct _EogJobTransform EogJobTransform;
typedef struct _EogJobTransformClass EogJobTransformClass;

typedef struct _EogJobSave EogJobSave;
typedef struct _EogJobSaveClass EogJobSaveClass;

typedef struct _EogJobSaveAs EogJobSaveAs;
typedef struct _EogJobSaveAsClass EogJobSaveAsClass;

typedef struct _EogJobCopy EogJobCopy;
typedef struct _EogJobCopyClass EogJobCopyClass;

#define EOG_TYPE_JOB		       (eog_job_get_type())
#define EOG_JOB(obj)		       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB, EogJob))
#define EOG_JOB_CLASS(klass)	       (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB, EogJobClass))
#define EOG_IS_JOB(obj)	               (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB))
#define EOG_JOB_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), EOG_TYPE_JOB, EogJobClass))

#define EOG_TYPE_JOB_THUMBNAIL	       (eog_job_thumbnail_get_type())
#define EOG_JOB_THUMBNAIL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_THUMBNAIL, EogJobThumbnail))
#define EOG_JOB_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB_THUMBNAIL, EogJobThumbnailClass))
#define EOG_IS_JOB_THUMBNAIL(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_THUMBNAIL))

#define EOG_TYPE_JOB_LOAD	       (eog_job_load_get_type())
#define EOG_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_LOAD, EogJobLoad))
#define EOG_JOB_LOAD_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB_LOAD, EogJobLoadClass))
#define EOG_IS_JOB_LOAD(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_LOAD))

#define EOG_TYPE_JOB_MODEL	       (eog_job_model_get_type())
#define EOG_JOB_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_MODEL, EogJobModel))
#define EOG_JOB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB_MODEL, EogJobModelClass))
#define EOG_IS_JOB_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_MODEL))

#define EOG_TYPE_JOB_TRANSFORM	       (eog_job_transform_get_type())
#define EOG_JOB_TRANSFORM(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_TRANSFORM, EogJobTransform))
#define EOG_JOB_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB_TRANSFORM, EogJobTransformClass))
#define EOG_IS_JOB_TRANSFORM(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_TRANSFORM))

#define EOG_TYPE_JOB_SAVE              (eog_job_save_get_type())
#define EOG_JOB_SAVE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_SAVE, EogJobSave))
#define EOG_JOB_SAVE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), EOG_TYPE_JOB_SAVE, EogJobSaveClass))
#define EOG_IS_JOB_SAVE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_SAVE))
#define EOG_JOB_SAVE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), EOG_TYPE_JOB_SAVE, EogJobSaveClass))

#define EOG_TYPE_JOB_SAVE_AS           (eog_job_save_as_get_type())
#define EOG_JOB_SAVE_AS(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_SAVE_AS, EogJobSaveAs))
#define EOG_JOB_SAVE_AS_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass), EOG_TYPE_JOB_SAVE_AS, EogJobSaveAsClass))
#define EOG_IS_JOB_SAVE_AS(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_SAVE_AS))

#define EOG_TYPE_JOB_COPY	       (eog_job_copy_get_type())
#define EOG_JOB_COPY(obj)	       (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_JOB_COPY, EogJobCopy))
#define EOG_JOB_COPY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_JOB_COPY, EogJobCopyClass))
#define EOG_IS_JOB_COPY(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_JOB_COPY))


struct _EogJob
{
	GObject  parent;

	GError   *error;
	GMutex   *mutex;
	float     progress;
	gboolean  finished;
};

struct _EogJobClass
{
	GObjectClass parent_class;

	void    (* finished) (EogJob *job);
	void    (* progress) (EogJob *job, float progress);
	void    (*run)       (EogJob *job);
};

struct _EogJobThumbnail
{
	EogJob       parent;
	EogImage    *image;
	GdkPixbuf   *thumbnail;
};

struct _EogJobThumbnailClass
{
	EogJobClass parent_class;
};

struct _EogJobLoad
{
	EogJob        parent;
	EogImage     *image;
	EogImageData  data;
};

struct _EogJobLoadClass
{
	EogJobClass parent_class;
};

struct _EogJobModel
{
	EogJob        parent;
	EogListStore *store;
	GSList       *file_list;
};

struct _EogJobModelClass
{
        EogJobClass parent_class;
};

struct _EogJobTransform
{
	EogJob        parent;
	GList        *images;
	EogTransform *trans;
};

struct _EogJobTransformClass
{
        EogJobClass parent_class;
};

typedef enum {
	EOG_SAVE_RESPONSE_NONE,
	EOG_SAVE_RESPONSE_RETRY,
	EOG_SAVE_RESPONSE_SKIP,
	EOG_SAVE_RESPONSE_OVERWRITE,
	EOG_SAVE_RESPONSE_CANCEL,
	EOG_SAVE_RESPONSE_LAST
} EogJobSaveResponse;

struct _EogJobSave
{
	EogJob    parent;
	GList	 *images;
	guint      current_pos;
	EogImage *current_image;
};

struct _EogJobSaveClass
{
	EogJobClass parent_class;
};

struct _EogJobSaveAs
{
	EogJobSave       parent;
	EogURIConverter *converter;
	GFile           *file;
};

struct _EogJobSaveAsClass
{
	EogJobSaveClass parent;
};

struct _EogJobCopy
{
	EogJob parent;
	GList *images;
	guint current_pos;
	gchar *dest;
};

struct _EogJobCopyClass
{
	EogJobClass parent_class;
};

/* base job class */
GType           eog_job_get_type           (void) G_GNUC_CONST;
void            eog_job_finished           (EogJob          *job);
void            eog_job_run                (EogJob          *job);
void            eog_job_set_progress       (EogJob          *job,
					    float            progress);

/* EogJobThumbnail */
GType           eog_job_thumbnail_get_type (void) G_GNUC_CONST;
EogJob         *eog_job_thumbnail_new      (EogImage     *image);

/* EogJobLoad */
GType           eog_job_load_get_type      (void) G_GNUC_CONST;
EogJob 	       *eog_job_load_new 	   (EogImage        *image,
					    EogImageData     data);

/* EogJobModel */
GType 		eog_job_model_get_type     (void) G_GNUC_CONST;
EogJob 	       *eog_job_model_new          (GSList          *file_list);

/* EogJobTransform */
GType 		eog_job_transform_get_type (void) G_GNUC_CONST;
EogJob 	       *eog_job_transform_new      (GList           *images,
					    EogTransform    *trans);

/* EogJobSave */
GType		eog_job_save_get_type      (void) G_GNUC_CONST;
EogJob         *eog_job_save_new           (GList           *images);

/* EogJobSaveAs */
GType		eog_job_save_as_get_type   (void) G_GNUC_CONST;
EogJob         *eog_job_save_as_new        (GList           *images,
					    EogURIConverter *converter,
					    GFile           *file);

/*EogJobCopy */
GType          eog_job_copy_get_type      (void) G_GNUC_CONST;
EogJob        *eog_job_copy_new           (GList            *images,
					   const gchar      *dest);

G_END_DECLS

#endif /* __EOG_JOBS_H__ */
