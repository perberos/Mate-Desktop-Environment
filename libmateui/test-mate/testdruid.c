#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <libmateui.h>
#include <libmateui/mate-druid.h>
#include <libmateui/mate-druid-page.h>
#include <libmateui/mate-druid-page-standard.h>


int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *druid;
  GtkWidget *druid_page;
  gtk_init (&argc, &argv);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  druid = mate_druid_new ();
  gtk_container_add (GTK_CONTAINER (window), druid);
  druid_page = mate_druid_page_standard_new_with_vals ("Test Druid", NULL, NULL);
  mate_druid_append_page (MATE_DRUID (druid), MATE_DRUID_PAGE (druid_page));
  mate_druid_page_standard_append_item (MATE_DRUID_PAGE_STANDARD (druid_page),
					 "Test _one:",
					 gtk_entry_new (),
					 "Longer information here");
  mate_druid_page_standard_append_item (MATE_DRUID_PAGE_STANDARD (druid_page),
					 "Test _two:",
					 gtk_entry_new (),
					 "Longer information here");
  mate_druid_page_standard_append_item (MATE_DRUID_PAGE_STANDARD (druid_page),
					 "Test t_hree:",
					 gtk_entry_new (),
					 "Longer information here");
  mate_druid_page_standard_append_item (MATE_DRUID_PAGE_STANDARD (druid_page),
					 "Test fou_r:",
					 gtk_entry_new (),
					 "Longer information here");
  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}
