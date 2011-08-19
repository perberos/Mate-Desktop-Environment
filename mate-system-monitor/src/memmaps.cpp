#include <config.h>

#include <glibtop/procmap.h>
#include <glibtop/mountlist.h>
#include <sys/stat.h>
#include <glib/gi18n.h>

#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdexcept>

using std::string;


#include "procman.h"
#include "memmaps.h"
#include "proctable.h"
#include "util.h"


/* be careful with this enum, you could break the column names */
enum
{
	MMAP_COL_FILENAME,
	MMAP_COL_VMSTART,
	MMAP_COL_VMEND,
	MMAP_COL_VMSZ,
	MMAP_COL_FLAGS,
	MMAP_COL_VMOFFSET,
	MMAP_COL_PRIVATE_CLEAN,
	MMAP_COL_PRIVATE_DIRTY,
	MMAP_COL_SHARED_CLEAN,
	MMAP_COL_SHARED_DIRTY,
	MMAP_COL_DEVICE,
	MMAP_COL_INODE,
	MMAP_COL_MAX
};


namespace
{
  class OffsetFormater
  {
    string format;

  public:

    void set(const glibtop_map_entry &last_map)
    {
      this->format = (last_map.end <= G_MAXUINT32) ? "%08" G_GINT64_MODIFIER "x" : "%016" G_GINT64_MODIFIER "x";
    }

    string operator()(guint64 v) const
    {
      char buffer[17];
      g_snprintf(buffer, sizeof buffer, this->format.c_str(), v);
      return buffer;
    }
  };




#if 0

  struct ColumnState
  {
    unsigned visible;
    unsigned id;
    unsigned width;

    int pack() const
    {
      unsigned p = 0;
      p |= (this->visible & 0x0001) << 24;
      p |= (this->id      & 0x00ff) << 16;
      p |= (this->width   & 0xffff);
      return p;
    }

    void unpack(int i)
    {
      this->visible = 0x0001 & (i >> 24);
      this->id      = 0x00ff & (i >> 16);
      this->width   = 0xffff & i;
    }
  };


  void
  procman_save_tree_state2(MateConfClient *client, GtkWidget *tree, const gchar *cprefix)
  {
    const string prefix(cprefix);

    GtkTreeModel *model;
    gint sort_col;
    GtkSortType order;

    g_assert(tree);
    g_assert(prefix != "");

    model = gtk_tree_view_get_model(GTK_TREE_VIEW (tree));

    if (gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &sort_col, &order)) {
      mateconf_client_set_int(client, (prefix + "/sort_col").c_str(), sort_col, 0);
      mateconf_client_set_int(client, (prefix + "/sort_order").c_str(), order, 0);
    }

    GList * const columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(tree));

    GSList *list = 0;

    for (GList *it = columns; it; it = it->next)
      {
	GtkTreeViewColumn *column;
	ColumnState cs;

	column = static_cast<GtkTreeViewColumn*>(it->data);
	cs.id = gtk_tree_view_column_get_sort_column_id(column);
	cs.visible = gtk_tree_view_column_get_visible(column);
	cs.width = gtk_tree_view_column_get_width(column);

	list = g_slist_append(list, GINT_TO_POINTER(cs.pack()));
      }

    g_list_free(columns);

    GError *error = 0;

    if (not mateconf_client_set_list(client, (prefix + "/columns").c_str(),
				  MATECONF_VALUE_INT, list,
				  &error)) {
      g_critical("Failed to save tree state %s : %s",
		 prefix.c_str(),
		 error->message);
      g_error_free(error);
    }

    g_slist_free(list);
  }


  gboolean procman_get_tree_state2(MateConfClient *client, GtkWidget *tree, const gchar *cprefix)
  {
    const string prefix(cprefix);
    GtkTreeModel *model;

    gint sort_col;
    GtkSortType order;

    g_assert(tree);
    g_assert(prefix != "");

    if (!mateconf_client_dir_exists(client, prefix.c_str(), 0))
      return FALSE;

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

    sort_col = mateconf_client_get_int(client, (prefix + "/sort_col").c_str(), 0);
    sort_order = mateconf_client_get_int(client, (prefix + "/sort_order").c_str(), 0);

    if (sort_col != -1)
      gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), sort_col, order);

    proctable_set_columns_order(GTK_TREE_VIEW(tree), order);

    GSlist *list = mateconf_client_get_list(client, (prefix + "/columns").c_str(),
					 MATECONF_VALUE_INT, 0);


    for (GSList *it = list; it; it = it->next) {
      ColumnState cs;
      cs.unpack(GPOINTER_TO_INT(it->data));

      GtkTreeViewColumn *column;
      column = gtk_tree_view_get_column(GTK_TREE_VIEW(tree), cs.id);

      if (!column)
	continue;

      gtk_tree_view_column_set_visible(column, cs.visible);
      if (cs.visible)
	gtk_tree_view_column_set_fixed_width(column, MAX(10, cs.width));
    }

    g_slist_free(list);


    GList * const columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(tree));

    for (GList * it = columns; it; it = it->next)
      {
	GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn*>(it->data);
	unsigned id = gtk_tree_view_column_get_sort_column_id(column);

	ColumnState &cs(states[id]);



	key = g_strdup_printf("%s/col_%d_width", prefix, id);
	value = mateconf_client_get (client, key, NULL);
	g_free (key);

	if (value != NULL) {
	  width = mateconf_value_get_int(value);
	  mateconf_value_free (value);

	  key = g_strdup_printf ("%s/col_%d_visible", prefix, id);
	  visible = mateconf_client_get_bool (client, key, NULL);
	  g_free (key);

	  column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), id);
	  if(!column) continue;
	  gtk_tree_view_column_set_visible (column, visible);
	  if (visible) {
	    /* ensure column is really visible */
	    width = MAX(width, 10);
	    gtk_tree_view_column_set_fixed_width(column, width);
	  }
	}
      }

    g_list_free(columns);

    return TRUE;
  }



#endif



  class InodeDevices
  {
    typedef std::map<guint16, string> Map;
    Map devices;

  public:

    void update()
    {
      this->devices.clear();

      glibtop_mountlist list;
      glibtop_mountentry *entries = glibtop_get_mountlist(&list, 1);

      for (unsigned i = 0; i != list.number; ++i) {
	struct stat buf;

	if (stat(entries[i].devname, &buf) != -1)
	  this->devices[buf.st_rdev] = entries[i].devname;
      }

      g_free(entries);
    }

    string get(guint64 dev64)
    {
      if (dev64 == 0)
	return "";

      guint16 dev = dev64 & 0xffff;

      if (dev != dev64)
	g_warning("weird device %" G_GINT64_MODIFIER "x", dev64);

      Map::iterator it(this->devices.find(dev));

      if (it != this->devices.end())
	return it->second;

      guint8 major, minor;
      major = dev >> 8;
      minor = dev;

      std::ostringstream out;
      out << std::hex
	  << std::setfill('0')
	  << std::setw(2) << unsigned(major)
	  << ':'
	  << std::setw(2) << unsigned(minor);

      this->devices[dev] = out.str();
      return out.str();
    }
  };


  class MemMapsData
  {
  public:
    guint timer;
    GtkWidget *tree;
    MateConfClient *client;
    ProcInfo *info;
    OffsetFormater format;
    mutable InodeDevices devices;
    const char * const key;

    MemMapsData(GtkWidget *a_tree, MateConfClient *a_client)
      : tree(a_tree),
	client(a_client),
	key("/apps/procman/memmapstree2")
    {
      procman_get_tree_state(this->client, this->tree, this->key);
    }

    ~MemMapsData()
    {
      procman_save_tree_state(this->client, this->tree, this->key);
    }
  };
}


struct glibtop_map_entry_cmp
{
  bool operator()(const glibtop_map_entry &a, const guint64 start) const
  {
    return a.start < start;
  }

  bool operator()(const guint64 &start, const glibtop_map_entry &a) const
  {
    return not (*this)(a, start);
  }

};


static void
update_row(GtkTreeModel *model, GtkTreeIter &row, const MemMapsData &mm, const glibtop_map_entry *memmaps)
{
	guint64 size;
	string filename, device;
	string vmstart, vmend, vmoffset;
	char flags[5] = "----";

	size = memmaps->end - memmaps->start;

	if(memmaps->perm & GLIBTOP_MAP_PERM_READ)    flags [0] = 'r';
	if(memmaps->perm & GLIBTOP_MAP_PERM_WRITE)   flags [1] = 'w';
	if(memmaps->perm & GLIBTOP_MAP_PERM_EXECUTE) flags [2] = 'x';
	if(memmaps->perm & GLIBTOP_MAP_PERM_SHARED)  flags [3] = 's';
	if(memmaps->perm & GLIBTOP_MAP_PERM_PRIVATE) flags [3] = 'p';

	if (memmaps->flags & (1 << GLIBTOP_MAP_ENTRY_FILENAME))
	  filename = memmaps->filename;

	vmstart  = mm.format(memmaps->start);
	vmend    = mm.format(memmaps->end);
	vmoffset = mm.format(memmaps->offset);
	device   = mm.devices.get(memmaps->device);

	gtk_list_store_set (GTK_LIST_STORE (model), &row,
			    MMAP_COL_FILENAME, filename.c_str(),
			    MMAP_COL_VMSTART, vmstart.c_str(),
			    MMAP_COL_VMEND, vmend.c_str(),
			    MMAP_COL_VMSZ, size,
			    MMAP_COL_FLAGS, flags,
			    MMAP_COL_VMOFFSET, vmoffset.c_str(),
			    MMAP_COL_PRIVATE_CLEAN, memmaps->private_clean,
			    MMAP_COL_PRIVATE_DIRTY, memmaps->private_dirty,
			    MMAP_COL_SHARED_CLEAN, memmaps->shared_clean,
			    MMAP_COL_SHARED_DIRTY, memmaps->shared_dirty,
			    MMAP_COL_DEVICE, device.c_str(),
			    MMAP_COL_INODE, memmaps->inode,
			    -1);
}




static void
update_memmaps_dialog (MemMapsData *mmdata)
{
	GtkTreeModel *model;
	glibtop_map_entry *memmaps;
	glibtop_proc_map procmap;

	memmaps = glibtop_get_proc_map (&procmap, mmdata->info->pid);
	/* process has disappeared */
	if(!memmaps or procmap.number == 0) return;

	mmdata->format.set(memmaps[procmap.number - 1]);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (mmdata->tree));

	GtkTreeIter iter;

	typedef std::map<guint64, GtkTreeIter> IterCache;
	IterCache iter_cache;

	/*
	  removes the old maps and
	   also fills a cache of start -> iter in order to speed
	   up add
	*/

	if (gtk_tree_model_get_iter_first(model, &iter)) {
	  while (true) {
	    char *vmstart = 0;
	    guint64 start;
	    gtk_tree_model_get(model, &iter,
			       MMAP_COL_VMSTART, &vmstart,
			       -1);

	    try {
		    std::istringstream(vmstart) >> std::hex >> start;
	    } catch (std::logic_error &e) {
		    g_warning("Could not parse %s", vmstart);
		    start = 0;
	    }

	    g_free(vmstart);

	    bool found = std::binary_search(memmaps, memmaps + procmap.number,
					    start, glibtop_map_entry_cmp());

	    if (found) {
	      iter_cache[start] = iter;
	      if (!gtk_tree_model_iter_next(model, &iter))
		break;
	    } else {
	      if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
		break;
	    }
	  }
	}

	mmdata->devices.update();

	/*
	  add the new maps
	*/

	for (guint i = 0; i != procmap.number; i++) {
	  GtkTreeIter iter;
	  IterCache::iterator it(iter_cache.find(memmaps[i].start));

	  if (it != iter_cache.end())
	    iter = it->second;
	  else
	    gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);

	  update_row(model, iter, *mmdata, &memmaps[i]);
	}

	g_free (memmaps);
}



static gboolean window_delete_event(GtkWidget *, GdkEvent *, gpointer data)
{
	MemMapsData * const mmdata = static_cast<MemMapsData*>(data);

	g_source_remove (mmdata->timer);

	delete mmdata;
	return FALSE;
}


static MemMapsData*
create_memmapsdata (ProcData *procdata)
{
	GtkWidget *tree;
	GtkListStore *model;
	guint i;

	const gchar * const titles[] = {
		N_("Filename"),
		// xgettext: virtual memory start
		N_("VM Start"),
		// xgettext: virtual memory end
		N_("VM End"),
		// xgettext: virtual memory syze
		N_("VM Size"),
		N_("Flags"),
		// xgettext: virtual memory offset
		N_("VM Offset"),
		// xgettext: memory that has not been modified since
		// it has been allocated
		N_("Private clean"),
		// xgettext: memory that has been modified since it
		// has been allocated
		N_("Private dirty"),
		// xgettext: shared memory that has not been modified
		// since it has been allocated
		N_("Shared clean"),
		// xgettext: shared memory that has been modified
		// since it has been allocated
		N_("Shared dirty"),
		N_("Device"),
		N_("Inode")
	};

	model = gtk_list_store_new (MMAP_COL_MAX,
				    G_TYPE_STRING, /* MMAP_COL_FILENAME  */
				    G_TYPE_STRING, /* MMAP_COL_VMSTART	 */
				    G_TYPE_STRING, /* MMAP_COL_VMEND	 */
				    G_TYPE_UINT64, /* MMAP_COL_VMSZ	 */
				    G_TYPE_STRING, /* MMAP_COL_FLAGS	 */
				    G_TYPE_STRING, /* MMAP_COL_VMOFFSET  */
				    G_TYPE_UINT64, /* MMAP_COL_PRIVATE_CLEAN */
				    G_TYPE_UINT64, /* MMAP_COL_PRIVATE_DIRTY */
				    G_TYPE_UINT64, /* MMAP_COL_SHARED_CLEAN */
				    G_TYPE_UINT64, /* MMAP_COL_SHARED_DIRTY */
				    G_TYPE_STRING, /* MMAP_COL_DEVICE	 */
				    G_TYPE_UINT64 /* MMAP_COL_INODE	 */
				    );

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);
	g_object_unref (G_OBJECT (model));

	for (i = 0; i < MMAP_COL_MAX; i++) {
		GtkCellRenderer *cell;
		GtkTreeViewColumn *col;

		cell = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(col, cell, TRUE);
		gtk_tree_view_column_set_title(col, _(titles[i]));
		gtk_tree_view_column_set_resizable(col, TRUE);
		gtk_tree_view_column_set_sort_column_id(col, i);
		gtk_tree_view_column_set_reorderable(col, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

		switch (i) {
		case MMAP_COL_PRIVATE_CLEAN:
		case MMAP_COL_PRIVATE_DIRTY:
		case MMAP_COL_SHARED_CLEAN:
		case MMAP_COL_SHARED_DIRTY:
		case MMAP_COL_VMSZ:
		  gtk_tree_view_column_set_cell_data_func(col, cell,
							  &procman::size_cell_data_func,
							  GUINT_TO_POINTER(i),
							  NULL);

		  g_object_set(cell, "xalign", 1.0f, NULL);
		  break;

		default:
		  gtk_tree_view_column_set_attributes(col, cell, "text", i, NULL);
		  break;
		}


		switch (i) {
		case MMAP_COL_VMSTART:
		case MMAP_COL_VMEND:
		case MMAP_COL_FLAGS:
		case MMAP_COL_VMOFFSET:
		case MMAP_COL_DEVICE:
			g_object_set(cell, "family", "monospace", NULL);
			break;
		}
	}

	return new MemMapsData(tree, procdata->client);
}


static gboolean
memmaps_timer (gpointer data)
{
	MemMapsData * const mmdata = static_cast<MemMapsData*>(data);
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (mmdata->tree));
	g_assert(model);

	update_memmaps_dialog (mmdata);

	return TRUE;
}


static void
create_single_memmaps_dialog (GtkTreeModel *model, GtkTreePath *path,
			      GtkTreeIter *iter, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	MemMapsData *mmdata;
	GtkWidget *memmapsdialog;
	GtkWidget *dialog_vbox, *vbox;
	GtkWidget *label;
	GtkWidget *scrolled;
	ProcInfo *info;

	gtk_tree_model_get (model, iter, COL_POINTER, &info, -1);

	if (!info)
		return;

	mmdata = create_memmapsdata (procdata);
	mmdata->info = info;

	memmapsdialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for(GTK_WINDOW(memmapsdialog), GTK_WINDOW(procdata->app));
	gtk_window_set_destroy_with_parent(GTK_WINDOW(memmapsdialog), TRUE);
	// gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(memmapsdialog), _("Memory Maps"));
	gtk_window_set_resizable(GTK_WINDOW(memmapsdialog), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(memmapsdialog), 575, 400);
	// gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(memmapsdialog), 12);

	GtkWidget *mainbox = gtk_vbox_new(FALSE, 12);
	gtk_container_add(GTK_CONTAINER(memmapsdialog), mainbox);

	vbox = mainbox;
	gtk_box_set_spacing (GTK_BOX (vbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	dialog_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
	gtk_box_pack_start (GTK_BOX (vbox), dialog_vbox, TRUE, TRUE, 0);


	label = procman_make_label_for_mmaps_or_ofiles (
		_("_Memory maps for process \"%s\" (PID %u):"),
		info->name,
		info->pid);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), label, FALSE, TRUE, 0);


	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_IN);

	gtk_container_add (GTK_CONTAINER (scrolled), mmdata->tree);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), mmdata->tree);

	gtk_box_pack_start (GTK_BOX (dialog_vbox), scrolled, TRUE, TRUE, 0);

	gtk_widget_show_all (memmapsdialog);

	g_signal_connect(G_OBJECT(memmapsdialog), "delete-event",
			 G_CALLBACK(window_delete_event), mmdata);

	mmdata->timer = g_timeout_add_seconds (5, memmaps_timer, mmdata);

	update_memmaps_dialog (mmdata);
}


void
create_memmaps_dialog (ProcData *procdata)
{
	/* TODO: do we really want to open multiple dialogs ? */
	gtk_tree_selection_selected_foreach (procdata->selection, create_single_memmaps_dialog,
					     procdata);
}
