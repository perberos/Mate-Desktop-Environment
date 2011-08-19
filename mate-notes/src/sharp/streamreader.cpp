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





#include "sharp/streamreader.hpp"

#include "debug.hpp"

namespace sharp {

#define READ_BUFFER_COUNT 1024

StreamReader::StreamReader()
  : m_file(NULL)
{
}

StreamReader::~StreamReader()
{
  if(m_file) {
    fclose(m_file);
  }
}

void StreamReader::init(const std::string & filename)
{
  m_file = fopen(filename.c_str(), "rb");
}


void StreamReader::read_to_end(std::string & text)
{
  DBG_ASSERT(m_file, "file is NULL");

  text.clear();

  char buffer[READ_BUFFER_COUNT + 1]; // +1 for the NUL terminate.
  size_t byte_read = 0;
  do {
    byte_read = fread(buffer, 1, READ_BUFFER_COUNT, m_file);
    buffer[byte_read+1] = 0; // NUL terminate.
    text += buffer;
  }
  while(byte_read == READ_BUFFER_COUNT);
}


void StreamReader::close()
{
  fclose(m_file);
  m_file = NULL;
}


}
