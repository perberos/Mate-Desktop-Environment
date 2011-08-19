#include <config.h>

#include "selection.h"
#include "proctable.h"
#include "util.h"

namespace procman
{
  void SelectionMemento::add_to_selected(GtkTreeModel* model, GtkTreePath*, GtkTreeIter* iter, gpointer data)
  {
    guint pid = 0;
    gtk_tree_model_get(model, iter, COL_PID, &pid, -1);
    if (pid)
      static_cast<SelectionMemento*>(data)->pids.push_back(pid);
  }


  void SelectionMemento::save(GtkWidget* tree)
  {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_selected_foreach(selection, &SelectionMemento::add_to_selected, this);
  }


  void SelectionMemento::restore(GtkWidget* tree)
  {
    if (not this->pids.empty())
      {
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	typedef std::vector<pid_t>::iterator iterator;
	for (iterator it(this->pids.begin()); it != this->pids.end(); ++it)
	  {
	    if (ProcInfo* proc = ProcInfo::find(*it))
	      {
		gtk_tree_selection_select_iter(selection, &proc->node);
		procman_debug("Re-selected process %u", unsigned(*it));
	      }
	    else
		procman_debug("Could not find process %u, cannot re-select it", unsigned(*it));
	  }
      }
  }
}
