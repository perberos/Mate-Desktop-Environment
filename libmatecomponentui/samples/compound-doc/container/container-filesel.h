#ifndef SAMPLE_CONTAINER_FILESEL_H
#define SAMPLE_CONTAINER_FILESEL_H

#include "container.h"

void container_request_file (SampleApp    *app,
			     gboolean      save,
			     GCallback     cb,
			     gpointer      user_data);
#endif
