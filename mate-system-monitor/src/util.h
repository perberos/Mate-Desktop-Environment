// -*- mode: c++ -*-

#ifndef H_MATE_SYSTEM_MONITOR_UTIL_1123178725
#define H_MATE_SYSTEM_MONITOR_UTIL_1123178725

#include <glib.h>
#include <gtk/gtk.h>
#include <stddef.h>
#include <cstring>
#include <string>
#include <functional>
#include <algorithm>

using std::string;

template<typename T>
inline int procman_cmp(T x, T y)
{
	if (x == y)
		return 0;

	if (x < y)
		return -1;

	return 1;
}

#define PROCMAN_CMP(X, Y) procman_cmp((X), (Y))
#define PROCMAN_RCMP(X, Y) procman_cmp((Y), (X));

GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid);

gboolean
load_symbols(const char *module, ...) G_GNUC_NULL_TERMINATED;


void
procman_debug_real(const char *file, int line, const char *func,
		   const char *format, ...) G_GNUC_PRINTF(4, 5);

#define procman_debug(FMT, ...) procman_debug_real(__FILE__, __LINE__, __func__, FMT, ##__VA_ARGS__)

inline string make_string(char *c_str)
{
	if (!c_str) {
		procman_debug("NULL string");
		return string();
	}

	string s(c_str);
	g_free(c_str);
	return s;
}




template<typename Map>
class UnrefMapValues
  : public std::unary_function<void, Map>
{
public:
  void operator()(const typename Map::value_type &it) const
  {
    if (it.second)
      g_object_unref(it.second);
  }
};


template<typename Map>
inline void unref_map_values(Map &map)
{
  std::for_each(map.begin(), map.end(), UnrefMapValues<Map>());
}


namespace procman
{
  void size_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
			   GtkTreeModel *model, GtkTreeIter *iter,
			   gpointer user_data);

  void size_na_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
			      GtkTreeModel *model, GtkTreeIter *iter,
			      gpointer user_data);

  void duration_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
			       GtkTreeModel *model, GtkTreeIter *iter,
			       gpointer user_data);

  void time_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
			   GtkTreeModel *model, GtkTreeIter *iter,
			   gpointer user_data);

  void status_cell_data_func(GtkTreeViewColumn *col, GtkCellRenderer *renderer,
			     GtkTreeModel *model, GtkTreeIter *iter,
			     gpointer user_data);

  template<typename T>
  void poison(T &t, char c)
  {
    memset(&t, c, sizeof t);
  }



  //
  // Stuff to update a tree_store in a smart way
  //

  template<typename T>
  void tree_store_update(GtkTreeModel* model, GtkTreeIter* iter, int column, const T& new_value)
  {
    T current_value;

    gtk_tree_model_get(model, iter, column, &current_value, -1);

    if (current_value != new_value)
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, column, new_value, -1);
  }

  // undefined
  // catch every thing about pointers
  // just to make sure i'm not doing anything wrong
  template<typename T>
  void tree_store_update(GtkTreeModel* model, GtkTreeIter* iter, int column, T* new_value);

  // specialized versions for strings
  template<>
  void tree_store_update<const char>(GtkTreeModel* model, GtkTreeIter* iter, int column, const char* new_value);

  template<>
  inline void tree_store_update<char>(GtkTreeModel* model, GtkTreeIter* iter, int column, char* new_value)
  {
    tree_store_update<const char>(model, iter, column, new_value);
  }

  gchar* format_size(guint64 size, guint64 max = 0, bool want_bits = false);

  std::string format_rate(guint64 rate, guint64 max_rate = 0, bool want_bits = false);

  std::string format_network(guint64 rate, guint64 max_rate = 0);
  std::string format_network_rate(guint64 rate, guint64 max_rate = 0);
}


#endif /* H_MATE_SYSTEM_MONITOR_UTIL_1123178725 */
