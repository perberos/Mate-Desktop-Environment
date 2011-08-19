/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#include <dlfcn.h>

#include <gmodule.h>
#include <glibmm/module.h>

#include "sharp/directory.hpp"
#include "sharp/dynamicmodule.hpp"
#include "sharp/files.hpp"
#include "sharp/map.hpp"
#include "sharp/modulemanager.hpp"
#include "debug.hpp"

namespace sharp {


  ModuleManager::~ModuleManager()
  {
    for(ModuleList::const_iterator mod_iter = m_modules.begin();
        mod_iter != m_modules.end(); ++mod_iter) {
      delete *mod_iter;
    }
  }


  void ModuleManager::add_path(const std::string & dir)
  {
    m_dirs.insert(dir);
    DBG_OUT("add path %s", dir.c_str());
  }


  void ModuleManager::load_modules()
  {
    std::string ext = std::string(".") + G_MODULE_SUFFIX;

    for(std::set<std::string>::const_iterator iter = m_dirs.begin();
        iter != m_dirs.end(); ++iter) {

      std::list<std::string> l;
      directory_get_files_with_ext(*iter, ext, l);
      
      for(std::list<std::string>::const_iterator mod_iter = l.begin();
          mod_iter != l.end(); ++mod_iter) {

        Glib::Module module(*iter + "/" + file_basename(*mod_iter), 
                            Glib::MODULE_BIND_LOCAL);
        DBG_OUT("load module %s", file_basename(*mod_iter).c_str());

        if(!module) {
          DBG_OUT("error loading %s", Glib::Module::get_last_error().c_str());
          continue;
        }
        void * func = NULL;
        bool found = module.get_symbol("dynamic_module_instanciate", func);

        if(!found) {
          DBG_OUT("error getting symbol %s", Glib::Module::get_last_error().c_str());
          continue;
        }
        instanciate_func_t real_func = (instanciate_func_t)func;
        DynamicModule * dmod = (*real_func)();

        if(dmod) {
          m_modules.push_back(dmod);
          module.make_resident();
        }
      }

    }
  }

const DynamicModule * ModuleManager::get_module(const std::string & id) const
{
  for(ModuleList::const_iterator iter = m_modules.begin();
      iter != m_modules.end(); ++iter) {
    if (id == (*iter)->id())
      return *iter;
  }
  return 0;
}

}
