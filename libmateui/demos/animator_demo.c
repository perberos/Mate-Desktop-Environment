#include <config.h>
#include <mate.h>

#define ANIMFILE "mailcheck/email.png"
/* #define ANIMFILE "mate-ppp-animation-large.png" */

static void quit_cb (GtkWidget * widget, void *data);
static void toggle_start_stop_cb (GtkWidget * widget, void *data);

static void
quit_cb (GtkWidget * widget, void *data)
{
  gtk_main_quit ();
}

static void
toggle_start_stop_cb (GtkWidget * widget, gpointer data)
{
  GtkWidget *tmp;
  MateAnimatorStatus status;

  tmp = (GtkWidget *) data;

  status = mate_animator_get_status (MATE_ANIMATOR (tmp));
  if (status == MATE_ANIMATOR_STATUS_STOPPED)
    mate_animator_start (MATE_ANIMATOR (tmp));
  else
    mate_animator_stop (MATE_ANIMATOR (tmp));
}

int
main (int argc, char *argv[])
{
  MateProgram *program;
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *animator;
  GdkPixbuf *pixbuf;
  char *s;

  program = mate_program_init ("mate-animator", VERSION, &libmateui_module_info,
		      argc, argv, NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (quit_cb), NULL);

  s = mate_program_locate_file (mate_program_get (),
				 MATE_FILE_DOMAIN_PIXMAP,
				 ANIMFILE, TRUE, NULL);
  pixbuf = gdk_pixbuf_new_from_file (s, NULL);
  g_free (s);
  animator = mate_animator_new_with_size (48, 48);
  mate_animator_append_frames_from_pixbuf (MATE_ANIMATOR (animator),
					    pixbuf, 0, 0, 150, 48);
  gdk_pixbuf_unref (pixbuf);
/*  mate_animator_set_range (MATE_ANIMATOR (animator), 1, 21); */
  mate_animator_set_loop_type (MATE_ANIMATOR (animator),
				MATE_ANIMATOR_LOOP_RESTART);
  mate_animator_set_playback_speed (MATE_ANIMATOR (animator), 5.0);

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), animator);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (toggle_start_stop_cb), animator);

  gtk_container_add (GTK_CONTAINER (window), button);
  gtk_widget_show_all (window);

  mate_animator_start (MATE_ANIMATOR (animator));

  gtk_main ();

  g_object_unref (program);

  return 0;
}
