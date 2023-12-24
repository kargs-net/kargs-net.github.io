/*********************************************************************
*
* Copyright (c) 2000 Stephen Karg
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
* Project:      Text Counter
*
* Task Domains: MS-DOS
*
* Functional
* Description:  This file is designed to run from DOS to count how many
*               times a piece of text occurs.  This is useful for taking
*               source safe reports and counting lines added "Ins:",
*               deleted "Del:", or modified "To:".
*
*********************************************************************/
static char version_date[]   = {"08-Feb-2000"};
static char version_number[] = {"1.00"};

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* LOCAL CONTSTANTS */
#define MAX_LINE_SIZE (128)
#define TRUE (1)
#define FALSE (0)

/**************************************************************************
* NAME:        Usage
* Description: Prints a help message to the user about how to use this
*              program.
* Parameters:  filename - name entered to get this program to run.
* Globals:     none
* Locals:      none
* Return:      none
**************************************************************************/
void Usage(char *filename)
{
  char *str = NULL;
  char *name = NULL;

  /* Find the last token - that is the actual filename */
  str = strtok(filename,":\\");
  while (str != NULL)
  {
    name = str;
    str = strtok(NULL,":\\");
  }

  printf("Text counter for text files\n");
  printf("Version %s by Steve Karg. Last update %s.\n",
    version_number,version_date);
  printf("\n");
  printf("This program counts how many times a piece of text occurs\n"
         "in a file.  This is useful for taking source safe reports\n"
         "created from ShowHistory-Reports-IncludeDifferences-ToFile\n"
         "and looking for lines added \"Ins:\", deleted \"Del:\", or\n"
         "modified \"To:\"\n");
  printf("\n");
  printf("Usage:\n");
  printf("  %s <filename> <case sensitive search text>\n",
    name ? name : "COUNT");
  printf("\n");

  return;
}

/******************************************************************
* NAME:         main
* DESCRIPTION:  Main funtion.
* PARAMETERS:   argc (IN) number of arguments entered at command line.
*               argv (IN) arguments in string form.
* GLOBALS:      none
* RETURN:       error level
* ALGORITHM:    none
* NOTES:        none
******************************************************************/
int main (int argc, char* argv[])
{
  unsigned long int value = 0; /* return value */
  FILE *pFile = NULL; /* file stream handle */
  char line[MAX_LINE_SIZE] = {""}; /* input line */


  /* are the filename and text argument included? */
  if (argc < 3)
  {
    Usage(argv[0]);
    exit(0);
  }

  pFile = fopen(argv[1],"r");

  if (pFile)
  {
    while (fgets(line, sizeof(line), pFile) != NULL)
    {
      if (strstr(line,argv[2]) != NULL)
        value++;
    }
    fclose(pFile);
  }
  else
  {
    printf("Error opening %s.\n",argv[1]);
    exit(0);
  }

  printf("%lu\n",value);

  return (0);
}
