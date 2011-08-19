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



#ifndef __SHARP_MODULEMANAGER_HPP_
#define __SHARP_MODULEMANAGER_HPP_

#include <list>
#include <set>
#include <string>

namespace sharp {

class DynamicModule;

typedef std::list<DynamicModule *> ModuleList;

class ModuleManager 
{
public:
  ~ModuleManager();

  /** add path to list the modules */
  void add_path(const std::string & dir);
  void load_modules();
  
  const ModuleList & get_modules() const
    { return m_modules; }
  const DynamicModule * get_module(const std::string & id) const;
private:
  std::set<std::string> m_dirs;

  ModuleList m_modules;
};


}

#endif

