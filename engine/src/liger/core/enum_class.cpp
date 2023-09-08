/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file enum_class.cpp
 * @date 2023-09-07
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 Nikita Mochalov
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "liger/core/enum_class.hpp"

using namespace liger;

int32 detail::TokenizeEnumString(char* base, int32 length, cstring tokens[], int32 size) {
  int32 count = 0;
  tokens[count++] = base + 1;
  for (int32 i = 2; i < length; ++i) {
    if (base[i] == ',') {
      base[i] = '\0';

      if (count == size) {
        return 0;
      }

      i += 3;  // Skip ',', whitespace and 'k'

      tokens[count++] = base + i;
    }
  }

  return 0;
}