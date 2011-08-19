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


#include <glibmm/ustring.h>

#include "debug.hpp"
#include "xmlwriter.hpp"

namespace sharp {

  XmlWriter::XmlWriter()
  {
    m_buf = xmlBufferCreate();
    m_writer = xmlNewTextWriterMemory(m_buf, 0);
  }

  XmlWriter::XmlWriter(const std::string & filename)
    : m_buf(NULL)
  {
    m_writer = xmlNewTextWriterFilename(filename.c_str(), 0);
  }

  
  XmlWriter::XmlWriter(xmlDocPtr doc)
    : m_buf(NULL)
  {
    m_writer = xmlNewTextWriterTree(doc, NULL, 0);
  }


  XmlWriter::~XmlWriter()
  {
    xmlFreeTextWriter(m_writer);
    if(m_buf)
      xmlBufferFree(m_buf);
  }


  int XmlWriter::write_char_entity(gunichar ch)
  {
    Glib::ustring unistring(1, (gunichar)ch);
    DBG_OUT("write entity %s", unistring.c_str());
    return xmlTextWriterWriteString(m_writer, (const xmlChar*)unistring.c_str());
  }

  int XmlWriter::write_string(const std::string & s)
  {
    return xmlTextWriterWriteString(m_writer, (const xmlChar*)s.c_str());
  }


  int  XmlWriter::close()
  {
    int rc = xmlTextWriterEndDocument(m_writer);
    xmlTextWriterFlush(m_writer);
    return rc;
  }


  std::string XmlWriter::to_string()
  {
    if(!m_buf) {
      return "";
    }
    std::string output((const char*)m_buf->content);
    return output;
  }

}
