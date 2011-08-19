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



#include "sharp/dynamicmodule.hpp"
#include "sharp/map.hpp"
#include "sharp/modulefactory.hpp"


namespace sharp {

  DynamicModule::DynamicModule()
    : m_enabled(true)
  {
  }


  DynamicModule::~DynamicModule()
  {
    sharp::map_delete_all_second(m_interfaces);
  }

  const char * DynamicModule::copyright() const
  {
    return "";
  }

  
  void DynamicModule::enabled(bool enable)
  {
    m_enabled = enable;
  }

  IfaceFactoryBase * DynamicModule::query_interface(const char * intf) const
  {
    std::map<std::string, IfaceFactoryBase *>::const_iterator iter;
    iter = m_interfaces.find(intf);
    if(iter == m_interfaces.end()) {
      return NULL;
    }

    return iter->second;
  }

  bool DynamicModule::has_interface(const char * intf) const
  {
    std::map<std::string, IfaceFactoryBase *>::const_iterator iter;
    iter = m_interfaces.find(intf);
    return (iter != m_interfaces.end());
  }


  void DynamicModule::add(const char * iface, IfaceFactoryBase* mod)
  {
    std::map<std::string, IfaceFactoryBase *>::iterator iter;
    iter = m_interfaces.find(iface);
    if(iter == m_interfaces.end()) {
      m_interfaces.insert(std::make_pair(iface, mod));
    }
    else {
      // replace
      delete iter->second;
      iter->second = mod;
    }
  }

  
}
