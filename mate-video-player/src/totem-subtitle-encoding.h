/* Encoding stuff */

#ifndef TOTEM_SUBTITLE_ENCODING_H
#define TOTEM_SUBTITLE_ENCODING_H

#include <gtk/gtk.h>

void totem_subtitle_encoding_init (GtkComboBox *combo);
void totem_subtitle_encoding_set (GtkComboBox *combo, const char *encoding);
const char * totem_subtitle_encoding_get_selected (GtkComboBox *combo);

#endif /* SUBTITLE_ENCODING_H */
