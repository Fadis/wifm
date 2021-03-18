/*
 * Copyright (C) 2020 Naomasa Matsubayashi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stamp/mapped_file.hpp>

namespace stamp {
  mapped_file::mapped_file( const std::string &name ) :
    fd( open( name.c_str(), O_RDONLY ) ),
    head( nullptr ),
    length( 0 ) {
    if( fd < 0 ) throw unable_to_open_file();
    struct stat buf;
    if( fstat( fd, &buf ) < 0 ) {
      close( fd );
      throw unable_to_open_file();
    }
    if( buf.st_size != 0 ) {
      void * const mapped = mmap( nullptr, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );
      if( mapped == nullptr ) {
        close( fd );
        throw unable_to_open_file();
      }
      head = reinterpret_cast< unsigned char* >( mapped );
      length = buf.st_size;
    }
  }
  mapped_file::~mapped_file() {
    if( head ) munmap( head, length );
    if( fd ) close( fd );
  }
  mapped_file::mapped_file( mapped_file &&src ) : fd( src.fd ), head( src.head ), length( src.length ) {
    src.fd = 0;
    src.head = nullptr;
    src.length = 0;
  }
  mapped_file &mapped_file::operator=( mapped_file &&src ) {
    fd = src.fd;
    head = src.head;
    length = src.length;
    src.fd = 0;
    src.head = nullptr;
    src.length = 0;
    return *this;
  }
}

