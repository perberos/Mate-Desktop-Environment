/*
 * matecorba-imodule.c:
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *                    Ximian, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors:
 *     Mark McLoughlin <mark@skynet.ie>
 *     Michael Meeks <michael@ximian.com>
 */

#include <string.h>

#include "matecorba-imodule.h"
#include "matecorba-imodule-utils.h"

typedef struct {
	IDL_tree        tree;
	CORBA_TypeCode  tc;
	GSList         *methods; /* IDLN_OP_DCLs */
} Interface;

typedef struct {
	IDL_tree  cur_node;
	guint     parents;
} InterfaceCountInfo;

typedef struct {
	IDL_tree                     cur_node;
	CORBA_sequence_CORBA_string *base_interfaces;
	guint                        index;
} InterfaceTraverseInfo;

static void
MateCORBA_iinterface_count_base_itypes (IDL_tree            node,
				    InterfaceCountInfo *iti)
{
	if (iti->cur_node == node)
		return;

	iti->parents++;
}

static void
MateCORBA_iinterface_fill_base_itypes (IDL_tree               node,
				   InterfaceTraverseInfo *iti)
{
	if (iti->cur_node == node)
		return;

	iti->base_interfaces->_buffer [iti->index++] =
		CORBA_string_dup (IDL_IDENT(IDL_INTERFACE(node).ident).repo_id);
}

static void
MateCORBA_iinterface_fill_iargs (GHashTable  *typecodes,
			     IDL_tree     tree,
			     MateCORBA_IArgs *ret_iargs)
{
	IDL_tree sub;
	int      arg_count;
	int      i;

	g_return_if_fail (tree != NULL);
	g_return_if_fail (ret_iargs != NULL);

	arg_count = IDL_list_length (IDL_OP_DCL (tree).parameter_dcls);

	ret_iargs->_length  = arg_count;
	ret_iargs->_maximum = arg_count;
	ret_iargs->_buffer  = MateCORBA_IArgs_allocbuf (arg_count);
	ret_iargs->_release = CORBA_TRUE;

	for (sub = IDL_OP_DCL (tree).parameter_dcls, i = 0; sub;
	     sub = IDL_LIST (sub).next, i++) {
		MateCORBA_IArg *iarg;
		IDL_tree    parm;

		iarg = &ret_iargs->_buffer [i];

		parm = IDL_LIST (sub).data;

		iarg->tc = MateCORBA_imodule_get_typecode (
				typecodes, 
				IDL_PARAM_DCL (parm).param_type_spec);

		iarg->name = CORBA_string_dup(
				 IDL_STRING (IDL_PARAM_DCL(parm).simple_declarator).value);

		switch (IDL_PARAM_DCL (parm).attr) {
		case IDL_PARAM_IN:
			iarg->flags = MateCORBA_I_ARG_IN;
			break;
		case IDL_PARAM_OUT:
			iarg->flags = MateCORBA_I_ARG_OUT;
			break;
		case IDL_PARAM_INOUT:
			iarg->flags = MateCORBA_I_ARG_INOUT;
			break;
		default:
			g_assert_not_reached ();
		}

		if (MateCORBA_imodule_type_is_fixed_length (
				IDL_PARAM_DCL (parm).param_type_spec))
			iarg->flags |= MateCORBA_I_COMMON_FIXED_SIZE;

#if 0
		else if (IDL_PARAM_DCL (parm).attr == IDL_PARAM_OUT) {
			IDL_tree ts = MateCORBA_imodule_get_typespec (
				IDL_PARAM_DCL (parm).param_type_spec);

			switch (IDL_NODE_TYPE (ts)) {
			case IDLN_TYPE_STRUCT:
			case IDLN_TYPE_UNION:
			case IDLN_TYPE_ARRAY:
				iarg->flags |= MATECORBA_I_ARG_FIXED;
				break;
			default:
				break;
			};
		}
#endif
	}
}

static void
MateCORBA_iinterface_fill_contexts (GHashTable      *typecodes,
				IDL_tree         tree,
				MateCORBA_IContexts *ret_contexts)
{
	g_return_if_fail (tree != NULL);
	g_return_if_fail (ret_contexts != NULL);

	memset (ret_contexts, 0, sizeof (MateCORBA_IContexts));

	if (IDL_OP_DCL (tree).context_expr) {
		IDL_tree curitem;
		int      count;
		int      i;

		count = IDL_list_length (IDL_OP_DCL (tree).context_expr);

		ret_contexts->_length  = count;
		ret_contexts->_maximum = count;
		ret_contexts->_buffer  = MateCORBA_IContexts_allocbuf (count);
		ret_contexts->_release = CORBA_TRUE;

		for (curitem = IDL_OP_DCL (tree).context_expr, i = 0; curitem;
		     curitem = IDL_LIST (curitem).next, i++)
			ret_contexts->_buffer [i] = CORBA_string_dup (
					IDL_STRING (IDL_LIST (curitem).data).value);
	}
}

static void
MateCORBA_iinterface_fill_exceptinfo (GHashTable   *typecodes,
				  IDL_tree      tree,
				  MateCORBA_ITypes *ret_itypes)
{
	g_return_if_fail (tree != NULL);
	g_return_if_fail (ret_itypes != NULL);

	memset (ret_itypes, 0, sizeof (MateCORBA_ITypes));

	if (IDL_OP_DCL (tree).raises_expr) {
		IDL_tree curitem;
		int      count;
		int      i;

		count = IDL_list_length (IDL_OP_DCL (tree).raises_expr);

		ret_itypes->_length  = count;
		ret_itypes->_maximum = count;
		ret_itypes->_buffer  = MateCORBA_ITypes_allocbuf (count);
		ret_itypes->_release = CORBA_TRUE;
		
		for (curitem = IDL_OP_DCL (tree).raises_expr, i = 0; curitem;
		     curitem = IDL_LIST (curitem).next, i++)
			ret_itypes->_buffer [i] =
				MateCORBA_imodule_get_typecode (
					typecodes, IDL_LIST (curitem).data);
	}
}

static void
MateCORBA_iinterface_fill_method (GHashTable    *typecodes,
			      IDL_tree       tree,
			      MateCORBA_IMethod *ret_imethod)
{
	char *method;

	g_return_if_fail (tree != NULL);
	g_return_if_fail (IDL_NODE_TYPE (tree) == IDLN_OP_DCL);
	g_return_if_fail (ret_imethod != NULL);

	MateCORBA_iinterface_fill_iargs (
			typecodes, tree, &ret_imethod->arguments);
	MateCORBA_iinterface_fill_contexts (
			typecodes, tree, &ret_imethod->contexts);
	MateCORBA_iinterface_fill_exceptinfo (
			typecodes, tree, &ret_imethod->exceptions);

	if (IDL_OP_DCL (tree).op_type_spec)
		ret_imethod->ret = MateCORBA_imodule_get_typecode (
					typecodes,
					IDL_OP_DCL (tree).op_type_spec);
	else
		ret_imethod->ret = TC_void;

	method = IDL_IDENT (IDL_OP_DCL (tree).ident).str;
	ret_imethod->name     = CORBA_string_dup (method);
	ret_imethod->name_len = strlen (method);

	ret_imethod->flags = 0;

	if (IDL_OP_DCL(tree).f_oneway)
		ret_imethod->flags |= MateCORBA_I_METHOD_1_WAY;

#if 0 /* FIXME: re-scan for no_out */
	if (no_out)
		ret_imethod->flags |= MateCORBA_I_METHOD_NO_OUT;
#endif

	if (IDL_OP_DCL (tree).op_type_spec &&
	    MateCORBA_imodule_type_is_fixed_length (
		    IDL_OP_DCL (tree).op_type_spec))
		ret_imethod->flags |= MateCORBA_I_COMMON_FIXED_SIZE;

	if (IDL_OP_DCL (tree).context_expr)
		ret_imethod->flags |= MateCORBA_I_METHOD_HAS_CONTEXT;
}

static void
MateCORBA_iinterface_from_interface (GHashTable       *typecodes,
				 Interface        *iface,
				 MateCORBA_IInterface *ret_iiface)
{
	InterfaceTraverseInfo  iti;
	InterfaceCountInfo     ici;
	GSList                *m;
	int                    method_count;
	int                    i;

	g_return_if_fail (iface != NULL);
	g_return_if_fail (ret_iiface != NULL);

	ret_iiface->tc = (CORBA_TypeCode) CORBA_Object_duplicate (
						(CORBA_Object) iface->tc, NULL);

	method_count = g_slist_length (iface->methods);

	ret_iiface->methods._length   = method_count;
	ret_iiface->methods._maximum  = method_count;
	ret_iiface->methods._buffer   = MateCORBA_IMethods_allocbuf (method_count);
	ret_iiface->methods._release  = CORBA_TRUE;

	for (m = iface->methods, i = 0; m; m = m->next, i++)
		MateCORBA_iinterface_fill_method (
			typecodes, m->data,
			&ret_iiface->methods._buffer [i]);

	ici.cur_node = iface->tree;
	ici.parents  = 0;
	MateCORBA_imodule_traverse_parents (iface->tree, (GFunc) MateCORBA_iinterface_count_base_itypes, &ici);

	ret_iiface->base_interfaces._length   = ici.parents + 1;
	ret_iiface->base_interfaces._maximum  = ici.parents + 1;
	ret_iiface->base_interfaces._buffer   = CORBA_sequence_CORBA_string_allocbuf (ici.parents + 1);
	ret_iiface->base_interfaces._release  = CORBA_TRUE;

	iti.cur_node        = iface->tree;
	iti.index           = 0;
	iti.base_interfaces = &ret_iiface->base_interfaces;
	MateCORBA_imodule_traverse_parents (iface->tree, (GFunc) MateCORBA_iinterface_fill_base_itypes, &iti);

	iti.base_interfaces->_buffer [iti.index] = CORBA_string_dup ("IDL:CORBA/Object:1.0");
}

static void
MateCORBA_iinterface_free_interfaces (GSList *list)
{
	GSList *l;

	for (l = list; l; l = l->next) {
		CORBA_Object_release ((CORBA_Object) ((Interface *) l->data)->tc, NULL);
		g_slist_free (((Interface *) l->data)->methods);
		g_free (l->data);
	}

	g_slist_free (list);
}

static void
MateCORBA_imodule_fake_attribute_ops (IDL_tree  attr,
				  IDL_tree  ident,
				  IDL_tree *get_op,
				  IDL_tree *set_op)
{
	IDL_tree fake_ident;

	g_return_if_fail (attr != NULL);
	g_return_if_fail (IDL_NODE_TYPE (attr) == IDLN_ATTR_DCL);
	g_return_if_fail (ident != NULL);
	g_return_if_fail (IDL_NODE_TYPE (ident) == IDLN_IDENT);

	fake_ident = IDL_ident_new (
			g_strdup_printf ("_get_%s", IDL_IDENT (ident).str));
	IDL_IDENT_TO_NS (fake_ident) = IDL_IDENT_TO_NS (ident);

	*get_op = IDL_op_dcl_new (
			0, IDL_ATTR_DCL (attr).param_type_spec, fake_ident, NULL, NULL, NULL);
	IDL_NODE_UP (*get_op) = IDL_NODE_UP (attr);

	if (!IDL_ATTR_DCL (attr).f_readonly) {
		IDL_tree param;

		fake_ident = IDL_ident_new (
				g_strdup_printf ("_set_%s", IDL_IDENT (ident).str));
		IDL_IDENT_TO_NS (fake_ident) = IDL_IDENT_TO_NS (ident);

		*set_op = IDL_op_dcl_new (
				0, NULL, fake_ident, NULL, NULL, NULL);
		IDL_NODE_UP (*set_op) = IDL_NODE_UP (attr);

		param = IDL_param_dcl_new (IDL_PARAM_IN,
					   IDL_ATTR_DCL (attr).param_type_spec,
					   IDL_ident_new (g_strdup("value")));
		IDL_OP_DCL (*set_op).parameter_dcls = IDL_list_new (param);
	}
}

static GSList *
MateCORBA_iinterface_build_interfaces (GHashTable *typecodes,
				   GSList     *list,
				   IDL_tree    tree)
{
	if (!tree)
		return list;

	switch (IDL_NODE_TYPE (tree)) {
	case IDLN_MODULE:
		list = MateCORBA_iinterface_build_interfaces (
			typecodes, list, IDL_MODULE (tree).definition_list);
		break;
	case IDLN_LIST: {
		IDL_tree sub;

		for (sub = tree; sub; sub = IDL_LIST (sub).next)
			list = MateCORBA_iinterface_build_interfaces (
				typecodes, list, IDL_LIST (sub).data);
		}
		break;
	case IDLN_ATTR_DCL: {
		IDL_tree sub;

		for (sub = IDL_ATTR_DCL (tree).simple_declarations;
		     sub; sub = IDL_LIST (sub).next) {
			IDL_tree get_op = NULL;
			IDL_tree set_op = NULL;

			MateCORBA_imodule_fake_attribute_ops (
				tree, IDL_LIST (sub).data, &get_op, &set_op);
	
			list = MateCORBA_iinterface_build_interfaces (
						typecodes, list, get_op);
			if (set_op)
				list = MateCORBA_iinterface_build_interfaces (
						typecodes, list, set_op);
		}
		}
		break;
	case IDLN_INTERFACE: {
		Interface *i = g_new0 (Interface, 1);

		i->tree = tree;
		i->tc   = MateCORBA_imodule_get_typecode (typecodes, tree);

		list = g_slist_append (list, i);

		list = MateCORBA_iinterface_build_interfaces (
				typecodes, list, IDL_INTERFACE (tree).body);
		}
		break;
	case IDLN_OP_DCL: {
		Interface *i;

		g_return_val_if_fail (list != NULL, NULL);

		i = (g_slist_last (list))->data;
		i->methods = g_slist_append (i->methods, tree);
		}
		break;
	case IDLN_EXCEPT_DCL:
	case IDLN_TYPE_STRUCT:
	case IDLN_TYPE_UNION:
	case IDLN_TYPE_ENUM:
	case IDLN_FORWARD_DCL:
		/* Load the types into the typecode hash */
		CORBA_Object_release (
			(CORBA_Object) MateCORBA_imodule_get_typecode (
						typecodes, tree), NULL);
		break;
	case IDLN_TYPE_DCL: {
		CORBA_TypeCode type;
		IDL_tree       l;

		type = MateCORBA_imodule_get_typecode (
				typecodes, IDL_TYPE_DCL (tree).type_spec);

		for (l = IDL_TYPE_DCL (tree).dcls;  l; l = IDL_LIST (l).next) {
			IDL_tree dcl;

			dcl = IDL_LIST (l).data;

			if (IDL_NODE_TYPE (dcl) == IDLN_IDENT)
				CORBA_Object_release (
					(CORBA_Object) MateCORBA_imodule_create_alias_typecode (
									typecodes, dcl, type), NULL);

			else /* IDLN_TYPE_ARRAY */
				CORBA_Object_release (
					(CORBA_Object) MateCORBA_imodule_get_typecode (
								typecodes, dcl), NULL);
		}

		CORBA_Object_release ((CORBA_Object) type, NULL);
		}
		break;
	default:
		break;
	}

	return list;
}

/*
 * MateCORBA_iinterfaces_from_file:
 * @tree: an %IDL_tree.
 * @typecodes_ret: return location for typecodes sequence (optional)
 *
 * Traverses the IDL parse tree, @tree,  and returns a sequence of
 * %MateCORBA_IInterface.  The output is equivalent to the static data
 * that is output by the idl compiler using the --add-imodule switch.
 * A sequence of %CORBA_TypeCode is returned through the @typecodes_ret
 * parameter.
 *
 * Both the return value and the typecodes sequence should be
 * freed using CORBA_free().
 *
 * This method is intended for use by scripting language bindings
 * so that they may invoke interface methods defined in the idl
 * tree parsing the idl themselves or loading a typelib
 * module. Use this instead of MateCORBA_iinterfaces_from_file if you
 * also wish to have access to the IDL parse tree.
 * 
 * Return Value: A sequence of %MateCORBA_IInterface.
 */

MateCORBA_IInterfaces *
MateCORBA_iinterfaces_from_tree (IDL_tree                        tree,
			     CORBA_sequence_CORBA_TypeCode **typecodes_ret)
{
	GHashTable        *typecodes;
	MateCORBA_IInterfaces *retval;
	GSList            *list, *l;
	int                count, i;

	g_return_val_if_fail (tree != NULL, NULL);

	typecodes = MateCORBA_imodule_new_typecodes ();

	list = MateCORBA_iinterface_build_interfaces (typecodes, NULL, tree);

	count = g_slist_length (list);

	retval = MateCORBA_IInterfaces__alloc ();

	retval->_length  = count;
	retval->_maximum = count;
	retval->_buffer  = MateCORBA_IInterfaces_allocbuf (count);
	retval->_release = CORBA_TRUE;

	for (l = list, i = 0; l; l = l->next, i++)
		MateCORBA_iinterface_from_interface (
			typecodes, l->data, &retval->_buffer [i]);

	MateCORBA_iinterface_free_interfaces (list);

	if (typecodes_ret)
		*typecodes_ret = MateCORBA_imodule_get_typecodes_seq (typecodes);

	MateCORBA_imodule_free_typecodes (typecodes);

	return retval;
}

static char *
build_cpp_args (const char *path,
		const char *cpp_args)
{
	char *ret;
	char *base;
	char *base_cpy;
	int   i;

	base = g_path_get_basename (path);
	if (strlen (base) <= 4) {
		ret = g_strconcat ("-D__MATECORBA_IDL__ ", cpp_args, NULL);
	} else {
		/* base minus .idl extension */
		base_cpy = g_strndup (base, strlen (base) - 4);
		for (i = 0; base_cpy[i] != '\0'; i++) {
			if (base_cpy[i] == '-')
				base_cpy[i] = '_';
		}
		ret = g_strconcat ("-D__MATECORBA_IDL__ -D__", base_cpy,
				   "_COMPILATION ", cpp_args, NULL);
		g_free (base_cpy);
	}
	g_free (base);
	return ret;
}

#define PARSE_FLAGS (IDLF_SHOW_CPP_ERRORS|	\
		     IDLF_TYPECODES|		\
		     IDLF_SRCFILES|		\
		     IDLF_CODEFRAGS)

/*
 * MateCORBA_iinterfaces_from_file:
 * @path: path to the idl file to parse
 * @cpp_args: arguments to pass to the preprocessor (optional)
 * @typecodes_ret: return location for typecodes sequence (optional)
 *
 * Parses @path idl file and returns a sequence of %MateCORBA_IInterface.
 * The output is equivalent to the static data that is output by
 * the idl compiler using the --add-imodule switch. A sequence
 * of %CORBA_TypeCode is returned through the @typecodes_ret parameter.
 *
 * Both the return value and the typecodes sequence should be
 * freed using CORBA_free().
 *
 * This method is intended for use by scripting language bindings
 * so that they may invoke interface methods defined in the idl
 * file without parsing the idl themselves or loading a typelib
 * module.
 * 
 * Return Value: A sequence of %MateCORBA_IInterface.
 */
MateCORBA_IInterfaces *
MateCORBA_iinterfaces_from_file (const char                     *path,
			     const char                     *cpp_args,
			     CORBA_sequence_CORBA_TypeCode **typecodes_ret)
{
	MateCORBA_IInterfaces *retval;
	char              *full_cpp_args;
	IDL_tree           tree;
	IDL_ns             namespace;
	int                ret;
	
	full_cpp_args = build_cpp_args (path, cpp_args);
	ret = IDL_parse_filename (path, full_cpp_args, NULL, &tree,
				  &namespace, PARSE_FLAGS, 0);
	g_free (full_cpp_args);
	if (ret != IDL_SUCCESS) {
		g_warning ("Cannot parse %s\n", path);
		return NULL;
	}

	retval = MateCORBA_iinterfaces_from_tree (tree, typecodes_ret);

	return retval;
}
