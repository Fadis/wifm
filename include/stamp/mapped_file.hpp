#ifndef LIBSTAMP_MAPPED_FILE_H
#define LIBSTAMP_MAPPED_FILE_H
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
#include <cstddef>
#include <iterator>
namespace stamp {
  struct unable_to_open_file {};
  class mapped_file {
  public:
    using value_type = unsigned char;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using iterator = const_pointer;
    using const_iterator = const_pointer;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    mapped_file( const std::string &name );
    ~mapped_file();
    mapped_file( const mapped_file& ) = delete;
    mapped_file( mapped_file &&src );
    mapped_file &operator=( const mapped_file& ) = delete;
    mapped_file &operator=( mapped_file &&src );
    iterator begin() const {
      return head;
    }
    iterator end() const {
      return std::next( head, length );
    }
    iterator cbegin() const {
      return head;
    }
    iterator cend() const {
      return std::next( head, length );
    }
    size_type size() const {
      return length;
    }
    bool empty() const {
      return length == 0u;
    }
  private:
    int fd;
    pointer head;
    size_type length;
  };
}

#endif

