/*
 * Copyright (C) 2005 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <Python.h>
#include <gmenu-tree.h>

typedef struct
{
  PyObject_HEAD
  GMenuTree *tree;
  GSList    *callbacks;
} PyGMenuTree;

typedef struct
{
  PyObject *tree;
  PyObject *callback;
  PyObject *user_data;
} PyGMenuTreeCallback;

typedef struct
{
  PyObject_HEAD
  GMenuTreeItem *item;
} PyGMenuTreeItem;

typedef PyGMenuTreeItem PyGMenuTreeDirectory;
typedef PyGMenuTreeItem PyGMenuTreeEntry;
typedef PyGMenuTreeItem PyGMenuTreeSeparator;
typedef PyGMenuTreeItem PyGMenuTreeHeader;
typedef PyGMenuTreeItem PyGMenuTreeAlias;

static PyGMenuTree          *pygmenu_tree_wrap           (GMenuTree          *tree);
static PyGMenuTreeDirectory *pygmenu_tree_directory_wrap (GMenuTreeDirectory *directory);
static PyGMenuTreeEntry     *pygmenu_tree_entry_wrap     (GMenuTreeEntry     *entry);
static PyGMenuTreeSeparator *pygmenu_tree_separator_wrap (GMenuTreeSeparator *separator);
static PyGMenuTreeHeader    *pygmenu_tree_header_wrap    (GMenuTreeHeader    *header);
static PyGMenuTreeAlias     *pygmenu_tree_alias_wrap     (GMenuTreeAlias     *alias);

static inline PyObject *
lookup_item_type_str (const char *item_type_str)
{
  PyObject *module;

  module = PyDict_GetItemString (PyImport_GetModuleDict (), "gmenu");

  return PyDict_GetItemString (PyModule_GetDict (module), item_type_str);
}

static void
pygmenu_tree_item_dealloc (PyGMenuTreeItem *self)
{
  if (self->item != NULL)
    {
      gmenu_tree_item_set_user_data (self->item, NULL, NULL);
      gmenu_tree_item_unref (self->item);
      self->item = NULL;
    }

  PyObject_DEL (self);
}

static PyObject *
pygmenu_tree_item_get_type (PyObject *self,
			    PyObject *args)
{
  PyGMenuTreeItem *item;
  PyObject        *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Item.get_type"))
	return NULL;
    }

  item = (PyGMenuTreeItem *) self;

  switch (gmenu_tree_item_get_type (item->item))
    {
    case GMENU_TREE_ITEM_DIRECTORY:
      retval = lookup_item_type_str ("TYPE_DIRECTORY");
      break;

    case GMENU_TREE_ITEM_ENTRY:
      retval = lookup_item_type_str ("TYPE_ENTRY");
      break;

    case GMENU_TREE_ITEM_SEPARATOR:
      retval = lookup_item_type_str ("TYPE_SEPARATOR");
      break;

    case GMENU_TREE_ITEM_HEADER:
      retval = lookup_item_type_str ("TYPE_HEADER");
      break;

    case GMENU_TREE_ITEM_ALIAS:
      retval = lookup_item_type_str ("TYPE_ALIAS");
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  Py_INCREF (retval);

  return retval;
}

static PyObject *
pygmenu_tree_item_get_parent (PyObject *self,
			      PyObject *args)
{
  PyGMenuTreeItem      *item;
  GMenuTreeDirectory   *parent;
  PyGMenuTreeDirectory *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Item.get_parent"))
	return NULL;
    }

  item = (PyGMenuTreeItem *) self;

  parent = gmenu_tree_item_get_parent (item->item);
  if (parent == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_directory_wrap (parent);

  gmenu_tree_item_unref (parent);

  return (PyObject *) retval;
}

static struct PyMethodDef pygmenu_tree_item_methods[] =
{
  { "get_type",   pygmenu_tree_item_get_type,   METH_VARARGS },
  { "get_parent", pygmenu_tree_item_get_parent, METH_VARARGS },
  { NULL,         NULL,                         0            }
};

static PyTypeObject PyGMenuTreeItem_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                             /* ob_size */
  "gmenu.Item",                                  /* tp_name */
  sizeof (PyGMenuTreeItem),                      /* tp_basicsize */
  0,                                             /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,        /* tp_dealloc */
  (printfunc)0,                                  /* tp_print */
  (getattrfunc)0,                                /* tp_getattr */
  (setattrfunc)0,                                /* tp_setattr */
  (cmpfunc)0,                                    /* tp_compare */
  (reprfunc)0,                                   /* tp_repr */
  0,                                             /* tp_as_number */
  0,                                             /* tp_as_sequence */
  0,                                             /* tp_as_mapping */
  (hashfunc)0,                                   /* tp_hash */
  (ternaryfunc)0,                                /* tp_call */
  (reprfunc)0,                                   /* tp_str */
  (getattrofunc)0,                               /* tp_getattro */
  (setattrofunc)0,                               /* tp_setattro */
  0,                                             /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,      /* tp_flags */
  NULL,                                          /* Documentation string */
  (traverseproc)0,                               /* tp_traverse */
  (inquiry)0,                                    /* tp_clear */
  (richcmpfunc)0,                                /* tp_richcompare */
  0,                                             /* tp_weaklistoffset */
  (getiterfunc)0,                                /* tp_iter */
  (iternextfunc)0,                               /* tp_iternext */
  pygmenu_tree_item_methods,                     /* tp_methods */
  0,                                             /* tp_members */
  0,                                             /* tp_getset */
  (PyTypeObject *)0,                             /* tp_base */
  (PyObject *)0,                                 /* tp_dict */
  0,                                             /* tp_descr_get */
  0,                                             /* tp_descr_set */
  0,                                             /* tp_dictoffset */
  (initproc)0,                                   /* tp_init */
  0,                                             /* tp_alloc */
  0,                                             /* tp_new */
  0,                                             /* tp_free */
  (inquiry)0,                                    /* tp_is_gc */
  (PyObject *)0,                                 /* tp_bases */
};

static PyObject *
pygmenu_tree_directory_get_contents (PyObject *self,
				     PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  PyObject             *retval;
  GSList               *contents;
  GSList               *tmp;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_contents"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  retval = PyList_New (0);

  contents = gmenu_tree_directory_get_contents (GMENU_TREE_DIRECTORY (directory->item));

  tmp = contents;
  while (tmp != NULL)
    {
      GMenuTreeItem *item = tmp->data;
      PyObject      *pyitem;

      switch (gmenu_tree_item_get_type (item))
	{
	case GMENU_TREE_ITEM_DIRECTORY:
	  pyitem = (PyObject *) pygmenu_tree_directory_wrap (GMENU_TREE_DIRECTORY (item));
	  break;

	case GMENU_TREE_ITEM_ENTRY:
	  pyitem = (PyObject *) pygmenu_tree_entry_wrap (GMENU_TREE_ENTRY (item));
	  break;

	case GMENU_TREE_ITEM_SEPARATOR:
	  pyitem = (PyObject *) pygmenu_tree_separator_wrap (GMENU_TREE_SEPARATOR (item));
	  break;

	case GMENU_TREE_ITEM_HEADER:
	  pyitem = (PyObject *) pygmenu_tree_header_wrap (GMENU_TREE_HEADER (item));
	  break;

	case GMENU_TREE_ITEM_ALIAS:
	  pyitem = (PyObject *) pygmenu_tree_alias_wrap (GMENU_TREE_ALIAS (item));
	  break;

	default:
	  g_assert_not_reached ();
	  break;
	}

      PyList_Append (retval, pyitem);
      Py_DECREF (pyitem);

      gmenu_tree_item_unref (item);

      tmp = tmp->next;
    }

  g_slist_free (contents);

  return retval;
}

static PyObject *
pygmenu_tree_directory_get_name (PyObject *self,
				 PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  const char           *name;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_name"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  name = gmenu_tree_directory_get_name (GMENU_TREE_DIRECTORY (directory->item));
  if (name == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (name);
}

static PyObject *
pygmenu_tree_directory_get_comment (PyObject *self,
				    PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  const char           *comment;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_comment"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  comment = gmenu_tree_directory_get_comment (GMENU_TREE_DIRECTORY (directory->item));
  if (comment == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (comment);
}

static PyObject *
pygmenu_tree_directory_get_icon (PyObject *self,
				 PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  const char           *icon;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_icon"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  icon = gmenu_tree_directory_get_icon (GMENU_TREE_DIRECTORY (directory->item));
  if (icon == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (icon);
}

static PyObject *
pygmenu_tree_directory_get_desktop_file_path (PyObject *self,
				              PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  const char           *path;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_desktop_file_path"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  path = gmenu_tree_directory_get_desktop_file_path (GMENU_TREE_DIRECTORY (directory->item));
  if (path == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (path);
}

static PyObject *
pygmenu_tree_directory_get_menu_id (PyObject *self,
				    PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  const char           *menu_id;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Directory.get_menu_id"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  menu_id = gmenu_tree_directory_get_menu_id (GMENU_TREE_DIRECTORY (directory->item));
  if (menu_id == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (menu_id);
}

static PyObject *
pygmenu_tree_directory_get_tree (PyObject *self,
				 PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  GMenuTree            *tree;
  PyGMenuTree          *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Item.get_tree"))
	return NULL;
    }

  directory = (PyGMenuTreeDirectory *) self;

  tree = gmenu_tree_directory_get_tree (GMENU_TREE_DIRECTORY (directory->item));
  if (tree == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_wrap (tree);

  gmenu_tree_unref (tree);

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_directory_make_path (PyObject *self,
				  PyObject *args)
{
  PyGMenuTreeDirectory *directory;
  PyGMenuTreeEntry     *entry;
  PyObject             *retval;
  char                 *path;

  if (!PyArg_ParseTuple (args, "O:gmenu.Directory.make_path", &entry))
	return NULL;

  directory = (PyGMenuTreeDirectory *) self;

  path = gmenu_tree_directory_make_path (GMENU_TREE_DIRECTORY (directory->item),
					GMENU_TREE_ENTRY (entry->item));
  if (path == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = PyString_FromString (path);

  g_free (path);

  return retval;
}

static PyObject *
pygmenu_tree_directory_getattro (PyGMenuTreeDirectory *self,
				 PyObject             *py_attr)
{
  if (PyString_Check (py_attr))
    {
      char *attr;
  
      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "__members__"))
	{
	  return Py_BuildValue ("[sssssssss]",
				"type",
				"parent",
				"contents",
				"name",
				"comment",
				"icon",
				"desktop_file_path",
				"menu_id",
				"tree");
	}
      else if (!strcmp (attr, "type"))
	{
	  return pygmenu_tree_item_get_type ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "parent"))
	{
	  return pygmenu_tree_item_get_parent ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "contents"))
	{
	  return pygmenu_tree_directory_get_contents ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "name"))
	{
	  return pygmenu_tree_directory_get_name ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "comment"))
	{
	  return pygmenu_tree_directory_get_comment ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "icon"))
	{
	  return pygmenu_tree_directory_get_icon ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "desktop_file_path"))
	{
	  return pygmenu_tree_directory_get_desktop_file_path ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "menu_id"))
	{
	  return pygmenu_tree_directory_get_menu_id ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "tree"))
	{
	  return pygmenu_tree_directory_get_tree ((PyObject *) self, NULL);
	}
    }

  return PyObject_GenericGetAttr ((PyObject *) self, py_attr);
}

static struct PyMethodDef pygmenu_tree_directory_methods[] =
{
  { "get_contents",           pygmenu_tree_directory_get_contents,           METH_VARARGS },
  { "get_name",               pygmenu_tree_directory_get_name,               METH_VARARGS },
  { "get_comment",            pygmenu_tree_directory_get_comment,            METH_VARARGS },
  { "get_icon",               pygmenu_tree_directory_get_icon,               METH_VARARGS },
  { "get_desktop_file_path",  pygmenu_tree_directory_get_desktop_file_path,  METH_VARARGS },
  { "get_menu_id",            pygmenu_tree_directory_get_menu_id,            METH_VARARGS },
  { "get_tree",               pygmenu_tree_directory_get_tree,               METH_VARARGS },
  { "make_path",              pygmenu_tree_directory_make_path,              METH_VARARGS },
  { NULL,                     NULL,                                          0            }
};

static PyTypeObject PyGMenuTreeDirectory_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                              /* ob_size */
  "gmenu.Directory",                              /* tp_name */
  sizeof (PyGMenuTreeDirectory),                  /* tp_basicsize */
  0,                                              /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,         /* tp_dealloc */
  (printfunc)0,                                   /* tp_print */
  (getattrfunc)0,                                 /* tp_getattr */
  (setattrfunc)0,                                 /* tp_setattr */
  (cmpfunc)0,                                     /* tp_compare */
  (reprfunc)0,                                    /* tp_repr */
  0,                                              /* tp_as_number */
  0,                                              /* tp_as_sequence */
  0,                                              /* tp_as_mapping */
  (hashfunc)0,                                    /* tp_hash */
  (ternaryfunc)0,                                 /* tp_call */
  (reprfunc)0,                                    /* tp_str */
  (getattrofunc) pygmenu_tree_directory_getattro, /* tp_getattro */
  (setattrofunc)0,                                /* tp_setattro */
  0,                                              /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                             /* tp_flags */
  NULL,                                           /* Documentation string */
  (traverseproc)0,                                /* tp_traverse */
  (inquiry)0,                                     /* tp_clear */
  (richcmpfunc)0,                                 /* tp_richcompare */
  0,                                              /* tp_weaklistoffset */
  (getiterfunc)0,                                 /* tp_iter */
  (iternextfunc)0,                                /* tp_iternext */
  pygmenu_tree_directory_methods,                 /* tp_methods */
  0,                                              /* tp_members */
  0,                                              /* tp_getset */
  (PyTypeObject *)0,                              /* tp_base */
  (PyObject *)0,                                  /* tp_dict */
  0,                                              /* tp_descr_get */
  0,                                              /* tp_descr_set */
  0,                                              /* tp_dictoffset */
  (initproc)0,                                    /* tp_init */
  0,                                              /* tp_alloc */
  0,                                              /* tp_new */
  0,                                              /* tp_free */
  (inquiry)0,                                     /* tp_is_gc */
  (PyObject *)0,                                  /* tp_bases */
};

static PyGMenuTreeDirectory *
pygmenu_tree_directory_wrap (GMenuTreeDirectory *directory)
{
  PyGMenuTreeDirectory *retval;

  if ((retval = gmenu_tree_item_get_user_data (GMENU_TREE_ITEM (directory))) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTreeDirectory *) PyObject_NEW (PyGMenuTreeDirectory,
							&PyGMenuTreeDirectory_Type)))
    return NULL;

  retval->item = gmenu_tree_item_ref (directory);

  gmenu_tree_item_set_user_data (GMENU_TREE_ITEM (directory), retval, NULL);

  return retval;
}

static PyObject *
pygmenu_tree_entry_get_name (PyObject *self,
			     PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *name;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_name"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  name = gmenu_tree_entry_get_name (GMENU_TREE_ENTRY (entry->item));
  if (name == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (name);
}

static PyObject *
pygmenu_tree_entry_get_generic_name (PyObject *self,
				     PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *generic_name;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_generic_name"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  generic_name = gmenu_tree_entry_get_generic_name (GMENU_TREE_ENTRY (entry->item));
  if (generic_name == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (generic_name);
}

static PyObject *
pygmenu_tree_entry_get_display_name (PyObject *self,
				     PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *display_name;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_display_name"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  display_name = gmenu_tree_entry_get_display_name (GMENU_TREE_ENTRY (entry->item));
  if (display_name == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (display_name);
}

static PyObject *
pygmenu_tree_entry_get_comment (PyObject *self,
				PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *comment;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_comment"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  comment = gmenu_tree_entry_get_comment (GMENU_TREE_ENTRY (entry->item));
  if (comment == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (comment);
}

static PyObject *
pygmenu_tree_entry_get_icon (PyObject *self,
			     PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *icon;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_icon"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  icon = gmenu_tree_entry_get_icon (GMENU_TREE_ENTRY (entry->item));
  if (icon == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (icon);
}

static PyObject *
pygmenu_tree_entry_get_exec (PyObject *self,
			     PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *exec;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_exec"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  exec = gmenu_tree_entry_get_exec (GMENU_TREE_ENTRY (entry->item));
  if (exec == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (exec);
}

static PyObject *
pygmenu_tree_entry_get_launch_in_terminal (PyObject *self,
				    PyObject *args)
{
  PyGMenuTreeEntry *entry;
  PyObject         *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_launch_in_terminal"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  if (gmenu_tree_entry_get_launch_in_terminal (GMENU_TREE_ENTRY (entry->item)))
    retval = Py_True;
  else
    retval = Py_False;
  Py_INCREF (retval);

  return retval;
}

static PyObject *
pygmenu_tree_entry_get_desktop_file_path (PyObject *self,
					  PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *desktop_file_path;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_desktop_file_path"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  desktop_file_path = gmenu_tree_entry_get_desktop_file_path (GMENU_TREE_ENTRY (entry->item));
  if (desktop_file_path == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (desktop_file_path);
}

static PyObject *
pygmenu_tree_entry_get_desktop_file_id (PyObject *self,
					PyObject *args)
{
  PyGMenuTreeEntry *entry;
  const char       *desktop_file_id;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_desktop_file_id"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  desktop_file_id = gmenu_tree_entry_get_desktop_file_id (GMENU_TREE_ENTRY (entry->item));
  if (desktop_file_id == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (desktop_file_id);
}

static PyObject *
pygmenu_tree_entry_get_is_excluded (PyObject *self,
				    PyObject *args)
{
  PyGMenuTreeEntry *entry;
  PyObject         *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_is_excluded"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  retval = gmenu_tree_entry_get_is_excluded (GMENU_TREE_ENTRY (entry->item)) ? Py_True : Py_False;
  Py_INCREF (retval);

  return retval;
}

static PyObject *
pygmenu_tree_entry_get_is_nodisplay (PyObject *self,
				    PyObject *args)
{
  PyGMenuTreeEntry *entry;
  PyObject         *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Entry.get_is_nodisplay"))
	return NULL;
    }

  entry = (PyGMenuTreeEntry *) self;

  if (gmenu_tree_entry_get_is_nodisplay (GMENU_TREE_ENTRY (entry->item)))
    retval = Py_True;
  else
    retval = Py_False;
  Py_INCREF (retval);

  return retval;
}

static PyObject *
pygmenu_tree_entry_getattro (PyGMenuTreeEntry *self,
			     PyObject         *py_attr)
{
  if (PyString_Check (py_attr))
    {
      char *attr;
  
      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "__members__"))
	{
	  return Py_BuildValue ("[sssssssssss]",
				"type",
				"parent",
				"name",
				"comment",
				"icon",
				"exec_info",
				"launch_in_terminal",
				"desktop_file_path",
				"desktop_file_id",
				"is_excluded",
				"is_nodisplay");
	}
      else if (!strcmp (attr, "type"))
	{
	  return pygmenu_tree_item_get_type ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "parent"))
	{
	  return pygmenu_tree_item_get_parent ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "name"))
	{
	  return pygmenu_tree_entry_get_name ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "generic_name"))
	{
	  return pygmenu_tree_entry_get_generic_name ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "display_name"))
	{
	  return pygmenu_tree_entry_get_display_name ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "comment"))
	{
	  return pygmenu_tree_entry_get_comment ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "icon"))
	{
	  return pygmenu_tree_entry_get_icon ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "exec_info"))
	{
	  return pygmenu_tree_entry_get_exec ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "launch_in_terminal"))
	{
	  return pygmenu_tree_entry_get_launch_in_terminal ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "desktop_file_path"))
	{
	  return pygmenu_tree_entry_get_desktop_file_path ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "desktop_file_id"))
	{
	  return pygmenu_tree_entry_get_desktop_file_id ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "is_excluded"))
	{
	  return pygmenu_tree_entry_get_is_excluded ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "is_nodisplay"))
	{
	  return pygmenu_tree_entry_get_is_nodisplay ((PyObject *) self, NULL);
	}
    }

  return PyObject_GenericGetAttr ((PyObject *) self, py_attr);
}

static struct PyMethodDef pygmenu_tree_entry_methods[] =
{
  { "get_name",               pygmenu_tree_entry_get_name,               METH_VARARGS },
  { "get_generic_name",       pygmenu_tree_entry_get_generic_name,       METH_VARARGS },
  { "get_display_name",       pygmenu_tree_entry_get_display_name,       METH_VARARGS },
  { "get_comment",            pygmenu_tree_entry_get_comment,            METH_VARARGS },
  { "get_icon",               pygmenu_tree_entry_get_icon,               METH_VARARGS },
  { "get_exec",               pygmenu_tree_entry_get_exec,               METH_VARARGS },
  { "get_launch_in_terminal", pygmenu_tree_entry_get_launch_in_terminal, METH_VARARGS },
  { "get_desktop_file_path",  pygmenu_tree_entry_get_desktop_file_path,  METH_VARARGS },
  { "get_desktop_file_id",    pygmenu_tree_entry_get_desktop_file_id,    METH_VARARGS },
  { "get_is_excluded",        pygmenu_tree_entry_get_is_excluded,        METH_VARARGS },
  { "get_is_nodisplay",       pygmenu_tree_entry_get_is_nodisplay,       METH_VARARGS },
  { NULL,                     NULL,                                      0            }
};

static PyTypeObject PyGMenuTreeEntry_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                             /* ob_size */
  "gmenu.Entry",                                 /* tp_name */
  sizeof (PyGMenuTreeEntry),                     /* tp_basicsize */
  0,                                             /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,        /* tp_dealloc */
  (printfunc)0,                                  /* tp_print */
  (getattrfunc)0,                                /* tp_getattr */
  (setattrfunc)0,                                /* tp_setattr */
  (cmpfunc)0,                                    /* tp_compare */
  (reprfunc)0,                                   /* tp_repr */
  0,                                             /* tp_as_number */
  0,                                             /* tp_as_sequence */
  0,                                             /* tp_as_mapping */
  (hashfunc)0,                                   /* tp_hash */
  (ternaryfunc)0,                                /* tp_call */
  (reprfunc)0,                                   /* tp_str */
  (getattrofunc) pygmenu_tree_entry_getattro,    /* tp_getattro */
  (setattrofunc)0,                               /* tp_setattro */
  0,                                             /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                            /* tp_flags */
  NULL,                                          /* Documentation string */
  (traverseproc)0,                               /* tp_traverse */
  (inquiry)0,                                    /* tp_clear */
  (richcmpfunc)0,                                /* tp_richcompare */
  0,                                             /* tp_weaklistoffset */
  (getiterfunc)0,                                /* tp_iter */
  (iternextfunc)0,                               /* tp_iternext */
  pygmenu_tree_entry_methods,                    /* tp_methods */
  0,                                             /* tp_members */
  0,                                             /* tp_getset */
  (PyTypeObject *)0,                             /* tp_base */
  (PyObject *)0,                                 /* tp_dict */
  0,                                             /* tp_descr_get */
  0,                                             /* tp_descr_set */
  0,                                             /* tp_dictoffset */
  (initproc)0,                                   /* tp_init */
  0,                                             /* tp_alloc */
  0,                                             /* tp_new */
  0,                                             /* tp_free */
  (inquiry)0,                                    /* tp_is_gc */
  (PyObject *)0,                                 /* tp_bases */
};

static PyGMenuTreeEntry *
pygmenu_tree_entry_wrap (GMenuTreeEntry *entry)
{
  PyGMenuTreeEntry *retval;

  if ((retval = gmenu_tree_item_get_user_data (GMENU_TREE_ITEM (entry))) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTreeEntry *) PyObject_NEW (PyGMenuTreeEntry,
						    &PyGMenuTreeEntry_Type)))
    return NULL;

  retval->item = gmenu_tree_item_ref (entry);

  gmenu_tree_item_set_user_data (GMENU_TREE_ITEM (entry), retval, NULL);

  return retval;
}

static PyTypeObject PyGMenuTreeSeparator_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                             /* ob_size */
  "gmenu.Separator",                             /* tp_name */
  sizeof (PyGMenuTreeSeparator),                 /* tp_basicsize */
  0,                                             /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,        /* tp_dealloc */
  (printfunc)0,                                  /* tp_print */
  (getattrfunc)0,                                /* tp_getattr */
  (setattrfunc)0,                                /* tp_setattr */
  (cmpfunc)0,                                    /* tp_compare */
  (reprfunc)0,                                   /* tp_repr */
  0,                                             /* tp_as_number */
  0,                                             /* tp_as_sequence */
  0,                                             /* tp_as_mapping */
  (hashfunc)0,                                   /* tp_hash */
  (ternaryfunc)0,                                /* tp_call */
  (reprfunc)0,                                   /* tp_str */
  (getattrofunc)0,                               /* tp_getattro */
  (setattrofunc)0,                               /* tp_setattro */
  0,                                             /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                            /* tp_flags */
  NULL,                                          /* Documentation string */
  (traverseproc)0,                               /* tp_traverse */
  (inquiry)0,                                    /* tp_clear */
  (richcmpfunc)0,                                /* tp_richcompare */
  0,                                             /* tp_weaklistoffset */
  (getiterfunc)0,                                /* tp_iter */
  (iternextfunc)0,                               /* tp_iternext */
  NULL,                                          /* tp_methods */
  0,                                             /* tp_members */
  0,                                             /* tp_getset */
  (PyTypeObject *)0,                             /* tp_base */
  (PyObject *)0,                                 /* tp_dict */
  0,                                             /* tp_descr_get */
  0,                                             /* tp_descr_set */
  0,                                             /* tp_dictoffset */
  (initproc)0,                                   /* tp_init */
  0,                                             /* tp_alloc */
  0,                                             /* tp_new */
  0,                                             /* tp_free */
  (inquiry)0,                                    /* tp_is_gc */
  (PyObject *)0,                                 /* tp_bases */
};

static PyGMenuTreeSeparator *
pygmenu_tree_separator_wrap (GMenuTreeSeparator *separator)
{
  PyGMenuTreeSeparator *retval;

  if ((retval = gmenu_tree_item_get_user_data (GMENU_TREE_ITEM (separator))) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTreeSeparator *) PyObject_NEW (PyGMenuTreeSeparator,
							&PyGMenuTreeSeparator_Type)))
    return NULL;

  retval->item = gmenu_tree_item_ref (separator);

  gmenu_tree_item_set_user_data (GMENU_TREE_ITEM (separator), retval, NULL);

  return retval;
}

static PyObject *
pygmenu_tree_header_get_directory (PyObject *self,
				   PyObject *args)
{
  PyGMenuTreeHeader    *header;
  GMenuTreeDirectory   *directory;
  PyGMenuTreeDirectory *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Header.get_directory"))
	return NULL;
    }

  header = (PyGMenuTreeHeader *) self;

  directory = gmenu_tree_header_get_directory (GMENU_TREE_HEADER (header->item));
  if (directory == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_directory_wrap (directory);

  gmenu_tree_item_unref (directory);

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_header_getattro (PyGMenuTreeHeader *self,
			      PyObject          *py_attr)
{
  if (PyString_Check (py_attr))
    {
      char *attr;
  
      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "__members__"))
	{
	  return Py_BuildValue ("[sss]",
				"type",
				"parent",
				"directory");
	}
      else if (!strcmp (attr, "type"))
	{
	  return pygmenu_tree_item_get_type ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "parent"))
	{
	  return pygmenu_tree_item_get_parent ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "directory"))
	{
	  return pygmenu_tree_header_get_directory ((PyObject *) self, NULL);
	}
    }

  return PyObject_GenericGetAttr ((PyObject *) self, py_attr);
}

static struct PyMethodDef pygmenu_tree_header_methods[] =
{
  { "get_directory", pygmenu_tree_header_get_directory, METH_VARARGS },
  { NULL,            NULL,                              0            }
};

static PyTypeObject PyGMenuTreeHeader_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                             /* ob_size */
  "gmenu.Header",                                /* tp_name */
  sizeof (PyGMenuTreeHeader),                    /* tp_basicsize */
  0,                                             /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,        /* tp_dealloc */
  (printfunc)0,                                  /* tp_print */
  (getattrfunc)0,                                /* tp_getattr */
  (setattrfunc)0,                                /* tp_setattr */
  (cmpfunc)0,                                    /* tp_compare */
  (reprfunc)0,                                   /* tp_repr */
  0,                                             /* tp_as_number */
  0,                                             /* tp_as_sequence */
  0,                                             /* tp_as_mapping */
  (hashfunc)0,                                   /* tp_hash */
  (ternaryfunc)0,                                /* tp_call */
  (reprfunc)0,                                   /* tp_str */
  (getattrofunc) pygmenu_tree_header_getattro,   /* tp_getattro */
  (setattrofunc)0,                               /* tp_setattro */
  0,                                             /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                            /* tp_flags */
  NULL,                                          /* Documentation string */
  (traverseproc)0,                               /* tp_traverse */
  (inquiry)0,                                    /* tp_clear */
  (richcmpfunc)0,                                /* tp_richcompare */
  0,                                             /* tp_weaklistoffset */
  (getiterfunc)0,                                /* tp_iter */
  (iternextfunc)0,                               /* tp_iternext */
  pygmenu_tree_header_methods,                   /* tp_methods */
  0,                                             /* tp_members */
  0,                                             /* tp_getset */
  (PyTypeObject *)0,                             /* tp_base */
  (PyObject *)0,                                 /* tp_dict */
  0,                                             /* tp_descr_get */
  0,                                             /* tp_descr_set */
  0,                                             /* tp_dictoffset */
  (initproc)0,                                   /* tp_init */
  0,                                             /* tp_alloc */
  0,                                             /* tp_new */
  0,                                             /* tp_free */
  (inquiry)0,                                    /* tp_is_gc */
  (PyObject *)0,                                 /* tp_bases */
};

static PyGMenuTreeHeader *
pygmenu_tree_header_wrap (GMenuTreeHeader *header)
{
  PyGMenuTreeHeader *retval;

  if ((retval = gmenu_tree_item_get_user_data (GMENU_TREE_ITEM (header))) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTreeHeader *) PyObject_NEW (PyGMenuTreeHeader,
						     &PyGMenuTreeHeader_Type)))
    return NULL;

  retval->item = gmenu_tree_item_ref (header);

  gmenu_tree_item_set_user_data (GMENU_TREE_ITEM (header), retval, NULL);

  return retval;
}

static PyObject *
pygmenu_tree_alias_get_directory (PyObject *self,
				  PyObject *args)
{
  PyGMenuTreeAlias     *alias;
  GMenuTreeDirectory   *directory;
  PyGMenuTreeDirectory *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Alias.get_directory"))
	return NULL;
    }

  alias = (PyGMenuTreeAlias *) self;

  directory = gmenu_tree_alias_get_directory (GMENU_TREE_ALIAS (alias->item));
  if (directory == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_directory_wrap (directory);

  gmenu_tree_item_unref (directory);

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_alias_get_item (PyObject *self,
			     PyObject *args)
{
  PyGMenuTreeAlias *alias;
  GMenuTreeItem    *item;
  PyObject         *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Alias.get_item"))
	return NULL;
    }

  alias = (PyGMenuTreeAlias *) self;

  item = gmenu_tree_alias_get_item (GMENU_TREE_ALIAS (alias->item));
  if (item == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  switch (gmenu_tree_item_get_type (item))
    {
    case GMENU_TREE_ITEM_DIRECTORY:
      retval = (PyObject *) pygmenu_tree_directory_wrap (GMENU_TREE_DIRECTORY (item));
      break;

    case GMENU_TREE_ITEM_ENTRY:
      retval = (PyObject *) pygmenu_tree_entry_wrap (GMENU_TREE_ENTRY (item));
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  gmenu_tree_item_unref (item);

  return retval;
}

static PyObject *
pygmenu_tree_alias_getattro (PyGMenuTreeAlias *self,
			     PyObject         *py_attr)
{
  if (PyString_Check (py_attr))
    {
      char *attr;
  
      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "__members__"))
	{
	  return Py_BuildValue ("[ssss]",
				"type",
				"parent",
				"directory",
				"item");
	}
      else if (!strcmp (attr, "type"))
	{
	  return pygmenu_tree_item_get_type ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "parent"))
	{
	  return pygmenu_tree_item_get_parent ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "directory"))
	{
	  return pygmenu_tree_alias_get_directory ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "item"))
	{
	  return pygmenu_tree_alias_get_item ((PyObject *) self, NULL);
	}
    }

  return PyObject_GenericGetAttr ((PyObject *) self, py_attr);
}

static struct PyMethodDef pygmenu_tree_alias_methods[] =
{
  { "get_directory", pygmenu_tree_alias_get_directory, METH_VARARGS },
  { "get_item",      pygmenu_tree_alias_get_item,      METH_VARARGS },
  { NULL,            NULL,                             0            }
};

static PyTypeObject PyGMenuTreeAlias_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                             /* ob_size */
  "gmenu.Alias",                                 /* tp_name */
  sizeof (PyGMenuTreeAlias),                     /* tp_basicsize */
  0,                                             /* tp_itemsize */
  (destructor) pygmenu_tree_item_dealloc,        /* tp_dealloc */
  (printfunc)0,                                  /* tp_print */
  (getattrfunc)0,                                /* tp_getattr */
  (setattrfunc)0,                                /* tp_setattr */
  (cmpfunc)0,                                    /* tp_compare */
  (reprfunc)0,                                   /* tp_repr */
  0,                                             /* tp_as_number */
  0,                                             /* tp_as_sequence */
  0,                                             /* tp_as_mapping */
  (hashfunc)0,                                   /* tp_hash */
  (ternaryfunc)0,                                /* tp_call */
  (reprfunc)0,                                   /* tp_str */
  (getattrofunc) pygmenu_tree_alias_getattro,    /* tp_getattro */
  (setattrofunc)0,                               /* tp_setattro */
  0,                                             /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                            /* tp_flags */
  NULL,                                          /* Documentation string */
  (traverseproc)0,                               /* tp_traverse */
  (inquiry)0,                                    /* tp_clear */
  (richcmpfunc)0,                                /* tp_richcompare */
  0,                                             /* tp_weaklistoffset */
  (getiterfunc)0,                                /* tp_iter */
  (iternextfunc)0,                               /* tp_iternext */
  pygmenu_tree_alias_methods,                    /* tp_methods */
  0,                                             /* tp_members */
  0,                                             /* tp_getset */
  (PyTypeObject *)0,                             /* tp_base */
  (PyObject *)0,                                 /* tp_dict */
  0,                                             /* tp_descr_get */
  0,                                             /* tp_descr_set */
  0,                                             /* tp_dictoffset */
  (initproc)0,                                   /* tp_init */
  0,                                             /* tp_alloc */
  0,                                             /* tp_new */
  0,                                             /* tp_free */
  (inquiry)0,                                    /* tp_is_gc */
  (PyObject *)0,                                 /* tp_bases */
};

static PyGMenuTreeAlias *
pygmenu_tree_alias_wrap (GMenuTreeAlias *alias)
{
  PyGMenuTreeAlias *retval;

  if ((retval = gmenu_tree_item_get_user_data (GMENU_TREE_ITEM (alias))) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTreeAlias *) PyObject_NEW (PyGMenuTreeAlias,
						    &PyGMenuTreeAlias_Type)))
    return NULL;

  retval->item = gmenu_tree_item_ref (alias);

  gmenu_tree_item_set_user_data (GMENU_TREE_ITEM (alias), retval, NULL);

  return retval;
}

static PyObject *
pygmenu_tree_get_menu_file (PyObject *self,
			    PyObject *args)
{
  PyGMenuTree *tree;
  const char  *menu_file;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Tree.get_menu_file"))
	return NULL;
    }

  tree = (PyGMenuTree *) self;

  menu_file = gmenu_tree_get_menu_file (tree->tree);
  if (menu_file == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  return PyString_FromString (menu_file);
}

static PyObject *
pygmenu_tree_get_root_directory (PyObject *self,
				 PyObject *args)
{
  PyGMenuTree          *tree;
  GMenuTreeDirectory   *directory;
  PyGMenuTreeDirectory *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Tree.get_root_directory"))
	return NULL;
    }

  tree = (PyGMenuTree *) self;

  directory = gmenu_tree_get_root_directory (tree->tree);
  if (directory == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_directory_wrap (directory);

  gmenu_tree_item_unref (directory);

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_get_directory_from_path (PyObject *self,
				      PyObject *args)
{
  PyGMenuTree          *tree;
  GMenuTreeDirectory   *directory;
  PyGMenuTreeDirectory *retval;
  char                 *path;

  if (!PyArg_ParseTuple (args, "s:gmenu.Tree.get_directory_from_path", &path))
    return NULL;

  tree = (PyGMenuTree *) self;

  directory = gmenu_tree_get_directory_from_path (tree->tree, path);
  if (directory == NULL)
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_directory_wrap (directory);

  gmenu_tree_item_unref (directory);

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_get_sort_key (PyObject *self,
			   PyObject *args)
{
  PyGMenuTree *tree;
  PyObject    *retval;

  if (args != NULL)
    {
      if (!PyArg_ParseTuple (args, ":gmenu.Tree.get_sort_key"))
	return NULL;
    }

  tree = (PyGMenuTree *) self;

  switch (gmenu_tree_get_sort_key (tree->tree))
    {
    case GMENU_TREE_SORT_NAME:
      retval = lookup_item_type_str ("SORT_NAME");
      break;

    case GMENU_TREE_SORT_DISPLAY_NAME:
      retval = lookup_item_type_str ("SORT_DISPLAY_NAME");
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  return (PyObject *) retval;
}

static PyObject *
pygmenu_tree_set_sort_key (PyObject *self,
			   PyObject *args)
{
  PyGMenuTree *tree;
  int          sort_key;

  if (!PyArg_ParseTuple (args, "i:gmenu.Tree.set_sort_key", &sort_key))
    return NULL;

  tree = (PyGMenuTree *) self;

  gmenu_tree_set_sort_key (tree->tree, sort_key);

  return Py_None;
}

static PyGMenuTreeCallback *
pygmenu_tree_callback_new (PyObject *tree,
			   PyObject *callback,
			   PyObject *user_data)
{
  PyGMenuTreeCallback *retval;

  retval = g_new0 (PyGMenuTreeCallback, 1);

  Py_INCREF (tree);
  retval->tree = tree;

  Py_INCREF (callback);
  retval->callback = callback;

  Py_XINCREF (user_data);
  retval->user_data = user_data;

  return retval;
}

static void
pygmenu_tree_callback_free (PyGMenuTreeCallback *callback)
{
  Py_XDECREF (callback->user_data);
  callback->user_data = NULL;

  Py_DECREF (callback->callback);
  callback->callback = NULL;

  Py_DECREF (callback->tree);
  callback->tree = NULL;

  g_free (callback);
}

static void
pygmenu_tree_handle_monitor_callback (GMenuTree            *tree,
				      PyGMenuTreeCallback *callback)
{
  PyObject *args;
  PyObject *ret;
  PyGILState_STATE gstate;

  gstate = PyGILState_Ensure();

  args = PyTuple_New (callback->user_data ? 2 : 1);

  Py_INCREF (callback->tree);
  PyTuple_SET_ITEM (args, 0, callback->tree);

  if (callback->user_data != NULL)
    {
      Py_INCREF (callback->user_data);
      PyTuple_SET_ITEM (args, 1, callback->user_data);
    }

  ret = PyObject_CallObject (callback->callback, args);

  Py_XDECREF (ret);
  Py_DECREF (args);

  PyGILState_Release(gstate);
}

static PyObject *
pygmenu_tree_add_monitor (PyObject *self,
			  PyObject *args)
{
  PyGMenuTree         *tree;
  PyGMenuTreeCallback *callback;
  PyObject            *pycallback;
  PyObject            *pyuser_data = NULL;

  if (!PyArg_ParseTuple (args, "O|O:gmenu.Tree.add_monitor", &pycallback, &pyuser_data))
    return NULL;
  if (!PyCallable_Check(pycallback))
    {
      PyErr_SetString(PyExc_TypeError, "callback must be callable");
      return NULL;
    }

  tree = (PyGMenuTree *) self;

  callback = pygmenu_tree_callback_new (self, pycallback, pyuser_data);

  tree->callbacks = g_slist_append (tree->callbacks, callback);

  {
    GMenuTreeDirectory *dir = gmenu_tree_get_root_directory (tree->tree);
    if (dir)
      gmenu_tree_item_unref (dir);
  }

  gmenu_tree_add_monitor (tree->tree,
			 (GMenuTreeChangedFunc) pygmenu_tree_handle_monitor_callback,
			 callback);

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
pygmenu_tree_remove_monitor (PyObject *self,
			     PyObject *args)
{

  PyGMenuTree *tree;
  PyObject    *pycallback;
  PyObject    *pyuser_data;
  GSList      *tmp;

  if (!PyArg_ParseTuple (args, "O|O:gmenu.Tree.remove_monitor", &pycallback, &pyuser_data))
    return NULL;

  tree = (PyGMenuTree *) self;

  tmp = tree->callbacks;
  while (tmp != NULL)
    {
      PyGMenuTreeCallback *callback = tmp->data;
      GSList              *next     = tmp->next;

      if (callback->callback  == pycallback &&
	  callback->user_data == pyuser_data)
	{
	  tree->callbacks = g_slist_delete_link (tree->callbacks, tmp);
	  pygmenu_tree_callback_free (callback);
	}

      tmp = next;
    }

  Py_INCREF (Py_None);
  return Py_None;
}

static void
pygmenu_tree_dealloc (PyGMenuTree *self)
{
  g_slist_foreach (self->callbacks,
		   (GFunc) pygmenu_tree_callback_free,
		   NULL);
  g_slist_free (self->callbacks);
  self->callbacks = NULL;

  if (self->tree != NULL)
    gmenu_tree_unref (self->tree);
  self->tree = NULL;

  PyObject_DEL (self);
}

static PyObject *
pygmenu_tree_getattro (PyGMenuTree *self,
		       PyObject    *py_attr)
{
  if (PyString_Check (py_attr))
    {
      char *attr;
  
      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "__members__"))
	{
	  return Py_BuildValue ("[sss]", "root", "menu_file", "sort_key");
	}
      else if (!strcmp (attr, "root"))
	{
	  return pygmenu_tree_get_root_directory ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "menu_file"))
	{
	  return pygmenu_tree_get_menu_file ((PyObject *) self, NULL);
	}
      else if (!strcmp (attr, "sort_key"))
	{
	  return pygmenu_tree_get_sort_key ((PyObject *) self, NULL);
	}
    }

  return PyObject_GenericGetAttr ((PyObject *) self, py_attr);
}

static int
pygmenu_tree_setattro (PyGMenuTree *self,
		       PyObject    *py_attr,
		       PyObject    *py_value)
{
  PyGMenuTree *tree;

  tree = (PyGMenuTree *) self;

  if (PyString_Check (py_attr))
    {
      char *attr;

      attr = PyString_AsString (py_attr);

      if (!strcmp (attr, "sort_key"))
	{
	  if (PyInt_Check (py_value))
	    {
	      int sort_key;

	      sort_key = PyInt_AsLong (py_value);
	      if (sort_key < GMENU_TREE_SORT_FIRST || sort_key > GMENU_TREE_SORT_LAST)
		return -1;
	      gmenu_tree_set_sort_key (tree->tree, sort_key);

	      return 0;
	    }
	}
    }

  return -1;
}

static struct PyMethodDef pygmenu_tree_methods[] =
{
  { "get_menu_file",           pygmenu_tree_get_menu_file,           METH_VARARGS },
  { "get_root_directory",      pygmenu_tree_get_root_directory,      METH_VARARGS },
  { "get_directory_from_path", pygmenu_tree_get_directory_from_path, METH_VARARGS },
  { "get_sort_key",            pygmenu_tree_get_sort_key,            METH_VARARGS },
  { "set_sort_key",            pygmenu_tree_set_sort_key,            METH_VARARGS },
  { "add_monitor",             pygmenu_tree_add_monitor,             METH_VARARGS },
  { "remove_monitor",          pygmenu_tree_remove_monitor,          METH_VARARGS },
  { NULL,                      NULL,                                 0            }
};

static PyTypeObject PyGMenuTree_Type = 
{
  PyObject_HEAD_INIT(NULL)
  0,                                   /* ob_size */
  "gmenu.Tree",                        /* tp_name */
  sizeof (PyGMenuTree),                /* tp_basicsize */
  0,                                   /* tp_itemsize */
  (destructor) pygmenu_tree_dealloc,   /* tp_dealloc */
  (printfunc)0,                        /* tp_print */
  (getattrfunc)0,                      /* tp_getattr */
  (setattrfunc)0,                      /* tp_setattr */
  (cmpfunc)0,                          /* tp_compare */
  (reprfunc)0,                         /* tp_repr */
  0,                                   /* tp_as_number */
  0,                                   /* tp_as_sequence */
  0,                                   /* tp_as_mapping */
  (hashfunc)0,                         /* tp_hash */
  (ternaryfunc)0,                      /* tp_call */
  (reprfunc)0,                         /* tp_str */
  (getattrofunc)pygmenu_tree_getattro, /* tp_getattro */
  (setattrofunc)pygmenu_tree_setattro, /* tp_setattro */
  0,                                   /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                  /* tp_flags */
  NULL,                                /* Documentation string */
  (traverseproc)0,                     /* tp_traverse */
  (inquiry)0,                          /* tp_clear */
  (richcmpfunc)0,                      /* tp_richcompare */
  0,                                   /* tp_weaklistoffset */
  (getiterfunc)0,                      /* tp_iter */
  (iternextfunc)0,                     /* tp_iternext */
  pygmenu_tree_methods,                /* tp_methods */
  0,                                   /* tp_members */
  0,                                   /* tp_getset */
  (PyTypeObject *)0,                   /* tp_base */
  (PyObject *)0,                       /* tp_dict */
  0,                                   /* tp_descr_get */
  0,                                   /* tp_descr_set */
  0,                                   /* tp_dictoffset */
  (initproc)0,                         /* tp_init */
  0,                                   /* tp_alloc */
  0,                                   /* tp_new */
  0,                                   /* tp_free */
  (inquiry)0,                          /* tp_is_gc */
  (PyObject *)0,                       /* tp_bases */
};

static PyGMenuTree *
pygmenu_tree_wrap (GMenuTree *tree)
{
  PyGMenuTree *retval;

  if ((retval = gmenu_tree_get_user_data (tree)) != NULL)
    {
      Py_INCREF (retval);
      return retval;
    }

  if (!(retval = (PyGMenuTree *) PyObject_NEW (PyGMenuTree, &PyGMenuTree_Type)))
    return NULL;

  retval->tree      = gmenu_tree_ref (tree);
  retval->callbacks = NULL;

  gmenu_tree_set_user_data (tree, retval, NULL);

  return retval;
}

static PyObject *
pygmenu_lookup_tree (PyObject *self,
		     PyObject *args)
{
  char        *menu_file;
  GMenuTree   *tree;
  PyGMenuTree *retval;
  int          flags;

  flags = GMENU_TREE_FLAGS_NONE;

  if (!PyArg_ParseTuple (args, "s|i:gmenu.lookup_tree", &menu_file, &flags))
    return NULL;

  if (!(tree = gmenu_tree_lookup (menu_file, flags)))
    {
      Py_INCREF (Py_None);
      return Py_None;
    }

  retval = pygmenu_tree_wrap (tree);

  gmenu_tree_unref (tree);

  return (PyObject *) retval;
}

static struct PyMethodDef pygmenu_methods[] =
{
  { "lookup_tree", pygmenu_lookup_tree, METH_VARARGS },
  { NULL,          NULL,                0            }
};

void initgmenu (void);

DL_EXPORT (void)
initgmenu (void)
{
  PyObject *mod;

  mod = Py_InitModule4 ("gmenu", pygmenu_methods, 0, 0, PYTHON_API_VERSION);

#define REGISTER_TYPE(t, n)  G_STMT_START                \
    {                                                    \
      t.ob_type = &PyType_Type;                          \
      PyType_Ready (&t);                                 \
      PyModule_AddObject (mod, n, (PyObject *) &t);  \
    } G_STMT_END

  REGISTER_TYPE (PyGMenuTree_Type,     "Tree");
  REGISTER_TYPE (PyGMenuTreeItem_Type, "Item");

#define REGISTER_ITEM_TYPE(t, n) G_STMT_START            \
    {                                                    \
      t.ob_type = &PyType_Type;                          \
      t.tp_base = &PyGMenuTreeItem_Type;                 \
      PyType_Ready (&t);                                 \
      PyModule_AddObject (mod, n, (PyObject *) &t);  \
    } G_STMT_END

  REGISTER_ITEM_TYPE (PyGMenuTreeDirectory_Type, "Directory");
  REGISTER_ITEM_TYPE (PyGMenuTreeEntry_Type,     "Entry");
  REGISTER_ITEM_TYPE (PyGMenuTreeSeparator_Type, "Separator");
  REGISTER_ITEM_TYPE (PyGMenuTreeHeader_Type,    "Header");
  REGISTER_ITEM_TYPE (PyGMenuTreeAlias_Type,     "Alias");

  PyModule_AddIntConstant (mod, "TYPE_INVALID",   GMENU_TREE_ITEM_INVALID);
  PyModule_AddIntConstant (mod, "TYPE_DIRECTORY", GMENU_TREE_ITEM_DIRECTORY);
  PyModule_AddIntConstant (mod, "TYPE_ENTRY",     GMENU_TREE_ITEM_ENTRY);
  PyModule_AddIntConstant (mod, "TYPE_SEPARATOR", GMENU_TREE_ITEM_SEPARATOR);
  PyModule_AddIntConstant (mod, "TYPE_HEADER",    GMENU_TREE_ITEM_HEADER);
  PyModule_AddIntConstant (mod, "TYPE_ALIAS",     GMENU_TREE_ITEM_ALIAS);

  PyModule_AddIntConstant (mod, "FLAGS_NONE",                GMENU_TREE_FLAGS_NONE);
  PyModule_AddIntConstant (mod, "FLAGS_INCLUDE_EXCLUDED",    GMENU_TREE_FLAGS_INCLUDE_EXCLUDED);
  PyModule_AddIntConstant (mod, "FLAGS_SHOW_EMPTY",          GMENU_TREE_FLAGS_SHOW_EMPTY);
  PyModule_AddIntConstant (mod, "FLAGS_INCLUDE_NODISPLAY",   GMENU_TREE_FLAGS_INCLUDE_NODISPLAY);
  PyModule_AddIntConstant (mod, "FLAGS_SHOW_ALL_SEPARATORS", GMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS);

  PyModule_AddIntConstant (mod, "SORT_NAME",         GMENU_TREE_SORT_NAME);
  PyModule_AddIntConstant (mod, "SORT_DISPLAY_NAME", GMENU_TREE_SORT_DISPLAY_NAME);
}
