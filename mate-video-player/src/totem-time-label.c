
#include "config.h"

#include "totem-time-label.h"
#include <glib/gi18n.h>
#include "video-utils.h"

struct _TotemTimeLabelPrivate {
	gint64 time;
	gint64 length;
	gboolean seeking;
};

G_DEFINE_TYPE (TotemTimeLabel, totem_time_label, GTK_TYPE_LABEL)
#define TOTEM_TIME_LABEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_TIME_LABEL, TotemTimeLabelPrivate))

static void
totem_time_label_init (TotemTimeLabel *label)
{
	char *time_string;
	label->priv = G_TYPE_INSTANCE_GET_PRIVATE (label, TOTEM_TYPE_TIME_LABEL, TotemTimeLabelPrivate);

	time_string = totem_time_to_string (0);
	gtk_label_set_text (GTK_LABEL (label), time_string);
	g_free (time_string);

	label->priv->time = 0;
	label->priv->length = -1;
	label->priv->seeking = FALSE;
}

GtkWidget*
totem_time_label_new (void)
{
	return GTK_WIDGET (g_object_new (TOTEM_TYPE_TIME_LABEL, NULL));
}

static void
totem_time_label_class_init (TotemTimeLabelClass *klass)
{
	g_type_class_add_private (klass, sizeof (TotemTimeLabelPrivate));
}

void
totem_time_label_set_time (TotemTimeLabel *label, gint64 _time, gint64 length)
{
	char *label_str;

	g_return_if_fail (TOTEM_IS_TIME_LABEL (label));

	if (_time / 1000 == label->priv->time / 1000
			&& length / 1000 == label->priv->length / 1000)
		return;

	if (length <= 0) {
		label_str = totem_time_to_string (_time);
	} else {
		char *time_str, *length_str;

		time_str = totem_time_to_string (_time);
		length_str = totem_time_to_string (length);
		if (label->priv->seeking == FALSE)
			/* Elapsed / Total Length */
			label_str = g_strdup_printf (_("%s / %s"), time_str, length_str);
		else 
			/* Seeking to Time / Total Length */
			label_str = g_strdup_printf (_("Seek to %s / %s"), time_str, length_str);
		g_free (time_str);
		g_free (length_str);
	}

	gtk_label_set_text (GTK_LABEL (label), label_str);
	g_free (label_str);

	label->priv->time = _time;
	label->priv->length = length;
}

void
totem_time_label_set_seeking (TotemTimeLabel *label, gboolean seeking)
{
	g_return_if_fail (TOTEM_IS_TIME_LABEL (label));

	label->priv->seeking = seeking;
}
