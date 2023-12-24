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
*********************************************************************/

/* includes */
#include <string.h>
#include <ctype.h>
#if defined(__BORLANDC__)
  #include <mem.h>
#endif

#ifdef TEST
  #include <stdio.h>
  #include <assert.h>
  #if defined(__BORLANDC__)
    #include <conio.h>
  #endif
  #include "ctest.h"
#endif

/******************************************************************
* NAME:         rmlead
* DESCRIPTION:  Remove leading whitespace
* PARAMETERS:   none
* GLOBALS:      none
* RETURN:       none
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
void rmlead(char *str)
{
  char *obuf;

  if (str)
  {
    for (obuf = str; *obuf && isspace(*obuf); ++obuf) {}
    if (str != obuf)
      memmove(str,obuf,strlen(obuf)+1);
  }
  return;
}

/******************************************************************
* NAME:         rmtrail
* DESCRIPTION:  Remove the trailing white space from a string
* PARAMETERS:   none
* GLOBALS:      none
* RETURN:       none
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
void rmtrail(char *str)
{
  size_t i;

  if (str && 0 != (i = strlen(str)))
  {
    /* start at end, not at 0 */
    --i;
    do
    {
      if (!isspace(str[i]))
        break;
    } while (--i);
    str[++i] = '\0';
  }
  return;
}

/******************************************************************
* NAME:         rmquotes
* DESCRIPTION:  Remove single or double quotation marks
*               enclosing the string 
* PARAMETERS:   str (IO) string containing quotes
* GLOBALS:      none
* RETURN:       0 if unsuccessful
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
int rmquotes(char *str)
{
  size_t len;
  int status = 0;

  if (str)
  {
    len = strlen(str);
    if (len > 1)
    {
      if (((str[0]     == '\'') &&
           (str[len-1] == '\'')) ||
          ((str[0]     == '\"') &&
           (str[len-1] == '\"')))
      {
        str[len-1] = '\0';
        memmove(str,&str[1],len);
        status = 1;
      }
    }
  }
  return (status);
}

/******************************************************************
* NAME:         rmbrackets
* DESCRIPTION:  Remove brackets enclosing the string 
* PARAMETERS:   str (IO) string containing brackets
* GLOBALS:      none
* RETURN:       0 if unsuccessful
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
int rmbrackets(char *str)
{
  size_t len;
  int status = 0;

  if (str)
  {
    len = strlen(str);
    if (len > 1)
    {
      if ((str[0]     == '[') &&
          (str[len-1] == ']'))
      {
        str[len-1] = '\0';
        memmove(str,&str[1],len);
        status = 1;
      }
    }
  }
  return (status);
}

#ifdef TEST
void test_rmlead(Test* pTest)
{
  char token[80];
  char result[80];
  size_t len;

  strcpy(token,"  spacing is crucial  ");
  strcpy(result,"spacing is crucial  ");
  len = strlen(token);
  rmlead(token);
  ct_test(pTest, strlen(token) == (len-2));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial  ");
  strcpy(result,token);
  len = strlen(token);
  rmlead(token);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"s          spacing is crucial   ");
  strcpy(result,token);
  len = strlen(token);
  rmlead(token);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  return;
}

void test_rmtrail(Test* pTest)
{
  char token[80];
  char result[80];
  size_t len;

  strcpy(token,"  spacing is crucial ");
  strcpy(result,"  spacing is crucial");
  len = strlen(token);
  rmtrail(token);
  ct_test(pTest, strlen(token) == (len-1));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"  spacing is crucial  ");
  strcpy(result,"  spacing is crucial");
  len = strlen(token);
  rmtrail(token);
  ct_test(pTest, strlen(token) == (len-2));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"  spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  rmtrail(token);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"   spacing is crucial   s");
  strcpy(result,token);
  len = strlen(token);
  rmtrail(token);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  return;
}

void test_rmquotes(Test* pTest)
{
  char token[80];
  char result[80];
  size_t len;
  int status;

  strcpy(token,"\"spacing is crucial\"");
  strcpy(result,"spacing is crucial");
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status != 0);
  ct_test(pTest, strlen(token) == (len-2));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"\'spacing is crucial\'");
  strcpy(result,"spacing is crucial");
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status != 0);
  ct_test(pTest, strlen(token) == (len-2));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"\'spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"\"spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial\'");
  strcpy(result,token);
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial\"");
  strcpy(result,token);
  len = strlen(token);
  status = rmquotes(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  return;
}

void test_rmbrackets(Test* pTest)
{
  char token[80];
  char result[80];
  size_t len;
  int status;

  strcpy(token,"[spacing is crucial]");
  strcpy(result,"spacing is crucial");
  len = strlen(token);
  status = rmbrackets(token);
  ct_test(pTest, status != 0);
  ct_test(pTest, strlen(token) == (len-2));
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  status = rmbrackets(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"[spacing is crucial");
  strcpy(result,token);
  len = strlen(token);
  status = rmbrackets(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  strcpy(token,"spacing is crucial]");
  strcpy(result,token);
  len = strlen(token);
  status = rmbrackets(token);
  ct_test(pTest, status == 0);
  ct_test(pTest, strlen(token) == len);
  ct_test(pTest, strcmp(result,token) == 0);

  return;
}
#endif

#ifdef TEST_RMSPACE
int main(void)
{
  Test *pTest;
  bool rc;

  pTest = ct_create("rmspace", NULL);

  /* individual tests */
  rc = ct_addTestFunction(pTest, test_rmlead);
  assert(rc);
  rc = ct_addTestFunction(pTest, test_rmtrail);
  assert(rc);
  rc = ct_addTestFunction(pTest, test_rmquotes);
  assert(rc);
  rc = ct_addTestFunction(pTest, test_rmbrackets);
  assert(rc);

  ct_setStream(pTest, stdout);
  ct_run(pTest);
  (void)ct_report(pTest);

  ct_destroy(pTest);

  printf("Press key to quit");
  getch();

  return 0;
}
#endif

