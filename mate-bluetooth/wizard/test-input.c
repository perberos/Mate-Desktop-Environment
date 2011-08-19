#include <gtk/gtk.h>
#include <bluetooth-input.h>

static void
keyboard_appeared_cb (BluetoothInput *input, gpointer data)
{
	g_message ("keyboard_appeared_cb");
}

static void
keyboard_disappeared_cb (BluetoothInput *input, gpointer data)
{
	g_message ("keyboard_disappeared_cb");
}

static void
mouse_appeared_cb (BluetoothInput *input, gpointer data)
{
	g_message ("mouse_appeared_cb");
}

static void
mouse_disappeared_cb (BluetoothInput *input, gpointer data)
{
	g_message ("mouse_disappeared_cb");
}

int main (int argc, char **argv)
{
	BluetoothInput *input;

	gtk_init (&argc, &argv);

	input = bluetooth_input_new ();
	if (input == NULL)
		return 1;

	g_signal_connect (G_OBJECT (input), "keyboard-appeared",
			  G_CALLBACK (keyboard_appeared_cb), NULL);
	g_signal_connect (G_OBJECT (input), "keyboard-disappeared",
			  G_CALLBACK (keyboard_disappeared_cb), NULL);
	g_signal_connect (G_OBJECT (input), "mouse-appeared",
			  G_CALLBACK (mouse_appeared_cb), NULL);
	g_signal_connect (G_OBJECT (input), "mouse-disappeared",
			  G_CALLBACK (mouse_disappeared_cb), NULL);

	bluetooth_input_check_for_devices (input);

	gtk_main ();

	return 0;
}
