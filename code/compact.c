/* idea for code from Circuit Cellar - Test Your EQ */
/*
* Copyright (c) 2001 Stephen Karg
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
*/
/* programmed in C - using some extreme programming techniques
  1. Code the unit test first.
     You create one test to define some small aspect of the
     problem at hand. Then you create the simplest code that
     will make that test pass. Then you create a second test.
     Now you add to the code you just created to make this
     new test pass, but no more! Not until you have yet a third
     test. You continue until there is nothing left to test.
  2. Always do the simplest thing that could possibly work
  3. All code must pass all unit tests before it can be released.
  4. When a bug is found new tests are created.
  5. Acceptance tests are run often and the score is published
  6. Leave optimization till last.
  7. Keep things as simple as possible as long as possible by
     never adding functionality before it is scheduled.
*/

#include <stdio.h>
#include <stdlib.h>

typedef int bool;

typedef struct
{
  char *pName;
  unsigned pass;
  unsigned fail;
} UNIT_TEST;

/* allow us to check the unit tests for failure */
//#define UNIT_TEST_CHECK

/* take a sorted array with some elements duplicated,
   and compact the array by removing the duplicates,
   returning the new length of the array */
int compact(int *p,int size)
{
  #ifdef UNIT_TEST_CHECK
  (void)p;
  #else
  int index = 0;
  int j; /* for loop index */


  if (p && (size > 1))
  {
    while (index < (size - 1))
    {
      while (p[index] == p[index + 1])
      {
        /* move all entries down */
        for (j = index + 1; j < (size - 1); j++)
        {
          p[j] = p[j + 1];
        }
        /* adjust new size */
        size--;
        if (index >= (size - 1))
          break;
      }
      index++;
    }
  }
  #endif
  return (size);
}

/* generic unit test block */
static bool unit_test(UNIT_TEST *pTest,bool cond,char *pText)
{
  if (!cond)
  {
    if (pTest && pTest->pName)
      printf("%s - ",pTest->pName);
    if (pText)
      printf("%s\r\n",pText);
    if (pTest)
      pTest->fail++;
  }
  else if (pTest)
    pTest->pass++;

  return (cond);
}

/* unit tests for compact() */
static void testCompact(
  UNIT_TEST *pTest,
  int *array,
  int *new_array,
  int size,
  int new_size)
{
  int return_value;
  int i;


  /* test for our function */
  return_value = compact(array,size);

  if (unit_test(pTest,return_value == new_size,"wrong size returned!"))
  {
    for (i = 0; i < new_size; i++)
    {
      unit_test(pTest,array[i] == new_array[i],"compacted incorrectly!");
    }
  }
  return;
}

/* our given example test */
static void testCompact1(UNIT_TEST *pTest)
{
  int array[] = {1,3,7,7,8,9,9,9,10};
  int new_array[] = {1,3,7,8,9,10};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* small single number test */
static void testCompact2(UNIT_TEST *pTest)
{
  int array[] = {1,1};
  int new_array[] = {1};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* larger single number test */
static void testCompact3(UNIT_TEST *pTest)
{
  int array[] = {1,1,1,1};
  int new_array[] = {1};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* end of array duplicate test */
static void testCompact4(UNIT_TEST *pTest)
{
  int array[] = {1,2,2,2,2};
  int new_array[] = {1,2};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* beginning of array duplicate test */
static void testCompact5(UNIT_TEST *pTest)
{
  int array[] = {2,2,2,2,3,5,6};
  int new_array[] = {2,3,5,6};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* verify that it doesn't mess up a perfectly good array */
static void testCompact6(UNIT_TEST *pTest)
{
  int array[] = {1,3,7,8,9,10};
  int new_array[] = {1,3,7,8,9,10};
  int size = sizeof(array)/sizeof(int);
  int new_size = sizeof(new_array)/sizeof(int);

  testCompact(pTest,array,new_array,size,new_size);

  return;
}

/* verify that it can handle bad data */
static void testCompact7(UNIT_TEST *pTest)
{
  int *array = NULL;
  int size = 40;
  int return_value;


  /* test for our function */
  return_value = compact(array,size);

  unit_test(pTest,return_value == size,"wrong size returned!");

  return;
}

/* run the unit test */
int main(void)
{
  UNIT_TEST test_data = {"compact",0,0};

  testCompact1(&test_data);
  testCompact2(&test_data);
  testCompact3(&test_data);
  testCompact4(&test_data);
  testCompact5(&test_data);
  testCompact6(&test_data);
  testCompact7(&test_data);

  printf("compact - %u passed, %u failed - testing complete\r\n",
    test_data.pass,test_data.fail);

  return (EXIT_SUCCESS);
}
