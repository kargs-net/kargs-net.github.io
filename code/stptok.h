/*********************************************************************
*
* Copyright (C) 1997-2002 Steve Karg
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Functional
* Description:  This file contains the function prototypes for the module.
*
*********************************************************************/
#ifndef STPTOK_H
#define STPTOK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

char *stptok(const char *s, char *tok, size_t toklen, const char *brk);

#ifdef TEST
  #include "ctest.h"
  void testTokens(Test* pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STPTOK_H */
