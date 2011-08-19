#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include "bluetooth-plugin-manager.h"

static gchar *bdaddr = NULL;
static gchar **uuids = NULL;
static gboolean list_uuid = FALSE;

static gboolean
delete_event_cb (GtkWidget *widget,
		 GdkEvent  *event,
		 gpointer   user_data)
{
	gtk_main_quit ();
	return FALSE;
}

static GOptionEntry options[] = {
	{ "device", 0, 0, G_OPTION_ARG_STRING, &bdaddr,
				"Remote device to use", "ADDRESS" },
	{ "uuid", 0, 0, G_OPTION_ARG_STRING_ARRAY, &uuids,
				"UUID(s) to test against", "UUID" },
	{ "list-uuids", 0, 0, G_OPTION_ARG_NONE, &list_uuid,
				"List valid UUIDs", NULL },
	{ NULL },
};

int main (int argc, char **argv)
{
	GtkWidget *window, *vbox;
	GList *list, *l;
	DBusGConnection *bus;
	GError *error = NULL;

	if (gtk_init_with_args (&argc, &argv, NULL,
				options, NULL, &error) == FALSE) {
		if (error != NULL) {
			g_printerr ("%s\n", error->message);
			g_error_free (error);
		} else
			g_printerr ("An unknown error occurred\n");

		return 1;
	}

	if (list_uuid) {
		g_print ("Valid UUIDs are:\n");
		/* UUIDs copied from bluetooth-client.c and sorted (do not translate them) */
		g_print ("AudioSink\n" \
			 "AudioSource\n" \
			 "A/V_RemoteControl\n" \
			 "A/V_RemoteControlTarget\n" \
			 "DialupNetworking\n" \
			 "GenericAudio\n" \
			 "GenericNetworking\n" \
			 "GN\n" \
			 "HandsfreeAudioGateway\n" \
			 "Handsfree\n" \
			 "Headset_-_AG\n" \
			 "HSP\n" \
			 "HumanInterfaceDeviceService\n" \
			 "IrMCSync\n" \
			 "NAP\n" \
			 "OBEXFileTransfer\n" \
			 "OBEXObjectPush\n" \
			 "PANU\n" \
			 "Phonebook_Access_-_PSE\n" \
			 "PnPInformation\n" \
			 "SEMC HLA\n" \
			 "SEMC Watch Phone\n" \
			 "SerialPort\n" \
			 "ServiceDiscoveryServerServiceClassID\n" \
			 "SIM_Access\n" \
			 "VideoSource\n");

		return 0;
	}

	/* Init the dbus-glib types */
	bus = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	dbus_g_connection_unref (bus);

	bluetooth_plugin_manager_init ();

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (delete_event_cb), NULL);
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	list = bluetooth_plugin_manager_get_widgets (bdaddr, (const char **) uuids);
	if (list == NULL) {
		g_message ("no plugins");
		return 1;
	}

	for (l = list; l != NULL; l = l->next) {
		GtkWidget *widget = l->data;
		gtk_container_add (GTK_CONTAINER (vbox), widget);
	}

	gtk_widget_show_all (window);

	gtk_main ();

	gtk_widget_destroy (window);

	bluetooth_plugin_manager_cleanup ();

	return 0;
}
