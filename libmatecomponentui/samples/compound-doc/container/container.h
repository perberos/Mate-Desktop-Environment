#ifndef SAMPLE_CONTAINER_H
#define SAMPLE_CONTAINER_H

#include <matecomponent/matecomponent-ui-container.h>
#include "document.h"
#include "doc-view.h"

typedef struct _SampleApp        SampleApp;
typedef struct _SampleClientSite SampleClientSite;

struct _SampleApp {
	SampleDoc		*doc;

	GList			*doc_views;

	GtkWidget		*curr_view;
	GtkWidget		*win;
	GtkWidget		*box;
	GtkWidget		*fileselection;
};

void              sample_app_exit             (SampleApp        *app);

#endif /* SAMPLE_CONTAINER_H */

