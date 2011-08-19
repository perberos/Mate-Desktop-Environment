#
# Copyright (C) 2005 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import os
import errno
import pwd
import gobject
import menutreemodel

DTD_DECLARATION = '<!DOCTYPE Menu PUBLIC "-//freedesktop//DTD Menu 1.0//EN"\n' \
                  ' "http://www.freedesktop.org/standards/menu-spec/menu-1.0.dtd">\n\n'

class MenuFileWriterError (Exception):
    pass

def get_home_dir ():
    try:
        pw = pwd.getpwuid (os.getuid ())
        if pw.pw_dir:
            return pw.pw_dir
    except KeyError:
        pass

    if os.environ.has_key ("HOME"):
        return os.environ["HOME"]
    else:
        raise MenuFileWriterError (_("Cannot find home directory: not set in /etc/passwd and no value for $HOME in environment"))

def get_user_menu_file_path (menu_file):
    config_dir = None
    if os.environ.has_key ("XDG_CONFIG_HOME"):
        config_dir = os.environ["XDG_CONFIG_HOME"]
    if not config_dir:
        config_dir = os.path.join (get_home_dir (), ".config")
    
    return os.path.join (config_dir, "menus", menu_file)

def write_file (filename, contents):
    dir = os.path.dirname (filename)
    try:
        os.makedirs (dir)
    except os.error, (err, str):
        if err != errno.EEXIST:
            raise
    
    temp = filename + ".new"
    try:
        f = file (temp, "w")
    except:
        temp = None
        f = file (filename, "w")
    
    try:
        f.write (contents)
        f.close ()
    except:
        if temp != None:
            os.remove (temp)
        raise

    if temp != None:
        os.rename (temp, filename)

class MenuFileWriter:
    def __init__ (self, menu_tree_model):
        self.model = menu_tree_model

        self.sync_idle_handlers = {}

    def __del__ (self):
        for (id, iter) in self.sync_idle_handlers:
            gobject.source_remove (id)

    def __append_menu (self, contents, indent, iter, system_menu_file = None):
        has_changes = False
        orig_contents = contents
        
        contents += indent + "<Menu>\n"
        contents += indent + "  <Name>%s</Name>\n" % self.model[iter][self.model.COLUMN_ID]

        if system_menu_file:
            contents += indent + '  <MergeFile type="parent">%s</MergeFile>\n' % system_menu_file

        includes = []
        excludes = []

        child_iter = self.model.iter_children (iter)
        while child_iter:
            if self.model[child_iter][self.model.COLUMN_IS_ENTRY]:
                desktop_file_id = self.model[child_iter][self.model.COLUMN_ID]
                system_visible  = self.model[child_iter][self.model.COLUMN_SYSTEM_VISIBLE]
                user_visible    = self.model[child_iter][self.model.COLUMN_USER_VISIBLE]

                if not system_visible and user_visible:
                    includes.append (desktop_file_id)
                elif system_visible and not user_visible:
                    excludes.append (desktop_file_id)

            child_iter = self.model.iter_next (child_iter)

        if len (includes) > 0:
            contents += indent + "  <Include>\n"
            for desktop_file_id in includes:
                contents += indent + "    <Filename>%s</Filename>\n" % desktop_file_id
            contents += indent + "  </Include>\n"
            has_changes = True
        
        if len (excludes) > 0:
            contents += indent + "  <Exclude>\n"
            for desktop_file_id in excludes:
                contents += indent + "    <Filename>%s</Filename>\n" % desktop_file_id
            contents += indent + "  </Exclude>\n"
            has_changes = True
        
        child_iter = self.model.iter_children (iter)
        while child_iter:
            if not self.model[child_iter][self.model.COLUMN_IS_ENTRY]:
                (contents, subdir_has_changes) = self.__append_menu (contents,
                                                                     indent + "  ",
                                                                     child_iter)
                if not has_changes:
                    has_changes = subdir_has_changes

            child_iter = self.model.iter_next (child_iter)

        if has_changes:
            return (contents + indent + "</Menu>\n", True)
        else:
            return (orig_contents, False)

    def sync (self, iter):
        menu_file = self.model[iter][self.model.COLUMN_MENU_FILE]
        system_menu_file = menutreemodel.lookup_system_menu_file (menu_file)
        
        (contents, has_changes) = self.__append_menu (DTD_DECLARATION,
                                                      "",
                                                      iter,
                                                      system_menu_file)

        if not has_changes:
            try:
                os.remove (get_user_menu_file_path (menu_file))
            except:
                pass
            return
            
        write_file (get_user_menu_file_path (menu_file), contents)

    def __sync_idle_handler_func (self, iter):
        self.sync (iter)
        del self.sync_idle_handlers[iter]
        return False

    def queue_sync (self, iter):
        def find_menu_file_parent (model, iter):
            if model[iter][model.COLUMN_MENU_FILE]:
                return iter
            
            parent_iter = model.iter_parent (iter)
            if not parent_iter:
                return None

            return find_menu_file_parent (model, parent_iter)

        menu_file_iter = find_menu_file_parent (self.model, iter)
        if not menu_file_iter:
            return

        menu_file_path = self.model.get_string_from_iter (menu_file_iter)
        for iter in self.sync_idle_handlers:
            path = self.model.get_string_from_iter (iter)
            if path == menu_file_path:
                return

        id = gobject.idle_add (self.__sync_idle_handler_func, menu_file_iter)

        self.sync_idle_handlers[menu_file_iter] = id
