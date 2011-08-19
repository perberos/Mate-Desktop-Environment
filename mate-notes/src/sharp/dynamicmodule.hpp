/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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




#ifndef __SHARP_DYNAMICMODULE_HPP_
#define __SHARP_DYNAMICMODULE_HPP_

#include <map>
#include <string>


namespace sharp {

class IfaceFactoryBase;
class DynamicModule;

typedef DynamicModule* (*instanciate_func_t)();

#define DECLARE_MODULE(klass) \
  extern "C" sharp::DynamicModule* dynamic_module_instanciate() \
  { return new klass; }


#define ADD_INTERFACE_IMPL(klass) \
    add(klass::IFACE_NAME, \
        new sharp::IfaceFactory<klass>)


class DynamicModule
{
public:

  virtual ~DynamicModule();

  virtual const char * id() const = 0;
  virtual const char * name() const = 0;
  virtual const char * description() const = 0;
  virtual const char * authors() const = 0;
  virtual int          category() const = 0;
  virtual const char * version() const = 0;
  virtual const char * copyright() const;
  bool is_enabled() const
    {
      return m_enabled;
    }

  void enabled(bool enable=true);

  /** Query an "interface" 
   * may return NULL
   */
  IfaceFactoryBase * query_interface(const char *) const;
  /** Check if the module provide and interface */
  bool has_interface(const char *) const;

  void load();

protected:
  DynamicModule();

  /** */
  void add(const char * iface, IfaceFactoryBase*);
  
private:
  bool m_enabled;
  std::map<std::string, IfaceFactoryBase *> m_interfaces;
};



}

#endif
