#ifndef H_MATE_SYSTEM_MONITOR_SELECTION_H_1183113337
#define H_MATE_SYSTEM_MONITOR_SELECTION_H_1183113337

#include <sys/types.h>
#include <gtk/gtk.h>
#include <vector>

namespace procman
{
  class SelectionMemento
  {
    std::vector<pid_t> pids;
    static void add_to_selected(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data);

  public:
    void save(GtkWidget* tree);
    void restore(GtkWidget* tree);
  };
}

#endif /* H_MATE_SYSTEM_MONITOR_SELECTION_H_1183113337 */
