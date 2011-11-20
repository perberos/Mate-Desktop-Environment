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
#include <matemenu-tree.h>

typedef struct {
	PyObject_HEAD
	MateMenuTree* tree;
	GSList* callbacks;
} PyMateMenuTree;

typedef struct {
	PyObject* tree;
	PyObject* callback;
	PyObject* user_data;
} PyMateMenuTreeCallback;

typedef struct {
	PyObject_HEAD
	MateMenuTreeItem* item;
} PyMateMenuTreeItem;

typedef PyMateMenuTreeItem PyMateMenuTreeDirectory;
typedef PyMateMenuTreeItem PyMateMenuTreeEntry;
typedef PyMateMenuTreeItem PyMateMenuTreeSeparator;
typedef PyMateMenuTreeItem PyMateMenuTreeHeader;
typedef PyMateMenuTreeItem PyMateMenuTreeAlias;

static PyMateMenuTree* pymatemenu_tree_wrap(MateMenuTree* tree);
static PyMateMenuTreeDirectory* pymatemenu_tree_directory_wrap(MateMenuTreeDirectory* directory);
static PyMateMenuTreeEntry* pymatemenu_tree_entry_wrap(MateMenuTreeEntry* entry);
static PyMateMenuTreeSeparator* pymatemenu_tree_separator_wrap(MateMenuTreeSeparator* separator);
static PyMateMenuTreeHeader* pymatemenu_tree_header_wrap(MateMenuTreeHeader* header);
static PyMateMenuTreeAlias* pymatemenu_tree_alias_wrap(MateMenuTreeAlias* alias);

static inline PyObject* lookup_item_type_str(const char* item_type_str)
{
	PyObject* module;

	module = PyDict_GetItemString(PyImport_GetModuleDict(), "matemenu");

	return PyDict_GetItemString(PyModule_GetDict(module), item_type_str);
}

static void pymatemenu_tree_item_dealloc(PyMateMenuTreeItem* self)
{
	if (self->item != NULL)
	{
		matemenu_tree_item_set_user_data(self->item, NULL, NULL);
		matemenu_tree_item_unref(self->item);
		self->item = NULL;
	}

	PyObject_DEL (self);
}

static PyObject* pymatemenu_tree_item_get_type(PyObject* self, PyObject* args)
{
	PyMateMenuTreeItem* item;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Item.get_type"))
		{
			return NULL;
		}
	}

	item = (PyMateMenuTreeItem*) self;

	switch (matemenu_tree_item_get_type(item->item))
	{
		case MATEMENU_TREE_ITEM_DIRECTORY:
			retval = lookup_item_type_str("TYPE_DIRECTORY");
			break;

		case MATEMENU_TREE_ITEM_ENTRY:
			retval = lookup_item_type_str("TYPE_ENTRY");
			break;

		case MATEMENU_TREE_ITEM_SEPARATOR:
			retval = lookup_item_type_str("TYPE_SEPARATOR");
			break;

		case MATEMENU_TREE_ITEM_HEADER:
			retval = lookup_item_type_str("TYPE_HEADER");
			break;

		case MATEMENU_TREE_ITEM_ALIAS:
			retval = lookup_item_type_str("TYPE_ALIAS");
			break;

		default:
			g_assert_not_reached();
			break;
	}

	Py_INCREF(retval);

	return retval;
}

static PyObject* pymatemenu_tree_item_get_parent(PyObject* self, PyObject* args)
{
	PyMateMenuTreeItem* item;
	MateMenuTreeDirectory* parent;
	PyMateMenuTreeDirectory* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Item.get_parent"))
		{
			return NULL;
		}
	}

	item = (PyMateMenuTreeItem*) self;

	parent = matemenu_tree_item_get_parent(item->item);

	if (parent == NULL)
	{
		Py_INCREF(Py_None);

		return Py_None;
	}

	retval = pymatemenu_tree_directory_wrap(parent);

	matemenu_tree_item_unref(parent);

	return (PyObject*) retval;
}

static struct PyMethodDef pymatemenu_tree_item_methods[] = {
	{"get_type", pymatemenu_tree_item_get_type,   METH_VARARGS},
	{"get_parent", pymatemenu_tree_item_get_parent, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTreeItem_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                             /* ob_size */
	"matemenu.Item",                               /* tp_name */
	sizeof(PyMateMenuTreeItem),                    /* tp_basicsize */
	0,                                             /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,     /* tp_dealloc */
	(printfunc) 0,                                 /* tp_print */
	(getattrfunc) 0,                               /* tp_getattr */
	(setattrfunc) 0,                               /* tp_setattr */
	(cmpfunc) 0,                                   /* tp_compare */
	(reprfunc) 0,                                  /* tp_repr */
	0,                                             /* tp_as_number */
	0,                                             /* tp_as_sequence */
	0,                                             /* tp_as_mapping */
	(hashfunc) 0,                                  /* tp_hash */
	(ternaryfunc) 0,                               /* tp_call */
	(reprfunc) 0,                                  /* tp_str */
	(getattrofunc) 0,                              /* tp_getattro */
	(setattrofunc) 0,                              /* tp_setattro */
	0,                                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,      /* tp_flags */
	NULL,                                          /* Documentation string */
	(traverseproc) 0,                              /* tp_traverse */
	(inquiry) 0,                                   /* tp_clear */
	(richcmpfunc) 0,                               /* tp_richcompare */
	0,                                             /* tp_weaklistoffset */
	(getiterfunc) 0,                               /* tp_iter */
	(iternextfunc) 0,                              /* tp_iternext */
	pymatemenu_tree_item_methods,                  /* tp_methods */
	0,                                             /* tp_members */
	0,                                             /* tp_getset */
	(PyTypeObject*) 0,                             /* tp_base */
	(PyObject*) 0,                                 /* tp_dict */
	0,                                             /* tp_descr_get */
	0,                                             /* tp_descr_set */
	0,                                             /* tp_dictoffset */
	(initproc) 0,                                  /* tp_init */
	0,                                             /* tp_alloc */
	0,                                             /* tp_new */
	0,                                             /* tp_free */
	(inquiry) 0,                                   /* tp_is_gc */
	(PyObject*) 0,                                 /* tp_bases */
};

static PyObject* pymatemenu_tree_directory_get_contents(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	PyObject* retval;
	GSList* contents;
	GSList* tmp;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_contents"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	retval = PyList_New(0);

	contents = matemenu_tree_directory_get_contents(MATEMENU_TREE_DIRECTORY(directory->item));

	tmp = contents;

	while (tmp != NULL)
	{
		MateMenuTreeItem* item = tmp->data;
		PyObject* pyitem;

		switch (matemenu_tree_item_get_type(item))
		{
			case MATEMENU_TREE_ITEM_DIRECTORY:
				pyitem = (PyObject*) pymatemenu_tree_directory_wrap(MATEMENU_TREE_DIRECTORY(item));
				break;

			case MATEMENU_TREE_ITEM_ENTRY:
				pyitem = (PyObject*) pymatemenu_tree_entry_wrap(MATEMENU_TREE_ENTRY(item));
				break;

			case MATEMENU_TREE_ITEM_SEPARATOR:
				pyitem = (PyObject*) pymatemenu_tree_separator_wrap(MATEMENU_TREE_SEPARATOR(item));
				break;

			case MATEMENU_TREE_ITEM_HEADER:
				pyitem = (PyObject*) pymatemenu_tree_header_wrap(MATEMENU_TREE_HEADER(item));
				break;

			case MATEMENU_TREE_ITEM_ALIAS:
				pyitem = (PyObject*) pymatemenu_tree_alias_wrap(MATEMENU_TREE_ALIAS(item));
				break;

			default:
				g_assert_not_reached();
				break;
		}

		PyList_Append(retval, pyitem);
		Py_DECREF(pyitem);

		matemenu_tree_item_unref(item);

		tmp = tmp->next;
	}

	g_slist_free(contents);

	return retval;
}

static PyObject* pymatemenu_tree_directory_get_name(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	const char* name;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_name"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	name = matemenu_tree_directory_get_name(MATEMENU_TREE_DIRECTORY(directory->item));

	if (name == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(name);
}

static PyObject* pymatemenu_tree_directory_get_comment(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	const char* comment;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_comment"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	comment = matemenu_tree_directory_get_comment(MATEMENU_TREE_DIRECTORY(directory->item));

	if (comment == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(comment);
}

static PyObject* pymatemenu_tree_directory_get_icon(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	const char* icon;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_icon"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	icon = matemenu_tree_directory_get_icon(MATEMENU_TREE_DIRECTORY(directory->item));

	if (icon == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
    }

	return PyString_FromString(icon);
}

static PyObject* pymatemenu_tree_directory_get_desktop_file_path(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	const char* path;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_desktop_file_path"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	path = matemenu_tree_directory_get_desktop_file_path(MATEMENU_TREE_DIRECTORY(directory->item));

	if (path == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(path);
}

static PyObject* pymatemenu_tree_directory_get_menu_id(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	const char* menu_id;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Directory.get_menu_id"))
		{
			return NULL;
		}
    }

	directory = (PyMateMenuTreeDirectory*) self;

	menu_id = matemenu_tree_directory_get_menu_id(MATEMENU_TREE_DIRECTORY(directory->item));

	if (menu_id == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(menu_id);
}

static PyObject* pymatemenu_tree_directory_get_tree(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	MateMenuTree* tree;
	PyMateMenuTree* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Item.get_tree"))
		{
			return NULL;
		}
	}

	directory = (PyMateMenuTreeDirectory*) self;

	tree = matemenu_tree_directory_get_tree(MATEMENU_TREE_DIRECTORY(directory->item));

	if (tree == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = pymatemenu_tree_wrap(tree);

	matemenu_tree_unref(tree);

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_directory_make_path(PyObject* self, PyObject* args)
{
	PyMateMenuTreeDirectory* directory;
	PyMateMenuTreeEntry* entry;
	PyObject* retval;
	char* path;

	if (!PyArg_ParseTuple(args, "O:matemenu.Directory.make_path", &entry))
	{
		return NULL;
	}

	directory = (PyMateMenuTreeDirectory*) self;

	path = matemenu_tree_directory_make_path(MATEMENU_TREE_DIRECTORY(directory->item), MATEMENU_TREE_ENTRY(entry->item));

	if (path == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = PyString_FromString(path);

	g_free(path);

	return retval;
}

static PyObject* pymatemenu_tree_directory_getattro(PyMateMenuTreeDirectory* self, PyObject* py_attr)
{
	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "__members__"))
		{
			return Py_BuildValue("[sssssssss]",
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
		else if (!strcmp(attr, "type"))
		{
			return pymatemenu_tree_item_get_type((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "parent"))
		{
			return pymatemenu_tree_item_get_parent((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "contents"))
		{
			return pymatemenu_tree_directory_get_contents((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "name"))
		{
			return pymatemenu_tree_directory_get_name((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "comment"))
		{
			return pymatemenu_tree_directory_get_comment((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "icon"))
		{
			return pymatemenu_tree_directory_get_icon((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "desktop_file_path"))
		{
			return pymatemenu_tree_directory_get_desktop_file_path((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "menu_id"))
		{
			return pymatemenu_tree_directory_get_menu_id((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "tree"))
		{
			return pymatemenu_tree_directory_get_tree((PyObject*) self, NULL);
		}
	}

	return PyObject_GenericGetAttr((PyObject*) self, py_attr);
}

static struct PyMethodDef pymatemenu_tree_directory_methods[] = {
	{"get_contents", pymatemenu_tree_directory_get_contents, METH_VARARGS},
	{"get_name", pymatemenu_tree_directory_get_name, METH_VARARGS},
	{"get_comment", pymatemenu_tree_directory_get_comment, METH_VARARGS},
	{"get_icon", pymatemenu_tree_directory_get_icon, METH_VARARGS},
	{"get_desktop_file_path", pymatemenu_tree_directory_get_desktop_file_path, METH_VARARGS},
	{"get_menu_id", pymatemenu_tree_directory_get_menu_id, METH_VARARGS},
	{"get_tree", pymatemenu_tree_directory_get_tree, METH_VARARGS},
	{"make_path", pymatemenu_tree_directory_make_path, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTreeDirectory_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                              /* ob_size */
	"matemenu.Directory",                           /* tp_name */
	sizeof(PyMateMenuTreeDirectory),                /* tp_basicsize */
	0,                                              /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,      /* tp_dealloc */
	(printfunc) 0,                                  /* tp_print */
	(getattrfunc) 0,                                /* tp_getattr */
	(setattrfunc) 0,                                /* tp_setattr */
	(cmpfunc) 0,                                    /* tp_compare */
	(reprfunc) 0,                                   /* tp_repr */
	0,                                              /* tp_as_number */
	0,                                              /* tp_as_sequence */
	0,                                              /* tp_as_mapping */
	(hashfunc) 0,                                   /* tp_hash */
	(ternaryfunc) 0,                                /* tp_call */
	(reprfunc) 0,                                   /* tp_str */
	(getattrofunc) pymatemenu_tree_directory_getattro, /* tp_getattro */
	(setattrofunc) 0,                               /* tp_setattro */
	0,                                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                             /* tp_flags */
	NULL,                                           /* Documentation string */
	(traverseproc) 0,                               /* tp_traverse */
	(inquiry) 0,                                    /* tp_clear */
	(richcmpfunc) 0,                                /* tp_richcompare */
	0,                                              /* tp_weaklistoffset */
	(getiterfunc) 0,                                /* tp_iter */
	(iternextfunc) 0,                               /* tp_iternext */
	pymatemenu_tree_directory_methods,              /* tp_methods */
	0,                                              /* tp_members */
	0,                                              /* tp_getset */
	(PyTypeObject*) 0,                              /* tp_base */
	(PyObject*) 0,                                  /* tp_dict */
	0,                                              /* tp_descr_get */
	0,                                              /* tp_descr_set */
	0,                                              /* tp_dictoffset */
	(initproc) 0,                                   /* tp_init */
	0,                                              /* tp_alloc */
	0,                                              /* tp_new */
	0,                                              /* tp_free */
	(inquiry) 0,                                    /* tp_is_gc */
	(PyObject*) 0,                                  /* tp_bases */
};

static PyMateMenuTreeDirectory* pymatemenu_tree_directory_wrap(MateMenuTreeDirectory* directory)
{
	PyMateMenuTreeDirectory* retval;

	if ((retval = matemenu_tree_item_get_user_data(MATEMENU_TREE_ITEM(directory))) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTreeDirectory*) PyObject_NEW(PyMateMenuTreeDirectory, &PyMateMenuTreeDirectory_Type)))
	{
		return NULL;
	}

	retval->item = matemenu_tree_item_ref(directory);

	matemenu_tree_item_set_user_data(MATEMENU_TREE_ITEM(directory), retval, NULL);

	return retval;
}

static PyObject* pymatemenu_tree_entry_get_name(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* name;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_name"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	name = matemenu_tree_entry_get_name(MATEMENU_TREE_ENTRY(entry->item));

	if (name == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(name);
}

static PyObject* pymatemenu_tree_entry_get_generic_name(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* generic_name;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_generic_name"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	generic_name = matemenu_tree_entry_get_generic_name(MATEMENU_TREE_ENTRY(entry->item));

	if (generic_name == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(generic_name);
}

static PyObject* pymatemenu_tree_entry_get_display_name(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* display_name;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_display_name"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	display_name = matemenu_tree_entry_get_display_name(MATEMENU_TREE_ENTRY(entry->item));

	if (display_name == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(display_name);
}

static PyObject* pymatemenu_tree_entry_get_comment(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* comment;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_comment"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	comment = matemenu_tree_entry_get_comment(MATEMENU_TREE_ENTRY(entry->item));

	if (comment == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(comment);
}

static PyObject* pymatemenu_tree_entry_get_icon(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* icon;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_icon"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	icon = matemenu_tree_entry_get_icon(MATEMENU_TREE_ENTRY(entry->item));

	if (icon == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(icon);
}

static PyObject* pymatemenu_tree_entry_get_exec(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* exec;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_exec"))
		{
			return NULL;
		}
    }

	entry = (PyMateMenuTreeEntry*) self;

	exec = matemenu_tree_entry_get_exec(MATEMENU_TREE_ENTRY(entry->item));

	if (exec == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(exec);
}

static PyObject* pymatemenu_tree_entry_get_launch_in_terminal(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_launch_in_terminal"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	if (matemenu_tree_entry_get_launch_in_terminal(MATEMENU_TREE_ENTRY(entry->item)))
	{
		retval = Py_True;
	}
	else
	{
		retval = Py_False;
	}

	Py_INCREF(retval);

	return retval;
}

static PyObject* pymatemenu_tree_entry_get_desktop_file_path(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* desktop_file_path;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_desktop_file_path"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	desktop_file_path = matemenu_tree_entry_get_desktop_file_path(MATEMENU_TREE_ENTRY(entry->item));

	if (desktop_file_path == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(desktop_file_path);
}

static PyObject* pymatemenu_tree_entry_get_desktop_file_id(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	const char* desktop_file_id;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_desktop_file_id"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	desktop_file_id = matemenu_tree_entry_get_desktop_file_id(MATEMENU_TREE_ENTRY(entry->item));

	if (desktop_file_id == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(desktop_file_id);
}

static PyObject* pymatemenu_tree_entry_get_is_excluded(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_is_excluded"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	retval = matemenu_tree_entry_get_is_excluded(MATEMENU_TREE_ENTRY(entry->item)) ? Py_True : Py_False;
	Py_INCREF(retval);

	return retval;
}

static PyObject* pymatemenu_tree_entry_get_is_nodisplay(PyObject* self, PyObject* args)
{
	PyMateMenuTreeEntry* entry;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Entry.get_is_nodisplay"))
		{
			return NULL;
		}
	}

	entry = (PyMateMenuTreeEntry*) self;

	if (matemenu_tree_entry_get_is_nodisplay(MATEMENU_TREE_ENTRY(entry->item)))
	{
		retval = Py_True;
	}
	else
	{
		retval = Py_False;
	}

	Py_INCREF(retval);

	return retval;
}

static PyObject* pymatemenu_tree_entry_getattro(PyMateMenuTreeEntry* self, PyObject* py_attr)
{
	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "__members__"))
		{
			return Py_BuildValue("[sssssssssss]",
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
		else if (!strcmp(attr, "type"))
		{
			return pymatemenu_tree_item_get_type((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "parent"))
		{
			return pymatemenu_tree_item_get_parent((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "name"))
		{
			return pymatemenu_tree_entry_get_name((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "generic_name"))
		{
			return pymatemenu_tree_entry_get_generic_name((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "display_name"))
		{
			return pymatemenu_tree_entry_get_display_name((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "comment"))
		{
			return pymatemenu_tree_entry_get_comment((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "icon"))
		{
			return pymatemenu_tree_entry_get_icon((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "exec_info"))
		{
			return pymatemenu_tree_entry_get_exec((PyObject*) self, NULL);
		}
			else if (!strcmp(attr, "launch_in_terminal"))
		{
			return pymatemenu_tree_entry_get_launch_in_terminal((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "desktop_file_path"))
		{
			return pymatemenu_tree_entry_get_desktop_file_path((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "desktop_file_id"))
		{
			return pymatemenu_tree_entry_get_desktop_file_id((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "is_excluded"))
		{
			return pymatemenu_tree_entry_get_is_excluded((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "is_nodisplay"))
		{
			return pymatemenu_tree_entry_get_is_nodisplay((PyObject*) self, NULL);
		}
	}

	return PyObject_GenericGetAttr((PyObject*) self, py_attr);
}

static struct PyMethodDef pymatemenu_tree_entry_methods[] = {
	{"get_name", pymatemenu_tree_entry_get_name, METH_VARARGS},
	{"get_generic_name", pymatemenu_tree_entry_get_generic_name, METH_VARARGS},
	{"get_display_name", pymatemenu_tree_entry_get_display_name, METH_VARARGS},
	{"get_comment", pymatemenu_tree_entry_get_comment, METH_VARARGS},
	{"get_icon", pymatemenu_tree_entry_get_icon, METH_VARARGS},
	{"get_exec", pymatemenu_tree_entry_get_exec, METH_VARARGS},
	{"get_launch_in_terminal", pymatemenu_tree_entry_get_launch_in_terminal, METH_VARARGS},
	{"get_desktop_file_path", pymatemenu_tree_entry_get_desktop_file_path, METH_VARARGS},
	{"get_desktop_file_id", pymatemenu_tree_entry_get_desktop_file_id, METH_VARARGS},
	{"get_is_excluded", pymatemenu_tree_entry_get_is_excluded, METH_VARARGS},
	{"get_is_nodisplay", pymatemenu_tree_entry_get_is_nodisplay, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTreeEntry_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                             /* ob_size */
	"matemenu.Entry",                              /* tp_name */
	sizeof(PyMateMenuTreeEntry),                   /* tp_basicsize */
	0,                                             /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,     /* tp_dealloc */
	(printfunc) 0,                                 /* tp_print */
	(getattrfunc) 0,                               /* tp_getattr */
	(setattrfunc) 0,                               /* tp_setattr */
	(cmpfunc) 0,                                   /* tp_compare */
	(reprfunc) 0,                                  /* tp_repr */
	0,                                             /* tp_as_number */
	0,                                             /* tp_as_sequence */
	0,                                             /* tp_as_mapping */
	(hashfunc) 0,                                  /* tp_hash */
	(ternaryfunc) 0,                               /* tp_call */
	(reprfunc) 0,                                  /* tp_str */
	(getattrofunc) pymatemenu_tree_entry_getattro, /* tp_getattro */
	(setattrofunc) 0,                              /* tp_setattro */
	0,                                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                            /* tp_flags */
	NULL,                                          /* Documentation string */
	(traverseproc) 0,                              /* tp_traverse */
	(inquiry) 0,                                   /* tp_clear */
	(richcmpfunc) 0,                               /* tp_richcompare */
	0,                                             /* tp_weaklistoffset */
	(getiterfunc) 0,                               /* tp_iter */
	(iternextfunc) 0,                              /* tp_iternext */
	pymatemenu_tree_entry_methods,                 /* tp_methods */
	0,                                             /* tp_members */
	0,                                             /* tp_getset */
	(PyTypeObject*) 0,                             /* tp_base */
	(PyObject*) 0,                                 /* tp_dict */
	0,                                             /* tp_descr_get */
	0,                                             /* tp_descr_set */
	0,                                             /* tp_dictoffset */
	(initproc) 0,                                  /* tp_init */
	0,                                             /* tp_alloc */
	0,                                             /* tp_new */
	0,                                             /* tp_free */
	(inquiry) 0,                                   /* tp_is_gc */
	(PyObject*) 0,                                 /* tp_bases */
};

static PyMateMenuTreeEntry* pymatemenu_tree_entry_wrap(MateMenuTreeEntry* entry)
{
	PyMateMenuTreeEntry* retval;

	if ((retval = matemenu_tree_item_get_user_data(MATEMENU_TREE_ITEM(entry))) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTreeEntry*) PyObject_NEW(PyMateMenuTreeEntry, &PyMateMenuTreeEntry_Type)))
	{
		return NULL;
	}

	retval->item = matemenu_tree_item_ref(entry);

	matemenu_tree_item_set_user_data(MATEMENU_TREE_ITEM(entry), retval, NULL);

	return retval;
}

static PyTypeObject PyMateMenuTreeSeparator_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                             /* ob_size */
	"matemenu.Separator",                          /* tp_name */
	sizeof(PyMateMenuTreeSeparator),               /* tp_basicsize */
	0,                                             /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,     /* tp_dealloc */
	(printfunc) 0,                                 /* tp_print */
	(getattrfunc) 0,                               /* tp_getattr */
	(setattrfunc) 0,                               /* tp_setattr */
	(cmpfunc) 0,                                   /* tp_compare */
	(reprfunc) 0,                                  /* tp_repr */
	0,                                             /* tp_as_number */
	0,                                             /* tp_as_sequence */
	0,                                             /* tp_as_mapping */
	(hashfunc) 0,                                  /* tp_hash */
	(ternaryfunc) 0,                               /* tp_call */
	(reprfunc) 0,                                  /* tp_str */
	(getattrofunc) 0,                              /* tp_getattro */
	(setattrofunc) 0,                              /* tp_setattro */
	0,                                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                            /* tp_flags */
	NULL,                                          /* Documentation string */
	(traverseproc) 0,                              /* tp_traverse */
	(inquiry) 0,                                   /* tp_clear */
	(richcmpfunc) 0,                               /* tp_richcompare */
	0,                                             /* tp_weaklistoffset */
	(getiterfunc) 0,                               /* tp_iter */
	(iternextfunc) 0,                              /* tp_iternext */
	NULL,                                          /* tp_methods */
	0,                                             /* tp_members */
	0,                                             /* tp_getset */
	(PyTypeObject*) 0,                             /* tp_base */
	(PyObject*) 0,                                 /* tp_dict */
	0,                                             /* tp_descr_get */
	0,                                             /* tp_descr_set */
	0,                                             /* tp_dictoffset */
	(initproc) 0,                                  /* tp_init */
	0,                                             /* tp_alloc */
	0,                                             /* tp_new */
	0,                                             /* tp_free */
	(inquiry) 0,                                   /* tp_is_gc */
	(PyObject*) 0,                                 /* tp_bases */
};

static PyMateMenuTreeSeparator* pymatemenu_tree_separator_wrap(MateMenuTreeSeparator* separator)
{
	PyMateMenuTreeSeparator* retval;

	if ((retval = matemenu_tree_item_get_user_data(MATEMENU_TREE_ITEM(separator))) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTreeSeparator*) PyObject_NEW(PyMateMenuTreeSeparator, &PyMateMenuTreeSeparator_Type)))
	{
		return NULL;
	}

	retval->item = matemenu_tree_item_ref(separator);

	matemenu_tree_item_set_user_data(MATEMENU_TREE_ITEM(separator), retval, NULL);

	return retval;
}

static PyObject* pymatemenu_tree_header_get_directory(PyObject* self, PyObject* args)
{
	PyMateMenuTreeHeader* header;
	MateMenuTreeDirectory* directory;
	PyMateMenuTreeDirectory* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Header.get_directory"))
		{
			return NULL;
		}
	}

	header = (PyMateMenuTreeHeader*) self;

	directory = matemenu_tree_header_get_directory(MATEMENU_TREE_HEADER(header->item));

	if (directory == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
    }

	retval = pymatemenu_tree_directory_wrap(directory);

	matemenu_tree_item_unref(directory);

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_header_getattro(PyMateMenuTreeHeader* self, PyObject* py_attr)
{
	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "__members__"))
		{
			return Py_BuildValue("[sss]",
				"type",
				"parent",
				"directory");
		}
		else if (!strcmp(attr, "type"))
		{
			return pymatemenu_tree_item_get_type((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "parent"))
		{
			return pymatemenu_tree_item_get_parent((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "directory"))
		{
			return pymatemenu_tree_header_get_directory((PyObject*) self, NULL);
		}
	}

	return PyObject_GenericGetAttr((PyObject*) self, py_attr);
}

static struct PyMethodDef pymatemenu_tree_header_methods[] = {
	{"get_directory", pymatemenu_tree_header_get_directory, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTreeHeader_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                             /* ob_size */
	"matemenu.Header",                             /* tp_name */
	sizeof(PyMateMenuTreeHeader),                  /* tp_basicsize */
	0,                                             /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,     /* tp_dealloc */
	(printfunc) 0,                                 /* tp_print */
	(getattrfunc) 0,                               /* tp_getattr */
	(setattrfunc) 0,                               /* tp_setattr */
	(cmpfunc) 0,                                   /* tp_compare */
	(reprfunc) 0,                                  /* tp_repr */
	0,                                             /* tp_as_number */
	0,                                             /* tp_as_sequence */
	0,                                             /* tp_as_mapping */
	(hashfunc) 0,                                  /* tp_hash */
	(ternaryfunc) 0,                               /* tp_call */
	(reprfunc) 0,                                  /* tp_str */
	(getattrofunc) pymatemenu_tree_header_getattro, /* tp_getattro */
	(setattrofunc) 0,                              /* tp_setattro */
	0,                                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                            /* tp_flags */
	NULL,                                          /* Documentation string */
	(traverseproc) 0,                              /* tp_traverse */
	(inquiry) 0,                                   /* tp_clear */
	(richcmpfunc) 0,                               /* tp_richcompare */
	0,                                             /* tp_weaklistoffset */
	(getiterfunc) 0,                               /* tp_iter */
	(iternextfunc) 0,                              /* tp_iternext */
	pymatemenu_tree_header_methods,                /* tp_methods */
	0,                                             /* tp_members */
	0,                                             /* tp_getset */
	(PyTypeObject*) 0,                             /* tp_base */
	(PyObject*) 0,                                 /* tp_dict */
	0,                                             /* tp_descr_get */
	0,                                             /* tp_descr_set */
	0,                                             /* tp_dictoffset */
	(initproc) 0,                                  /* tp_init */
	0,                                             /* tp_alloc */
	0,                                             /* tp_new */
	0,                                             /* tp_free */
	(inquiry) 0,                                   /* tp_is_gc */
	(PyObject*) 0,                                 /* tp_bases */
};

static PyMateMenuTreeHeader* pymatemenu_tree_header_wrap(MateMenuTreeHeader* header)
{
	PyMateMenuTreeHeader* retval;

	if ((retval = matemenu_tree_item_get_user_data(MATEMENU_TREE_ITEM(header))) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTreeHeader*) PyObject_NEW(PyMateMenuTreeHeader, &PyMateMenuTreeHeader_Type)))
	{
		return NULL;
	}

	retval->item = matemenu_tree_item_ref(header);

	matemenu_tree_item_set_user_data(MATEMENU_TREE_ITEM(header), retval, NULL);

	return retval;
}

static PyObject* pymatemenu_tree_alias_get_directory(PyObject*self, PyObject* args)
{
	PyMateMenuTreeAlias* alias;
	MateMenuTreeDirectory* directory;
	PyMateMenuTreeDirectory* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Alias.get_directory"))
		{
			return NULL;
		}
	}

	alias = (PyMateMenuTreeAlias*) self;

	directory = matemenu_tree_alias_get_directory(MATEMENU_TREE_ALIAS(alias->item));

	if (directory == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = pymatemenu_tree_directory_wrap(directory);

	matemenu_tree_item_unref(directory);

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_alias_get_item(PyObject* self, PyObject* args)
{
	PyMateMenuTreeAlias* alias;
	MateMenuTreeItem* item;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Alias.get_item"))
		{
			return NULL;
		}
	}

	alias = (PyMateMenuTreeAlias*) self;

	item = matemenu_tree_alias_get_item(MATEMENU_TREE_ALIAS(alias->item));

	if (item == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	switch (matemenu_tree_item_get_type(item))
	{
		case MATEMENU_TREE_ITEM_DIRECTORY:
			retval = (PyObject*) pymatemenu_tree_directory_wrap(MATEMENU_TREE_DIRECTORY(item));
			break;

		case MATEMENU_TREE_ITEM_ENTRY:
			retval = (PyObject*) pymatemenu_tree_entry_wrap(MATEMENU_TREE_ENTRY(item));
			break;

		default:
			g_assert_not_reached();
			break;
	}

	matemenu_tree_item_unref(item);

	return retval;
}

static PyObject* pymatemenu_tree_alias_getattro(PyMateMenuTreeAlias* self, PyObject* py_attr)
{
	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "__members__"))
		{
			return Py_BuildValue("[ssss]",
				"type",
				"parent",
				"directory",
				"item");
		}
		else if (!strcmp(attr, "type"))
		{
			return pymatemenu_tree_item_get_type((PyObject*) self, NULL);
		}
		  else if (!strcmp(attr, "parent"))
		{
			return pymatemenu_tree_item_get_parent((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "directory"))
		{
			return pymatemenu_tree_alias_get_directory((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "item"))
		{
			return pymatemenu_tree_alias_get_item((PyObject*) self, NULL);
		}
	}

	return PyObject_GenericGetAttr((PyObject*) self, py_attr);
}

static struct PyMethodDef pymatemenu_tree_alias_methods[] = {
	{"get_directory", pymatemenu_tree_alias_get_directory, METH_VARARGS},
	{"get_item", pymatemenu_tree_alias_get_item, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTreeAlias_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                             /* ob_size */
	"matemenu.Alias",                              /* tp_name */
	sizeof(PyMateMenuTreeAlias),                   /* tp_basicsize */
	0,                                             /* tp_itemsize */
	(destructor) pymatemenu_tree_item_dealloc,     /* tp_dealloc */
	(printfunc) 0,                                 /* tp_print */
	(getattrfunc) 0,                               /* tp_getattr */
	(setattrfunc) 0,                               /* tp_setattr */
	(cmpfunc) 0,                                   /* tp_compare */
	(reprfunc) 0,                                  /* tp_repr */
	0,                                             /* tp_as_number */
	0,                                             /* tp_as_sequence */
	0,                                             /* tp_as_mapping */
	(hashfunc) 0,                                  /* tp_hash */
	(ternaryfunc) 0,                               /* tp_call */
	(reprfunc) 0,                                  /* tp_str */
	(getattrofunc) pymatemenu_tree_alias_getattro, /* tp_getattro */
	(setattrofunc) 0,                              /* tp_setattro */
	0,                                             /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                            /* tp_flags */
	NULL,                                          /* Documentation string */
	(traverseproc) 0,                              /* tp_traverse */
	(inquiry) 0,                                   /* tp_clear */
	(richcmpfunc) 0,                               /* tp_richcompare */
	0,                                             /* tp_weaklistoffset */
	(getiterfunc) 0,                               /* tp_iter */
	(iternextfunc) 0,                              /* tp_iternext */
	pymatemenu_tree_alias_methods,                 /* tp_methods */
	0,                                             /* tp_members */
	0,                                             /* tp_getset */
	(PyTypeObject*) 0,                             /* tp_base */
	(PyObject*) 0,                                 /* tp_dict */
	0,                                             /* tp_descr_get */
	0,                                             /* tp_descr_set */
	0,                                             /* tp_dictoffset */
	(initproc) 0,                                  /* tp_init */
	0,                                             /* tp_alloc */
	0,                                             /* tp_new */
	0,                                             /* tp_free */
	(inquiry) 0,                                   /* tp_is_gc */
	(PyObject*) 0,                                 /* tp_bases */
};

static PyMateMenuTreeAlias* pymatemenu_tree_alias_wrap(MateMenuTreeAlias* alias)
{
	PyMateMenuTreeAlias* retval;

	if ((retval = matemenu_tree_item_get_user_data(MATEMENU_TREE_ITEM(alias))) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTreeAlias*) PyObject_NEW(PyMateMenuTreeAlias, &PyMateMenuTreeAlias_Type)))
	{
		return NULL;
	}

	retval->item = matemenu_tree_item_ref(alias);

	matemenu_tree_item_set_user_data(MATEMENU_TREE_ITEM(alias), retval, NULL);

	return retval;
}

static PyObject* pymatemenu_tree_get_menu_file(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	const char* menu_file;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Tree.get_menu_file"))
		{
			return NULL;
		}
	}

	tree = (PyMateMenuTree*) self;

	menu_file = matemenu_tree_get_menu_file(tree->tree);

	if (menu_file == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	return PyString_FromString(menu_file);
}

static PyObject* pymatemenu_tree_get_root_directory(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	MateMenuTreeDirectory* directory;
	PyMateMenuTreeDirectory* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Tree.get_root_directory"))
		{
			return NULL;
		}
	}

	tree = (PyMateMenuTree*) self;

	directory = matemenu_tree_get_root_directory(tree->tree);

	if (directory == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = pymatemenu_tree_directory_wrap (directory);

	matemenu_tree_item_unref(directory);

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_get_directory_from_path(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	MateMenuTreeDirectory* directory;
	PyMateMenuTreeDirectory* retval;
	char* path;

	if (!PyArg_ParseTuple(args, "s:matemenu.Tree.get_directory_from_path", &path))
	{
		return NULL;
	}

	tree = (PyMateMenuTree*) self;

	directory = matemenu_tree_get_directory_from_path(tree->tree, path);

	if (directory == NULL)
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = pymatemenu_tree_directory_wrap(directory);

	matemenu_tree_item_unref(directory);

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_get_sort_key(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	PyObject* retval;

	if (args != NULL)
	{
		if (!PyArg_ParseTuple(args, ":matemenu.Tree.get_sort_key"))
		{
			return NULL;
		}
	}

	tree = (PyMateMenuTree*) self;

	switch (matemenu_tree_get_sort_key(tree->tree))
	{
		case MATEMENU_TREE_SORT_NAME:
			retval = lookup_item_type_str("SORT_NAME");
			break;

		case MATEMENU_TREE_SORT_DISPLAY_NAME:
			retval = lookup_item_type_str("SORT_DISPLAY_NAME");
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return (PyObject*) retval;
}

static PyObject* pymatemenu_tree_set_sort_key(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	int sort_key;

	if (!PyArg_ParseTuple(args, "i:matemenu.Tree.set_sort_key", &sort_key))
	{
		return NULL;
	}

	tree = (PyMateMenuTree*) self;

	matemenu_tree_set_sort_key(tree->tree, sort_key);

	return Py_None;
}

static PyMateMenuTreeCallback* pymatemenu_tree_callback_new(PyObject* tree, PyObject* callback, PyObject* user_data)
{
	PyMateMenuTreeCallback* retval;

	retval = g_new0(PyMateMenuTreeCallback, 1);

	Py_INCREF(tree);
	retval->tree = tree;

	Py_INCREF(callback);
	retval->callback = callback;

	Py_XINCREF(user_data);
	retval->user_data = user_data;

	return retval;
}

static void pymatemenu_tree_callback_free(PyMateMenuTreeCallback* callback)
{
	Py_XDECREF(callback->user_data);
	callback->user_data = NULL;

	Py_DECREF(callback->callback);
	callback->callback = NULL;

	Py_DECREF(callback->tree);
	callback->tree = NULL;

	g_free(callback);
}

static void pymatemenu_tree_handle_monitor_callback(MateMenuTree* tree, PyMateMenuTreeCallback* callback)
{
	PyObject* args;
	PyObject* ret;
	PyGILState_STATE gstate;

	gstate = PyGILState_Ensure();

	args = PyTuple_New(callback->user_data ? 2 : 1);

	Py_INCREF(callback->tree);
	PyTuple_SET_ITEM(args, 0, callback->tree);

	if (callback->user_data != NULL)
	{
		Py_INCREF(callback->user_data);
		PyTuple_SET_ITEM(args, 1, callback->user_data);
	}

	ret = PyObject_CallObject(callback->callback, args);

	Py_XDECREF(ret);
	Py_DECREF(args);

	PyGILState_Release(gstate);
}

static PyObject* pymatemenu_tree_add_monitor(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	PyMateMenuTreeCallback* callback;
	PyObject* pycallback;
	PyObject* pyuser_data = NULL;

	if (!PyArg_ParseTuple(args, "O|O:matemenu.Tree.add_monitor", &pycallback, &pyuser_data))
	{
		return NULL;
	}

	if (!PyCallable_Check(pycallback))
	{
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		return NULL;
	}

	tree = (PyMateMenuTree*) self;

	callback = pymatemenu_tree_callback_new(self, pycallback, pyuser_data);

	tree->callbacks = g_slist_append(tree->callbacks, callback);

	{
		MateMenuTreeDirectory* dir = matemenu_tree_get_root_directory(tree->tree);
		if (dir)
		{
			matemenu_tree_item_unref(dir);
		}
	}

	matemenu_tree_add_monitor(tree->tree, (MateMenuTreeChangedFunc) pymatemenu_tree_handle_monitor_callback, callback);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* pymatemenu_tree_remove_monitor(PyObject* self, PyObject* args)
{
	PyMateMenuTree* tree;
	PyObject* pycallback;
	PyObject* pyuser_data;
	GSList* tmp;

	if (!PyArg_ParseTuple(args, "O|O:matemenu.Tree.remove_monitor", &pycallback, &pyuser_data))
	{
		return NULL;
	}

	tree = (PyMateMenuTree*) self;

	tmp = tree->callbacks;

	while (tmp != NULL)
	{
		PyMateMenuTreeCallback* callback = tmp->data;
		GSList* next = tmp->next;

		if (callback->callback  == pycallback && callback->user_data == pyuser_data)
		{
			tree->callbacks = g_slist_delete_link(tree->callbacks, tmp);
			pymatemenu_tree_callback_free(callback);
		}

		tmp = next;
	}

	Py_INCREF(Py_None);

	return Py_None;
}

static void pymatemenu_tree_dealloc(PyMateMenuTree* self)
{
	g_slist_foreach(self->callbacks, (GFunc) pymatemenu_tree_callback_free, NULL);
	g_slist_free(self->callbacks);
	self->callbacks = NULL;

	if (self->tree != NULL)
	{
		matemenu_tree_unref(self->tree);
	}

	self->tree = NULL;

	PyObject_DEL(self);
}

static PyObject* pymatemenu_tree_getattro(PyMateMenuTree* self, PyObject* py_attr)
{
	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "__members__"))
		{
			return Py_BuildValue("[sss]", "root", "menu_file", "sort_key");
		}
		else if (!strcmp(attr, "root"))
		{
			return pymatemenu_tree_get_root_directory((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "menu_file"))
		{
			return pymatemenu_tree_get_menu_file((PyObject*) self, NULL);
		}
		else if (!strcmp(attr, "sort_key"))
		{
			return pymatemenu_tree_get_sort_key((PyObject*) self, NULL);
		}
	}

	return PyObject_GenericGetAttr((PyObject*) self, py_attr);
}

static int pymatemenu_tree_setattro(PyMateMenuTree* self, PyObject* py_attr, PyObject* py_value)
{
	PyMateMenuTree* tree;

	tree = (PyMateMenuTree*) self;

	if (PyString_Check(py_attr))
	{
		char* attr;

		attr = PyString_AsString(py_attr);

		if (!strcmp(attr, "sort_key"))
		{
			if (PyInt_Check(py_value))
			{
				int sort_key;

				sort_key = PyInt_AsLong(py_value);

				if (sort_key < MATEMENU_TREE_SORT_FIRST || sort_key > MATEMENU_TREE_SORT_LAST)
				{
					return -1;
				}

				matemenu_tree_set_sort_key(tree->tree, sort_key);

				return 0;
			}
		}
	}

	return -1;
}

static struct PyMethodDef pymatemenu_tree_methods[] = {
	{"get_menu_file", pymatemenu_tree_get_menu_file, METH_VARARGS},
	{"get_root_directory", pymatemenu_tree_get_root_directory, METH_VARARGS},
	{"get_directory_from_path", pymatemenu_tree_get_directory_from_path, METH_VARARGS},
	{"get_sort_key", pymatemenu_tree_get_sort_key, METH_VARARGS},
	{"set_sort_key", pymatemenu_tree_set_sort_key, METH_VARARGS},
	{"add_monitor", pymatemenu_tree_add_monitor, METH_VARARGS},
	{"remove_monitor", pymatemenu_tree_remove_monitor, METH_VARARGS},
	{NULL, NULL, 0}
};

static PyTypeObject PyMateMenuTree_Type = {
	PyObject_HEAD_INIT(NULL)
	0,                                    /* ob_size */
	"matemenu.Tree",                      /* tp_name */
	sizeof(PyMateMenuTree),               /* tp_basicsize */
	0,                                    /* tp_itemsize */
	(destructor) pymatemenu_tree_dealloc, /* tp_dealloc */
	(printfunc) 0,                        /* tp_print */
	(getattrfunc) 0,                      /* tp_getattr */
	(setattrfunc) 0,                      /* tp_setattr */
	(cmpfunc) 0,                          /* tp_compare */
	(reprfunc) 0,                         /* tp_repr */
	0,                                    /* tp_as_number */
	0,                                    /* tp_as_sequence */
	0,                                    /* tp_as_mapping */
	(hashfunc) 0,                         /* tp_hash */
	(ternaryfunc) 0,                      /* tp_call */
	(reprfunc) 0,                         /* tp_str */
	(getattrofunc) pymatemenu_tree_getattro, /* tp_getattro */
	(setattrofunc) pymatemenu_tree_setattro, /* tp_setattro */
	0,                                    /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                   /* tp_flags */
	NULL,                                 /* Documentation string */
	(traverseproc) 0,                     /* tp_traverse */
	(inquiry) 0,                          /* tp_clear */
	(richcmpfunc) 0,                      /* tp_richcompare */
	0,                                    /* tp_weaklistoffset */
	(getiterfunc) 0,                      /* tp_iter */
	(iternextfunc) 0,                     /* tp_iternext */
	pymatemenu_tree_methods,              /* tp_methods */
	0,                                    /* tp_members */
	0,                                    /* tp_getset */
	(PyTypeObject*) 0,                    /* tp_base */
	(PyObject*) 0,                        /* tp_dict */
	0,                                    /* tp_descr_get */
	0,                                    /* tp_descr_set */
	0,                                    /* tp_dictoffset */
	(initproc) 0,                         /* tp_init */
	0,                                    /* tp_alloc */
	0,                                    /* tp_new */
	0,                                    /* tp_free */
	(inquiry) 0,                          /* tp_is_gc */
	(PyObject*) 0,                        /* tp_bases */
};

static PyMateMenuTree* pymatemenu_tree_wrap(MateMenuTree* tree)
{
	PyMateMenuTree* retval;

	if ((retval = matemenu_tree_get_user_data(tree)) != NULL)
	{
		Py_INCREF(retval);
		return retval;
	}

	if (!(retval = (PyMateMenuTree*) PyObject_NEW(PyMateMenuTree, &PyMateMenuTree_Type)))
	{
		return NULL;
	}

	retval->tree = matemenu_tree_ref(tree);
	retval->callbacks = NULL;

	matemenu_tree_set_user_data(tree, retval, NULL);

	return retval;
}

static PyObject* pymatemenu_lookup_tree(PyObject* self, PyObject* args)
{
	char* menu_file;

	MateMenuTree* tree;
	PyMateMenuTree* retval;
	int flags;

	flags = MATEMENU_TREE_FLAGS_NONE;

	if (!PyArg_ParseTuple(args, "s|i:matemenu.lookup_tree", &menu_file, &flags))
	{
		return NULL;
	}

	if (!(tree = matemenu_tree_lookup(menu_file, flags)))
	{
		Py_INCREF(Py_None);
		return Py_None;
	}

	retval = pymatemenu_tree_wrap(tree);

	matemenu_tree_unref(tree);

	return (PyObject*) retval;
}

static struct PyMethodDef pymatemenu_methods[] = {
	{"lookup_tree", pymatemenu_lookup_tree, METH_VARARGS},
	{NULL, NULL, 0 }
};

void initmatemenu(void);

DL_EXPORT(void) initmatemenu(void)
{
	PyObject* mod;

	mod = Py_InitModule4("matemenu", pymatemenu_methods, 0, 0, PYTHON_API_VERSION);

	#define REGISTER_TYPE(t, n) G_STMT_START \
	{ \
		t.ob_type = &PyType_Type; \
		PyType_Ready(&t); \
		PyModule_AddObject(mod, n, (PyObject*) &t); \
	} G_STMT_END

	REGISTER_TYPE(PyMateMenuTree_Type,     "Tree");
	REGISTER_TYPE(PyMateMenuTreeItem_Type, "Item");

	#define REGISTER_ITEM_TYPE(t, n) G_STMT_START \
	{ \
		t.ob_type = &PyType_Type; \
		t.tp_base = &PyMateMenuTreeItem_Type; \
		PyType_Ready(&t); \
		PyModule_AddObject(mod, n, (PyObject*) &t); \
	} G_STMT_END

	REGISTER_ITEM_TYPE(PyMateMenuTreeDirectory_Type, "Directory");
	REGISTER_ITEM_TYPE(PyMateMenuTreeEntry_Type,     "Entry");
	REGISTER_ITEM_TYPE(PyMateMenuTreeSeparator_Type, "Separator");
	REGISTER_ITEM_TYPE(PyMateMenuTreeHeader_Type,    "Header");
	REGISTER_ITEM_TYPE(PyMateMenuTreeAlias_Type,     "Alias");

	PyModule_AddIntConstant(mod, "TYPE_INVALID",   MATEMENU_TREE_ITEM_INVALID);
	PyModule_AddIntConstant(mod, "TYPE_DIRECTORY", MATEMENU_TREE_ITEM_DIRECTORY);
	PyModule_AddIntConstant(mod, "TYPE_ENTRY",     MATEMENU_TREE_ITEM_ENTRY);
	PyModule_AddIntConstant(mod, "TYPE_SEPARATOR", MATEMENU_TREE_ITEM_SEPARATOR);
	PyModule_AddIntConstant(mod, "TYPE_HEADER",    MATEMENU_TREE_ITEM_HEADER);
	PyModule_AddIntConstant(mod, "TYPE_ALIAS",     MATEMENU_TREE_ITEM_ALIAS);

	PyModule_AddIntConstant(mod, "FLAGS_NONE",                MATEMENU_TREE_FLAGS_NONE);
	PyModule_AddIntConstant(mod, "FLAGS_INCLUDE_EXCLUDED",    MATEMENU_TREE_FLAGS_INCLUDE_EXCLUDED);
	PyModule_AddIntConstant(mod, "FLAGS_SHOW_EMPTY",          MATEMENU_TREE_FLAGS_SHOW_EMPTY);
	PyModule_AddIntConstant(mod, "FLAGS_INCLUDE_NODISPLAY",   MATEMENU_TREE_FLAGS_INCLUDE_NODISPLAY);
	PyModule_AddIntConstant(mod, "FLAGS_SHOW_ALL_SEPARATORS", MATEMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS);

	PyModule_AddIntConstant(mod, "SORT_NAME",         MATEMENU_TREE_SORT_NAME);
	PyModule_AddIntConstant(mod, "SORT_DISPLAY_NAME", MATEMENU_TREE_SORT_DISPLAY_NAME);
}
