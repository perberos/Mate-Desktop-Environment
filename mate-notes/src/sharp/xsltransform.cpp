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

#include <stdlib.h>

#include <libxslt/xsltutils.h>

#include "sharp/exception.hpp"
#include "sharp/xmlwriter.hpp"
#include "sharp/xsltransform.hpp"
#include "debug.hpp"

namespace sharp {

  bool XslTransformLink_ = true;;


XslTransform:: XslTransform()
  : m_stylesheet(NULL)
{
}


XslTransform::~XslTransform()
{
  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
}


void XslTransform::load(const std::string & sheet)
{
  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
  m_stylesheet = xsltParseStylesheetFile((const xmlChar *)sheet.c_str());
  DBG_ASSERT(m_stylesheet, "stylesheet failed to load");
}


void XslTransform::transform(xmlDocPtr doc, const XsltArgumentList & args, StreamWriter & output, const XmlResolver & /*resolved*/)
{
  const char **params = NULL;
  if(m_stylesheet == NULL) {
    ERR_OUT("NULL stylesheet");
    return;
  }

  xmlDocPtr res;

  params = args.get_xlst_params();

  res = xsltApplyStylesheet(m_stylesheet, doc, params);
  free(params);
  if(res) {
    xmlOutputBufferPtr output_buf 
      = xmlOutputBufferCreateFile(output.file(), 
                                  xmlGetCharEncodingHandler(XML_CHAR_ENCODING_UTF8));
    xsltSaveResultTo(output_buf, res, m_stylesheet);
    xmlOutputBufferClose(output_buf);
  
    xmlFreeDoc(res);
  }
  else {
    // FIXME get the real error message
    throw(sharp::Exception("XSLT Error"));
  }
}

}
