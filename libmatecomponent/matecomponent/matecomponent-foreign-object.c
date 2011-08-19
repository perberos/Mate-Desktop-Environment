/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <matecomponent/matecomponent-foreign-object.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-running-context.h>

static void
matecomponent_foreign_object_class_init (MateComponentForeignObjectClass *klass)
{
}

static void
matecomponent_foreign_object_instance_init (GObject    *g_object,
				     GTypeClass *klass)
{
}

GType
matecomponent_foreign_object_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeInfo info = {
			sizeof (MateComponentForeignObjectClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) matecomponent_foreign_object_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MateComponentForeignObject),
			0, /* n_preallocs */
			(GInstanceInitFunc) matecomponent_foreign_object_instance_init
		};
		
		type = g_type_register_static (MATECOMPONENT_TYPE_OBJECT, "MateComponentForeignObject",
					       &info, 0);
	}

	return type;
}


MateComponentObject *
matecomponent_foreign_object_new (CORBA_Object corba_objref)
{
	MateComponentObject *object;
	CORBA_Environment ev[1];

	g_return_val_if_fail (corba_objref != CORBA_OBJECT_NIL, NULL);

	CORBA_exception_init (ev);
	if (!CORBA_Object_is_a (corba_objref, "IDL:MateComponent/Unknown:1.0", ev)) {
		if (ev->_major != CORBA_NO_EXCEPTION) {
			char *text = matecomponent_exception_get_text (ev);
			g_warning ("CORBA_Object_is_a: %s", text);
			g_free (text);
		} else
			g_warning ("matecomponent_foreign_object_new: corba_objref"
				   " doesn't have interface MateComponent::Unknown");
		object = NULL;

	} else {
		object = MATECOMPONENT_OBJECT (g_object_new (MATECOMPONENT_TYPE_FOREIGN_OBJECT, NULL));
		object->corba_objref = CORBA_Object_duplicate (corba_objref, NULL);
		matecomponent_running_context_add_object_T (object->corba_objref);
	}
	CORBA_exception_free (ev);

	return object;
}

