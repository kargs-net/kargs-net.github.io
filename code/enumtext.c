/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to 
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
//
// This file contains the enumeration to text library
//
#include <stdio.h>              /* Standard I/O */
#include <stdlib.h>             /* Standard Library */
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// returns the number of entries in a NULL terminated list of strings
static unsigned string_list_len(const char **string_list)
{
    unsigned len = 0;

    if (string_list) {
        while (string_list[len]) {
            len++;
        }
    }

    return len;
}

// generic function to return the text
static const char *enum_to_text(int enumeration,
    const char *unknown_text,
    unsigned *len, 
    const char **string_list)
{
    const char *text = unknown_text;

    if (*len == 0)
        *len = string_list_len(string_list);
    if (enumeration < *len)
        text = string_list[enumeration];

    return text;
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

enum test_text_t
{
  TEST_TEXT_1,
  TEST_TEXT_2,
  TEST_TEXT_3,
  TEST_TEXT_4,
  TEST_TEXT_5,
};

const char *test_text[] = 
{
  "test-text-1",
  "test-text-2",
  "test-text-3",
  "test-text-4",
  "test-text-5",
  NULL
};

const char *enum_to_test_text(enum test_text_t enumeration)
{
    static unsigned len = 0;    // length of the string list

    return enum_to_text(enumeration, "unknown test text", &len, test_text);
}

void testTextList(Test * pTest)
{
    int i;
    const char *pstring = NULL;
    const char *test_string = NULL;

    for (i = 0; i < 260; i++) {
        pstring = enum_to_test_text(i);
        ct_test(pTest, pstring != NULL);
    }

    // test the boundary points
    pstring = enum_to_test_text(0);
    test_string = test_text[0];
    ct_test(pTest, pstring == test_string);

    // last known
    pstring = enum_to_test_text(4);
    test_string = test_text[4];
    ct_test(pTest, pstring == test_string);

    // last unknowns - should match if we haven't filled all the enums
    pstring = enum_to_test_text(6);
    test_string = enum_to_test_text(6);
    ct_test(pTest, pstring == test_string);

    return;
}


#ifdef TEST_ENUMTEXT
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("enum text", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testTextList);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
#endif                          // TEST_ENUMTEXT
#endif                          // TEST
