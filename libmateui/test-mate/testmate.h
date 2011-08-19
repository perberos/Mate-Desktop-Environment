#ifndef TESTMATE_H
#define TESTMATE_H

#include <matecomponent/matecomponent-object.h>
#include <matecomponent/matecomponent-ui-container.h>
#include <matecomponent/matecomponent-ui-component.h>

typedef struct _TestMateApp		TestMateApp;

struct _TestMateApp {
	MateComponentUIContainer	*ui_container;
	MateComponentUIComponent	*ui_component;

	GtkWidget		*app;

};

void    sample_app_exit         (TestMateApp        *app);

#endif /* TESTMATE_H */
