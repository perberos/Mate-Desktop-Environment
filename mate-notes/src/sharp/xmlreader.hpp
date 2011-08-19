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




#ifndef __SHARP_XMLREADER_HPP_
#define __SHARP_XMLREADER_HPP_

#include <string>

#include <boost/noncopyable.hpp>
#include <libxml/xmlreader.h>

namespace sharp {

class XmlReader
    : public boost::noncopyable
{
public:
  /** Create a XmlReader to read from a buffer */
  XmlReader();
  /** Create a XmlReader to read from a file */
  XmlReader(const std::string & filename);
  ~XmlReader();

  /** load the buffer from the s 
   *  The parser is reset.
   */
  void load_buffer(const std::string &s);
  

  /** read the next node 
   *  return false if it couldn't be read. (either end or error)
   */
  bool read();

  xmlReaderTypes get_node_type();
  
  std::string    get_name();
  std::string    get_attribute(const char *);
  std::string    get_value();
  std::string    read_string();
  std::string    read_inner_xml();
  std::string    read_outer_xml();
  bool           move_to_next_attribute();
  bool           read_attribute_value();

  void           close();
private:

  void setup_error_handling();
  static void error_handler(void* arg, const char* msg, int severity, void* locator);

  std::string      m_buffer;
  xmlTextReaderPtr m_reader;
  // error signaling
  bool             m_error;
};


}


#endif
