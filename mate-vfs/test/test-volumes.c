#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-volume-monitor.h>
#include <libmatevfs/mate-vfs-volume.h>

static GMainLoop *loop;

static void
print_volume (MateVFSVolume *volume)
{
	char *path, *uri, *name, *icon;
	MateVFSDrive *drive;

	path = mate_vfs_volume_get_device_path (volume);
	uri = mate_vfs_volume_get_activation_uri (volume);
	icon = mate_vfs_volume_get_icon (volume);
	name = mate_vfs_volume_get_display_name (volume);
	drive = mate_vfs_volume_get_drive (volume);
	g_print ("vol(%p)[dev: %s, mount: %s, device type: %d, handles_trash: %d, icon: %s, name: %s, user_visible: %d, drive: %p]\n",
		 volume,
		 path?path:"(nil)",
		 uri,
		 mate_vfs_volume_get_device_type (volume),
		 mate_vfs_volume_handles_trash (volume),
		 icon,
		 name,
		 mate_vfs_volume_is_user_visible (volume),
		 drive);
	g_free (path);
	g_free (uri);
	g_free (icon);
	g_free (name);
	mate_vfs_drive_unref (drive);
}

static void
print_drive (MateVFSDrive *drive)
{
	char *path, *uri, *name, *icon;
	MateVFSVolume *volume;

	path = mate_vfs_drive_get_device_path (drive);
	uri = mate_vfs_drive_get_activation_uri (drive);
	icon = mate_vfs_drive_get_icon (drive);
	name = mate_vfs_drive_get_display_name (drive);
	volume = mate_vfs_drive_get_mounted_volume (drive);
	
	g_print ("drive(%p)[dev: %s, mount: %s, device type: %d, icon: %s, name: %s, user_visible: %d, volume: %p]\n",
		 drive,
		 path?path:"(nil)",
		 uri,
		 mate_vfs_drive_get_device_type (drive),
		 icon,
		 name,
		 mate_vfs_drive_is_user_visible (drive),
		 volume);
	g_free (path);
	g_free (uri);
	g_free (icon);
	g_free (name);
	mate_vfs_volume_unref (volume);
}

static void
volume_mounted (MateVFSVolumeMonitor *volume_monitor,
		MateVFSVolume	       *volume)
{
	g_print ("Volume mounted: ");
	print_volume (volume);
}

static void
volume_unmounted (MateVFSVolumeMonitor *volume_monitor,
		  MateVFSVolume	       *volume)
{
	g_print ("Volume unmounted: ");
	print_volume (volume);
}

static void
drive_connected (MateVFSVolumeMonitor *volume_monitor,
		 MateVFSDrive	       *drive)
{
	g_print ("drive connected: ");
	print_drive (drive);
}

static void
drive_disconnected (MateVFSVolumeMonitor *volume_monitor,
		    MateVFSDrive	       *drive)
{
	g_print ("drive disconnected: ");
	print_drive (drive);
}

int
main (int argc, char *argv[])
{
  MateVFSVolumeMonitor *monitor;
  GList *l, *volumes, *drives;
  
  mate_vfs_init ();

  monitor = mate_vfs_get_volume_monitor ();

  g_signal_connect (monitor, "volume_mounted",
		    G_CALLBACK (volume_mounted), NULL);
  g_signal_connect (monitor, "volume_unmounted",
		    G_CALLBACK (volume_unmounted), NULL);
  g_signal_connect (monitor, "drive_connected",
		    G_CALLBACK (drive_connected), NULL);
  g_signal_connect (monitor, "drive_disconnected",
		    G_CALLBACK (drive_disconnected), NULL);
  
  volumes = mate_vfs_volume_monitor_get_mounted_volumes (monitor);

  g_print ("Mounted volumes:\n");
  for (l = volumes; l != NULL; l = l->next) {
	  print_volume (l->data);
	  mate_vfs_volume_unref (l->data);
  }
  g_list_free (volumes);

  drives = mate_vfs_volume_monitor_get_connected_drives (monitor);

  g_print ("Connected drives:\n");
  for (l = drives; l != NULL; l = l->next) {
	  print_drive (l->data);
	  mate_vfs_drive_unref (l->data);
  }
  g_list_free (drives);

  g_print ("Waiting for volume events:\n");
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  
  mate_vfs_shutdown ();
  
  return 0;
}
