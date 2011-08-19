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



#include "debug.hpp"

#include "sharp/xmlreader.hpp"

namespace sharp {

  std::string xmlchar_to_string(const xmlChar * s)
  {
    return (s ? (const char*)s : "");
  }

  std::string xmlchar_to_string(xmlChar * s, bool freemem)
  {
    if(s == NULL) {
      return "";
    }
    std::string value = (const char*)s;
    if(freemem) {
      xmlFree(s);
    }
    return value;
  }

  XmlReader::XmlReader()
    : m_reader(NULL)
    , m_error(false)
  {
    m_error = true;
  }

  XmlReader::XmlReader(const std::string & filename)
    : m_reader(NULL)
    , m_error(false)
  {
    m_reader = xmlNewTextReaderFilename(filename.c_str());
    m_error = (m_reader == NULL);
    if(m_reader) {
      setup_error_handling();
    }
  }


  XmlReader::~XmlReader()
  {
    close();
  }


  /** load the buffer from the s
   *  The parser is reset.
   */
  void XmlReader::load_buffer(const std::string &s)
  {
    close();
    /** we copy the string. It shouldn't be a big deal as the strings
     * are copy on write.
     */
    m_buffer = s;
    m_reader = xmlReaderForMemory(m_buffer.c_str(), m_buffer.size(), "",
                                  "UTF-8", 0);//XML_PARSE_RECOVER);
    m_error = (m_reader == NULL);
    if(m_reader) {
      setup_error_handling();
    }
  }
  

  /** read the next node */
  bool XmlReader::read()
  {
    if(m_error) {
      return false;
    }
    int res = xmlTextReaderRead(m_reader);
    return (res > 0);
  }

  xmlReaderTypes XmlReader::get_node_type()
  {
    int type = xmlTextReaderNodeType(m_reader);
    if(type == -1) {
      m_error = true;
    }
    return (xmlReaderTypes)type;
  }
  
  std::string XmlReader::get_name()
  {
    return xmlchar_to_string(xmlTextReaderConstName(m_reader));
  }

  std::string XmlReader::get_attribute(const char * name)
  {
    return xmlchar_to_string(xmlTextReaderGetAttribute(m_reader, (const xmlChar*)name), true);
  }

  std::string XmlReader::get_value()
  {
    return xmlchar_to_string(xmlTextReaderConstValue(m_reader));
  }

  std::string XmlReader::read_string()
  {
    return xmlchar_to_string(xmlTextReaderReadString(m_reader), true);
  }

  std::string XmlReader::read_inner_xml()
  {
    return xmlchar_to_string(xmlTextReaderReadInnerXml(m_reader), true);
  }

  std::string XmlReader::read_outer_xml()
  {
    return xmlchar_to_string(xmlTextReaderReadOuterXml(m_reader), true);
  }

  bool XmlReader::move_to_next_attribute()
  {
    if(m_error) {
      return false;
    }
    int res = xmlTextReaderMoveToNextAttribute(m_reader);
    return (res > 0);
  }

  bool XmlReader::read_attribute_value()
  {
    if(m_error) {
      return false;
    }
    int res = xmlTextReaderReadAttributeValue(m_reader);
    return (res > 0);
  }


  void XmlReader::close()
  {
    if(m_reader) {
      xmlFreeTextReader(m_reader);
      m_reader = NULL;
    }
    m_error = true;
  }


  void XmlReader::setup_error_handling()
  {
    xmlTextReaderErrorFunc func = NULL;
    void* arg = NULL; 

    // We respect any other error handlers already setup:
    xmlTextReaderGetErrorHandler(m_reader, &func, &arg);
    if(!func)
    {
      func = (xmlTextReaderErrorFunc)&XmlReader::error_handler;
      xmlTextReaderSetErrorHandler(m_reader, func, this);
    }
  }


  void XmlReader::error_handler(void* arg, const char* msg, 
                                int /*severity*/, void* /*locator*/)
  {
    XmlReader* self = (XmlReader*)arg;
    self->m_error = true;
    ERR_OUT("XML error %s", msg ? msg : "unknown parse error");
  }

}
