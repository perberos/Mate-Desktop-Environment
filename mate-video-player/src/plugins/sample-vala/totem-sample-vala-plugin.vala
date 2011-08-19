using GLib;
using Totem;

class SampleValaPlugin: Totem.Plugin {
	public override bool activate (Totem.Object totem) throws GLib.Error {
		stdout.printf ("Hello world\n");
		return true;
	}

	public override void deactivate (Totem.Object totem) {
		stdout.printf ("Goodbye world\n");
	}
}


[ModuleInit]
public GLib.Type register_totem_plugin (GLib.TypeModule module)
{
	stdout.printf ("Registering plugin %s\n", "SampleValaPlugin");

	return typeof (SampleValaPlugin);
}
