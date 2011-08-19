#ifndef SAMPLE_DOC_H
#define SAMPLE_DOC_H

#include <glib-object.h>
#include "component.h"

#define SAMPLE_DOC_TYPE        (sample_doc_get_type ())
#define SAMPLE_DOC(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), SAMPLE_DOC_TYPE, SampleDoc))
#define SAMPLE_DOC_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), SAMPLE_DOC_TYPE, SampleDocClass))
#define SAMPLE_IS_DOC(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), SAMPLE_DOC_TYPE))
#define SAMPLE_IS_DOC_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SAMPLE_DOC_TYPE))

typedef struct _SampleDoc SampleDoc;
typedef struct _SampleDocPrivate SampleDocPrivate;

struct _SampleDoc {
	GObject			 parent;

	SampleDocPrivate 	*priv;
};

typedef struct {
	GObjectClass	 parent_class;

	/* Signals */
	void (*component_added) (SampleDoc *, SampleComponent *);
	void (*component_removed) (SampleDoc *, SampleComponent *);

} SampleDocClass;

GType 		 sample_doc_get_type (void);

SampleDoc 	*sample_doc_new (void);

gchar		*sample_doc_get_filename (SampleDoc *);

gboolean	 sample_doc_is_dirty (SampleDoc *);

void		 sample_doc_add_component (SampleDoc *, gchar *iid);

GList		*sample_doc_get_components (SampleDoc *);

void		 sample_doc_load (SampleDoc *, const gchar *filename);

void		 sample_doc_save (SampleDoc *, const gchar *filename);

#endif
