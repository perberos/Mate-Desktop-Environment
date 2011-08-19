/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 *  MateComponent::Zoomable - zoomable interface for Controls.
 *
 *  Authors: Maciej Stachowiak <mjs@eazel.com>
 *           Martin Baulig <baulig@suse.de>
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *                2000 SuSE GmbH.
 */
#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-zoomable.h>
#include <matecomponent/matecomponent-property-bag.h>

#undef ZOOMABLE_DEBUG

static GObjectClass *matecomponent_zoomable_parent_class = NULL;

struct _MateComponentZoomablePrivate {
	CORBA_float		 zoom_level;

	CORBA_float		 min_zoom_level;
	CORBA_float		 max_zoom_level;
	CORBA_boolean		 has_min_zoom_level;
	CORBA_boolean		 has_max_zoom_level;
	CORBA_boolean		 is_continuous;

	GArray                  *pref_levels;
	GPtrArray               *pref_names;

	MateComponent_ZoomableFrame	 zoomable_frame;
};

enum {
	SET_FRAME,
	SET_ZOOM_LEVEL,
	ZOOM_IN,
	ZOOM_OUT,
	ZOOM_TO_FIT,
	ZOOM_TO_DEFAULT,
	LAST_SIGNAL
};

enum {
	PARAM_0,
	PARAM_ZOOM_LEVEL,
	PARAM_MIN_ZOOM_LEVEL,
	PARAM_MAX_ZOOM_LEVEL,
	PARAM_HAS_MIN_ZOOM_LEVEL,
	PARAM_HAS_MAX_ZOOM_LEVEL,
	PARAM_IS_CONTINUOUS
};

static guint signals [LAST_SIGNAL];

#define CLASS(o) MATECOMPONENT_ZOOMABLE_GET_CLASS(o)

static inline MateComponentZoomable *
matecomponent_zoomable_from_servant (PortableServer_Servant servant)
{
	return MATECOMPONENT_ZOOMABLE (matecomponent_object_from_servant (servant));
}

static CORBA_float
impl_MateComponent_Zoomable__get_level (PortableServer_Servant  servant,
				 CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->zoom_level;
}

static CORBA_float
impl_MateComponent_Zoomable__get_minLevel (PortableServer_Servant  servant,
				    CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->min_zoom_level;
}

static CORBA_float
impl_MateComponent_Zoomable__get_maxLevel (PortableServer_Servant  servant,
				    CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->max_zoom_level;
}

static CORBA_boolean
impl_MateComponent_Zoomable__get_hasMinLevel (PortableServer_Servant  servant,
				       CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->has_min_zoom_level;
}

static CORBA_boolean
impl_MateComponent_Zoomable__get_hasMaxLevel (PortableServer_Servant  servant,
				       CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->has_max_zoom_level;
}

static CORBA_boolean
impl_MateComponent_Zoomable__get_isContinuous (PortableServer_Servant  servant,
					CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	return zoomable->priv->is_continuous;
}

static MateComponent_ZoomLevelList *
impl_MateComponent_Zoomable__get_preferredLevels (PortableServer_Servant  servant,
					   CORBA_Environment      *ev)
{
	int                   levels;
	MateComponentZoomable       *zoomable;
	MateComponent_ZoomLevelList *list;

	zoomable = matecomponent_zoomable_from_servant (servant);
	levels = zoomable->priv->pref_levels->len;

	list = MateComponent_ZoomLevelList__alloc ();
	list->_length  = levels;
	list->_buffer  = CORBA_sequence_MateComponent_ZoomLevel_allocbuf (levels);

	memcpy (list->_buffer, zoomable->priv->pref_levels->data,
		sizeof (CORBA_float) * levels);
	
	CORBA_sequence_set_release (list, CORBA_TRUE);

	return list;
}

static MateComponent_ZoomLevelNameList *
impl_MateComponent_Zoomable__get_preferredLevelNames (PortableServer_Servant servant,
					       CORBA_Environment     *ev)
{
	int levels,i;
	gchar **names;
	MateComponentZoomable *zoomable;
	MateComponent_ZoomLevelNameList *list;

	zoomable = matecomponent_zoomable_from_servant (servant);

	levels = zoomable->priv->pref_names->len;
	names = (gchar **) zoomable->priv->pref_names->pdata;

	list = MateComponent_ZoomLevelNameList__alloc ();
	list->_length  = levels;
	list->_buffer  = CORBA_sequence_MateComponent_ZoomLevelName_allocbuf (levels);

	for (i = 0; i < levels; ++i)
		list->_buffer [i] = CORBA_string_dup (names [i]);

	CORBA_sequence_set_release (list, CORBA_TRUE);

	return list;
}

static void 
impl_MateComponent_Zoomable_setLevel (PortableServer_Servant  servant,
			       const CORBA_float       zoom_level,
			       CORBA_Environment      *ev)
{
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_signal_emit (G_OBJECT (zoomable), signals [SET_ZOOM_LEVEL],
		       0, zoom_level);
}

static void
impl_MateComponent_Zoomable_zoomIn (PortableServer_Servant  servant,
			     CORBA_Environment      *ev)
{	
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_signal_emit (G_OBJECT (zoomable), signals [ZOOM_IN], 0);
}

static void
impl_MateComponent_Zoomable_zoomOut (PortableServer_Servant  servant,
			      CORBA_Environment      *ev)
{	
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_signal_emit (G_OBJECT (zoomable), signals [ZOOM_OUT], 0);
}

static void
impl_MateComponent_Zoomable_zoomFit (PortableServer_Servant  servant,
			      CORBA_Environment      *ev)
{	
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_signal_emit (G_OBJECT (zoomable), signals [ZOOM_TO_FIT], 0);
}

static void
impl_MateComponent_Zoomable_zoomDefault (PortableServer_Servant  servant,
				  CORBA_Environment      *ev)
{	
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_signal_emit (G_OBJECT (zoomable), signals [ZOOM_TO_DEFAULT], 0);
}

static void
impl_MateComponent_Zoomable_setFrame (PortableServer_Servant  servant,
			       MateComponent_ZoomableFrame    zoomable_frame,
			       CORBA_Environment      *ev)
{	
	MateComponentZoomable *zoomable = matecomponent_zoomable_from_servant (servant);

	g_assert (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL);

	zoomable->priv->zoomable_frame = CORBA_Object_duplicate (zoomable_frame, ev);

	g_signal_emit (G_OBJECT (zoomable), signals [SET_FRAME], 0);
}


static void
matecomponent_zoomable_get_property (GObject    *object,
			      guint       property_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
	MateComponentZoomable *zoomable = MATECOMPONENT_ZOOMABLE (object);
	MateComponentZoomablePrivate *priv = zoomable->priv;

	switch (property_id) {
	case PARAM_ZOOM_LEVEL:
		g_value_set_float (value, priv->zoom_level);
		break;
	case PARAM_MIN_ZOOM_LEVEL:
		g_value_set_float (value, priv->min_zoom_level);
		break;
	case PARAM_MAX_ZOOM_LEVEL:
		g_value_set_float (value, priv->max_zoom_level);
		break;
	case PARAM_HAS_MIN_ZOOM_LEVEL:
		g_value_set_boolean (value, priv->has_min_zoom_level);;
		break;
	case PARAM_HAS_MAX_ZOOM_LEVEL:
		g_value_set_boolean (value, priv->has_max_zoom_level);;
		break;
	case PARAM_IS_CONTINUOUS:
		g_value_set_boolean (value, priv->is_continuous);
		break;
	default:
		g_message ("Unknown property_id `%d'", property_id);
		break;
	};
}

static void
matecomponent_zoomable_free_preferred_zoom_level_arrays (MateComponentZoomable *zoomable)
{
	if (zoomable->priv->pref_names) {
		int        i;
		GPtrArray *array;

		array = zoomable->priv->pref_names;

		for (i = 0; i < array->len; i++)
			g_free (g_ptr_array_index (array, i));
	
		g_ptr_array_free (array, TRUE);
		zoomable->priv->pref_names = NULL;
	}

	if (zoomable->priv->pref_levels) {
		g_array_free (zoomable->priv->pref_levels, TRUE);
		zoomable->priv->pref_levels = NULL;
	}
}

static void
matecomponent_zoomable_dispose (GObject *object)
{
	MateComponentZoomable *zoomable = (MateComponentZoomable *) object;

	matecomponent_zoomable_free_preferred_zoom_level_arrays (zoomable);

	matecomponent_zoomable_parent_class->dispose (object);
}

static void
matecomponent_zoomable_finalize (GObject *object)
{
	MateComponentZoomable *zoomable = (MateComponentZoomable *) object;

	g_free (zoomable->priv);

	matecomponent_zoomable_parent_class->finalize (object);
}

static void
matecomponent_zoomable_class_init (MateComponentZoomableClass *klass)
{
	POA_MateComponent_Zoomable__epv *epv = &klass->epv;
	GObjectClass *object_class;
	
	object_class = (GObjectClass *) klass;
	
	matecomponent_zoomable_parent_class = g_type_class_peek_parent (klass);

	object_class->get_property = matecomponent_zoomable_get_property;
	object_class->dispose  = matecomponent_zoomable_dispose;
	object_class->finalize = matecomponent_zoomable_finalize;

	g_object_class_install_property (
		object_class,
		PARAM_ZOOM_LEVEL,
		g_param_spec_float ("zoom_level",
				    _("Zoom level"),
				    _("The degree of enlargment"),
				    0, G_MAXFLOAT, 1.0,
				    G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PARAM_MIN_ZOOM_LEVEL,
		g_param_spec_float ("min_zoom_level",
				    _("Minimum Zoom level"),
				    _("The minimum degree of enlargment"),
				    0, G_MAXFLOAT, 0.0,
				    G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PARAM_MAX_ZOOM_LEVEL,
		g_param_spec_float ("max_zoom_level",
				    _("Maximum Zoom level"),
				    _("The maximum degree of enlargment"),
				    0, G_MAXFLOAT, 0.0,
				    G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PARAM_HAS_MIN_ZOOM_LEVEL,
		g_param_spec_boolean ("has_min_zoom_level",
				    _("Has a minimum Zoom level"),
				    _("Whether we have a valid minimum zoom level"),
				      FALSE, G_PARAM_READABLE));
				    
	g_object_class_install_property (
		object_class,
		PARAM_HAS_MAX_ZOOM_LEVEL,
		g_param_spec_boolean ("has_max_zoom_level",
				    _("Has a maximum Zoom level"),
				    _("Whether we have a valid maximum zoom level"),
				      FALSE, G_PARAM_READABLE));

	g_object_class_install_property (
		object_class,
		PARAM_IS_CONTINUOUS,
		g_param_spec_boolean ("is_continuous",
				    _("Is continuous"),
				    _("Whether we zoom continuously (as opposed to jumps)"),
				      FALSE, G_PARAM_READABLE));
				    
	signals [SET_FRAME] =
		g_signal_new ("set_frame",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, set_frame),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [SET_ZOOM_LEVEL] =
		g_signal_new ("set_zoom_level",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, set_zoom_level),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 1, G_TYPE_FLOAT);
	signals [ZOOM_IN] = 
		g_signal_new ("zoom_in",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, zoom_in),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [ZOOM_OUT] = 
		g_signal_new ("zoom_out",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, zoom_out),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [ZOOM_TO_FIT] = 
		g_signal_new ("zoom_to_fit",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, zoom_to_fit),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [ZOOM_TO_DEFAULT] = 
		g_signal_new ("zoom_to_default",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateComponentZoomableClass, zoom_to_default),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	epv->_get_level = impl_MateComponent_Zoomable__get_level;
	epv->_get_minLevel = impl_MateComponent_Zoomable__get_minLevel;
	epv->_get_maxLevel = impl_MateComponent_Zoomable__get_maxLevel;
	epv->_get_hasMinLevel = impl_MateComponent_Zoomable__get_hasMinLevel;
	epv->_get_hasMaxLevel = impl_MateComponent_Zoomable__get_hasMaxLevel;
	epv->_get_isContinuous = impl_MateComponent_Zoomable__get_isContinuous;
	epv->_get_preferredLevels = impl_MateComponent_Zoomable__get_preferredLevels;
	epv->_get_preferredLevelNames = impl_MateComponent_Zoomable__get_preferredLevelNames;

	epv->zoomIn      = impl_MateComponent_Zoomable_zoomIn;
	epv->zoomOut     = impl_MateComponent_Zoomable_zoomOut;
	epv->zoomFit     = impl_MateComponent_Zoomable_zoomFit;
	epv->zoomDefault = impl_MateComponent_Zoomable_zoomDefault;

	epv->setLevel = impl_MateComponent_Zoomable_setLevel;
	epv->setFrame = impl_MateComponent_Zoomable_setFrame;
}

static void
matecomponent_zoomable_init (MateComponentZoomable *zoomable)
{
	zoomable->priv = g_new0 (MateComponentZoomablePrivate, 1);

	zoomable->priv->zoom_level = 0.0;
	zoomable->priv->min_zoom_level = 0.0;
	zoomable->priv->max_zoom_level = 0.0;
	zoomable->priv->has_min_zoom_level = FALSE;
	zoomable->priv->has_max_zoom_level = FALSE;
	zoomable->priv->is_continuous = TRUE;
	zoomable->priv->pref_levels = g_array_new (FALSE, TRUE, sizeof (CORBA_float));
	zoomable->priv->pref_names = g_ptr_array_new ();
}

MATECOMPONENT_TYPE_FUNC_FULL (MateComponentZoomable, MateComponent_Zoomable, MATECOMPONENT_TYPE_OBJECT, matecomponent_zoomable)

/**
 * matecomponent_zoomable_set_parameters_full:
 * 
 * This is used by the component to set new zooming parameters (and to set the
 * initial zooming parameters including the initial zoom level after creating
 * the MateComponentZoomable) - for instance after loading a new file.
 *
 * If any of the zoom parameters such as the minimum or maximum zoom level has
 * changed, it is likely that the zoom level has become invalid as well - at
 * least, the container must query it in any case, so we set it here.
 * 
 * Return value: 
 **/
void
matecomponent_zoomable_set_parameters_full (MateComponentZoomable  *zoomable,
				     float            zoom_level,
				     float            min_zoom_level,
				     float            max_zoom_level,
				     gboolean         has_min_zoom_level,
				     gboolean         has_max_zoom_level,
				     gboolean         is_continuous,
				     float           *pref_levels,
				     const gchar    **pref_names,
				     gint             num_pref_levels)
{
	MateComponentZoomablePrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_ZOOMABLE (zoomable));

	priv = zoomable->priv;

	priv->zoom_level = zoom_level;
	priv->min_zoom_level = min_zoom_level;
	priv->max_zoom_level = max_zoom_level;
	priv->has_min_zoom_level = has_min_zoom_level;
	priv->has_max_zoom_level = has_max_zoom_level;
	priv->is_continuous = is_continuous;

	matecomponent_zoomable_free_preferred_zoom_level_arrays (zoomable);
	
	priv->pref_levels = g_array_new (FALSE, TRUE, sizeof (CORBA_float));
	
	if (pref_levels)
		g_array_append_vals (priv->pref_levels,
				     pref_levels,
				     num_pref_levels);
	
	priv->pref_names = g_ptr_array_new ();
	
	if (pref_names) {
		int     i;
		gchar **p;

		g_ptr_array_set_size (priv->pref_names, num_pref_levels);
		
		p = (gchar **) priv->pref_names->pdata;

		for (i = 0; i < num_pref_levels; i++)
			p [i] = g_strdup (pref_names [i]);
	}
}

/**
 * matecomponent_zoomable_set_parameters:
 * 
 * This is a simple version of @matecomponent_zoomable_set_parameters_full()
 * for components which support continuous zooming. It does not
 * override any of the parameters which can only be set by the _full
 * version.
 **/
void
matecomponent_zoomable_set_parameters (MateComponentZoomable  *zoomable,
                                float            zoom_level,
                                float            min_zoom_level,
                                float            max_zoom_level,
                                gboolean         has_min_zoom_level,
                                gboolean         has_max_zoom_level)
{
	MateComponentZoomablePrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_ZOOMABLE (zoomable));

	priv = zoomable->priv;

	priv->zoom_level = zoom_level;
	priv->min_zoom_level = min_zoom_level;
	priv->max_zoom_level = max_zoom_level;
	priv->has_min_zoom_level = has_min_zoom_level;
	priv->has_max_zoom_level = has_max_zoom_level;
}

/**
 * matecomponent_zoomable_add_preferred_zoom_level:
 * @zoomable: the zoomable
 * @zoom_level: the new level
 * @zoom_level_name: the new level's name
 * 
 * This appends a new zoom level's name and value to the
 * internal list of these.
 **/
void
matecomponent_zoomable_add_preferred_zoom_level (MateComponentZoomable *zoomable,
                                          CORBA_float     zoom_level,
                                          const gchar    *zoom_level_name)
{
	gchar *name;
	
	g_array_append_val (zoomable->priv->pref_levels, zoom_level);

	name = g_strdup (zoom_level_name);
	g_ptr_array_add (zoomable->priv->pref_names, name);
}


/**
 * matecomponent_zoomable_new:
 * 
 * Create a new matecomponent-zoomable implementing MateComponentObject
 * interface.
 * 
 * Return value: 
 **/
MateComponentZoomable *
matecomponent_zoomable_new (void)
{
	return g_object_new (matecomponent_zoomable_get_type (), NULL);
}

/**
 * matecomponent_zoomable_report_zoom_level_changed:
 *
 * @new_zoom_level: The new zoom level.
 * 
 * Reports the MateComponentZoomableFrame that the zoom level has changed (but the
 * other zoom parameters are still the same).
 *
 * This is called after the component has successfully completed a zooming
 * operation - the @new_zoom_level may have been modified from what the
 * container requested to match what the component actually displays at the
 * moment.
 **/
void
matecomponent_zoomable_report_zoom_level_changed (MateComponentZoomable    *zoomable,
					   float              new_zoom_level,
					   CORBA_Environment *opt_ev)
{
	CORBA_Environment *ev, temp_ev;

	g_return_if_fail (MATECOMPONENT_IS_ZOOMABLE (zoomable));

	zoomable->priv->zoom_level = new_zoom_level;

	if (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL)
		return;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_ZoomableFrame_onLevelChanged (
		zoomable->priv->zoomable_frame,
		zoomable->priv->zoom_level, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);
}

/**
 * matecomponent_zoomable_report_zoom_parameters_changed:
 *
 * Reports the MateComponentZoomableFrame that the zoom parameters have changed;
 * this also includes the zoom level.
 *
 * On the container side (the MateComponentZoomableFrame) this implies that the
 * zoom level has changed as well, so you need to query the MateComponentZoomable
 * for the new zoom level as well.
 **/
void
matecomponent_zoomable_report_zoom_parameters_changed (MateComponentZoomable    *zoomable,
						CORBA_Environment *opt_ev)
{
	CORBA_Environment *ev, temp_ev;

	g_return_if_fail (MATECOMPONENT_IS_ZOOMABLE (zoomable));

	if (zoomable->priv->zoomable_frame == CORBA_OBJECT_NIL)
		return;

	if (!opt_ev) {
		CORBA_exception_init (&temp_ev);
		ev = &temp_ev;
	} else
		ev = opt_ev;

	MateComponent_ZoomableFrame_onParametersChanged (
		zoomable->priv->zoomable_frame, ev);

	if (!opt_ev)
		CORBA_exception_free (&temp_ev);
}
