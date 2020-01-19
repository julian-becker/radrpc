/*
 * MIT License

 * Copyright (c) 2020 reapler

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
   all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 */

#ifndef RADRPC_COMMON_PACKING_HPP
#define RADRPC_COMMON_PACKING_HPP

namespace radrpc {
namespace common {

/**
 * @brief Macro for structure packing.
 * This has been tested on several compilers on https://godbolt.org/.
 * Godbolt can also be used to determine the ideal default alignment.
 * @example
 *
 * PACK(struct my_struct
 * {
 *      uint64_t a;
 * });
 *
 * for typedefs:
 * PACK(typedef struct 
 * { 
 *      int x;
 * }) my_struct;
 */
#if defined(_MSC_VER) || defined(__INTEL_COMPILER) || defined(__BORLANDC__)
#define PACK(__Declaration__)                                                  \
    __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#else
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

} // namespace common
} // namespace radrpc

#endif // RADRPC_COMMON_PACKING_HPP