#ifndef SAMPLE_COMPONENT_H
#define SAMPLE_COMPONENT_H

#include <matecomponent/MateComponent.h>
#include <glib-object.h>

#define SAMPLE_COMPONENT_TYPE        (sample_component_get_type ())
#define SAMPLE_COMPONENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), SAMPLE_COMPONENT_TYPE, SampleComponent))
#define SAMPLE_COMPONENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), SAMPLE_COMPONENT_TYPE, SampleComponentClass))
#define SAMPLE_IS_COMPONENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), SAMPLE_COMPONENT_TYPE))
#define SAMPLE_IS_COMPONENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), SAMPLE_COMPONENT_TYPE))

typedef struct _SampleComponent 	SampleComponent;
typedef struct _SampleComponentClass 	SampleComponentClass;
typedef struct _SampleComponentPrivate 	SampleComponentPrivate;

struct _SampleComponent {
	GObject parent;

	SampleComponentPrivate	*priv;
};

struct _SampleComponentClass {
	GObjectClass parent_class;

	void (*changed) (GObject *obj);
};

GType		 sample_component_get_type (void);

SampleComponent *sample_component_new (gchar *iid);

SampleComponent *sample_component_new_from_storage (MateComponent_Storage storage);

void		 sample_component_move (SampleComponent *, 
					gdouble x, gdouble y);

void		 sample_component_resize (SampleComponent *, 
					  gdouble width, gdouble height);

gboolean	 sample_component_is_dirty (SampleComponent *);

void		 sample_component_get_affine (SampleComponent *, gdouble *);

MateComponent_Unknown	 sample_component_get_server (SampleComponent *);

#endif
