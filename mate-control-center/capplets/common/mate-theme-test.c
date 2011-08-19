#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libmate/mate-desktop-item.h>
#include "mate-theme-info.h"

int
main (int argc, char *argv[])
{
  GList *themes, *list;

  g_thread_init (NULL);
  gtk_init (&argc, &argv);
  mate_theme_init ();

  themes = mate_theme_meta_info_find_all ();
  if (themes == NULL)
    {
      g_print ("No meta themes were found.\n");
    }
  else
    {
      g_print ("%d meta themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeMetaInfo *meta_theme_info;

	  meta_theme_info = list->data;
	  g_print ("\t%s\n", meta_theme_info->readable_name);
	}
    }
  g_list_free (themes);

  themes = mate_theme_icon_info_find_all ();
  if (themes == NULL)
    {
      g_print ("No icon themes were found.\n");
    }
  else
    {
      g_print ("%d icon themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeIconInfo *icon_theme_info;

	  icon_theme_info = list->data;
	  g_print ("\t%s\n", icon_theme_info->name);
	}
    }
  g_list_free (themes);

  themes = mate_theme_info_find_by_type (MATE_THEME_MARCO);
  if (themes == NULL)
    {
      g_print ("No marco themes were found.\n");
    }
  else
    {
      g_print ("%d marco themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeInfo *theme_info;

	  theme_info = list->data;
	  g_print ("\t%s\n", theme_info->name);
	}
    }
  g_list_free (themes);

  themes = mate_theme_info_find_by_type (MATE_THEME_GTK_2);
  if (themes == NULL)
    {
      gchar *str;

      g_print ("No gtk-2 themes were found.  The following directories were tested:\n");
      str = gtk_rc_get_theme_dir ();
      g_print ("\t%s\n", str);
      g_free (str);
      str = g_build_filename (g_get_home_dir (), ".themes", NULL);
      g_print ("\t%s\n", str);
      g_free (str);
    }
  else
    {
      g_print ("%d gtk-2 themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeInfo *theme_info;

	  theme_info = list->data;
	  g_print ("\t%s\n", theme_info->name);
	}
    }
  g_list_free (themes);

  themes = mate_theme_info_find_by_type (MATE_THEME_GTK_2_KEYBINDING);
  if (themes == NULL)
    {
      g_print ("No keybinding themes were found.\n");
    }
  else
    {
      g_print ("%d keybinding themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeInfo *theme_info;

	  theme_info = list->data;
	  g_print ("\t%s\n", theme_info->name);
	}
    }
  g_list_free (themes);

  themes = mate_theme_cursor_info_find_all ();
  if (themes == NULL)
    {
      g_print ("No cursor themes were found.\n");
    }
  else
    {
      g_print ("%d cursor themes were found:\n", g_list_length (themes));
      for (list = themes; list; list = list->next)
	{
	  MateThemeCursorInfo *cursor_theme_info;

	  cursor_theme_info = list->data;
	  g_print ("\t%s\n", cursor_theme_info->name);
	}
    }
  g_list_free (themes);

  return 0;
}

