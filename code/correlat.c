/**************************************************************************
*
* Copyright (c) 1997 Stephen Karg
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
* Description: Reads in a list of numbers from a file, and performs
*              the correlation calculation for those numbers.
*
* History: 1: 13-Jan-1997: Created File - Steve Karg
*          2: 17-Jan-1997: Added ability to comment data file with ;
*                          in first column. - Steve Karg
*
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* LOCAL CONTSTANTS */
#define MAX_LINE_SIZE (80)
#define TRUE (1)
#define FALSE (0)

/* structure for linked list */
typedef struct statistics
{
  double data[2];
  struct statistics *next;
} STATISTICS;

/* set up generic element */
typedef struct statistics ELEMENT;

/* set up the start of the linked list */
static ELEMENT *head;

/* set up debug */
unsigned char debug_flag;

/* set up counter type */
typedef unsigned long int COUNTER;

/* FUNCTION PROTOTYPES */
ELEMENT *create_list_element();
void add_element(ELEMENT *e);
void delete_elements();
unsigned long count_entries();
double xsum_entries();
double ysum_entries();
double xysum_entries();
double xsquare_sum_entries();
double ysquare_sum_entries();
double lower_square_root(COUNTER n,double x2i,double xi,double y2i,double yi);
double upper_formula(COUNTER n,double xiyi,double xi,double yi);
FILE *open_input_file(char *filename);
void load_file_into_list(FILE *file_ptr);

/**************************************************************************
*
* Function:    main   
*
* Description: Main task for linear regression.

* Parameters:  arcc - number of arguments passed into the program when
*                     run from the command line.
*              argv - a pointer to each of the arguments passed into
*                     the program from the command line.
*
* Return:      none
*
**************************************************************************/
void main(int argc,char *argv[])
{
  char filename[256];  /*input filename */
  COUNTER count;       /* number of entries */
  double xsum;         /* sum of x entries */
  double ysum;         /* sum of y entries */
  double xysum;        /* sum of x * y entries */
  double xsquare_sum;  /* sum of x squared entries */
  double ysquare_sum;  /* sum of x squared entries */
  double upper_num;    /* calculation on top */       
  double lower_num;    /* calculation on bottom */
  double r_xy;         /* final result */       
  double r_squared;    /* final result squared - should be >= 0.5 */       
  FILE *file_ptr;      /* file stream */
  char debug_string[256]; /* used to decode the debug string */

  debug_flag = FALSE;
  
  /* prompt for filename */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));
    if (argc > 2)
    {
      strcpy(debug_string,*(argv+2));
      if (debug_string[0] == 'd')
        debug_flag = TRUE;
    }
  }
  else
  {
    printf("Correlation for Size Estimates\n");
    printf("written by Steve Karg\n");
    printf("13-Jan-1997\n");
    printf("\n");
    printf("This program computes a correlation size estimating\n");
    printf("parameter for a set of n programs where historical function\n");
    printf("LOC and new and changed LOC data are available.\n");
    printf("\n");
    printf("Place the data in a file with two columns x and y seperated\n");
    printf("by a comma.\n");
    printf("\n");
    printf("Usage:\n");
    printf("      %s [filename]\n",*(argv+0));
    printf("\n");
    printf("\n");
    return;
  }

  /* open the file */
  file_ptr = open_input_file(filename);
  if (file_ptr == NULL)
    return;

  /* load the file into linked list */
  load_file_into_list(file_ptr);

  /* CORRELATION FORMULA 1:   
                                     n           n
                                    ---         ---    ---
                                  n \   x y   - \   x  \   y
                                    /    i i    /    i /    i
                                    ---         ---    ---
                                    i=1         i=1    i=1
              r(x,y)= ------------------------------------------------------
                       ----------------------------------------------------
                      | +-  n           n      -+ +-  n           n      -+
                      | |  ---   2     ---    2 | |  ---   2     ---    2 |
                      | |n \   x   - ( \   x )  | |n \   y   - ( \   y )  | 
                      | |  /    i      /    i   | |  /    i      /    i   | 
                      | |  ---         ---      | |  ---         ---      | 
                     \| |  i=1         i=1      | |  i=1         i=1      |
                        +-                     -+ +-                     -+ 
  */

  /* get n */
  count = count_entries();

  /* get summations */
  xsum = xsum_entries();
  ysum = ysum_entries();
  xysum = xysum_entries();
  xsquare_sum = xsquare_sum_entries();
  ysquare_sum = ysquare_sum_entries();
  lower_num = lower_square_root(count,xsquare_sum,xsum,ysquare_sum,ysum);
  upper_num = upper_formula(count,xysum,xsum,ysum);

  /* calculate the average and protect against divide by zero */
  if (lower_num != 0.0)
    r_xy = upper_num / lower_num;
  else
    r_xy = 0.0;

  /* CORRELATION FORMULA 2:
                 2    2
           r(x,y)  = r      
  */
  r_squared = r_xy * r_xy;

  /* Print the results */
  printf("r squared = %f\n",r_squared);
  printf("\n");

  /* House Keeping */
  delete_elements();

  return;
}

/* === Functions & Methods === */

/**************************************************************************
*
* Function:    create_list_element
*
* Description: Allocates memory for element in the linked list.
*
* Parameters:  none
*
* Globals:     none
*
* Locals:      ELEMENT - type of element.
*
* Return:      p - pointer to ELEMENT memory.
*
**************************************************************************/
ELEMENT *create_list_element()
{
  ELEMENT *p;

  p = (ELEMENT *) malloc( sizeof(ELEMENT));
  if (p==NULL)
  {
    printf("create_list_element: malloc failed.\n");
    exit(1);
  }
  p->next = NULL;
  return p;
} /* end of function */

/**************************************************************************
*
* Function:    add_element
*
* Description: Attaches element to last element in the linked list.
*
* Parameters:  none
*
* Globals:     none
*
* Locals:      head - start of the linked list.
*              ELEMENT - type of element.
*
* Return:      none
*
**************************************************************************/
void add_element(ELEMENT *e)
{
  ELEMENT *p;

  /* if the first element has not been created, create it now */
  if (head == NULL)
  {
    head = e;
    return;
  }

  /* otherwise, find the last element in the list */
  for (p=head;p->next != NULL;p=p->next) {}

  p->next = e;
  return;
}

/**************************************************************************
*
* Function:    delete_elements
*
* Description: Deletes memory of all elements in the linked list.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      head - start of the linked list.
*              ELEMENT - type of element.
*
* Return:      none
*
**************************************************************************/
void delete_elements()
{
  ELEMENT *current;
  ELEMENT *next;

  current = head;
  while(current != NULL)
  {
    next = current->next;
    free(current);
    current = next;
  }
  return;
}

/**************************************************************************
*
* Function:    count_entries
*
* Description: Counts the number of entries in a linked list.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      count - number of entries
*
**************************************************************************/
COUNTER count_entries()
{
  COUNTER count;
  ELEMENT *current;

  count = 0;
  current = head;

  while(current != NULL)
  {
    count++;
    current = current->next;
  }

  if (debug_flag)
    printf("count: %lu\n",count);

  return count;
}

/**************************************************************************
*
* Function:    xsum_entries
*
* Description: Performs a summation equation for the x data.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      sum - sum of entries
*
**************************************************************************/
double xsum_entries()
{
  double sum;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    sum += current->data[0];
    current = current->next;
  }

  if (debug_flag)
    printf("x: %f\n",sum);

  return sum;
}

/**************************************************************************
*
* Function:    ysum_entries
*
* Description: Performs a summation equation for the y data.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      sum - sum of entries
*
**************************************************************************/
double ysum_entries()
{
  double sum;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    sum += current->data[1];
    current = current->next;
  }

  if (debug_flag)
    printf("y: %f\n",sum);

  return sum;
}

/**************************************************************************
*
* Function:    xysum_entries
*
* Description: Performs a summation equation for the x*y data.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      sum - sum of x*y entries
*
**************************************************************************/
double xysum_entries()
{
  double sum;
  double x;
  double y;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    x = current->data[0];
    y = current->data[1];
    sum += (x * y);
    current = current->next;
  }

  if (debug_flag)
    printf("x times y: %f\n",sum);

  return sum;
}

/**************************************************************************
*
* Function:    xsquare_sum_entries
*
* Description: Performs a summation equation for the x squared data.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      sum - sum of x squared entries
*
**************************************************************************/
double xsquare_sum_entries()
{
  double sum;
  double x;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    x = current->data[0];
    sum += (x * x);
    current = current->next;
  }

  if (debug_flag)
    printf("x squared: %f\n",sum);

  return sum;
}

/**************************************************************************
*
* Function:    ysquare_sum_entries
*
* Description: Performs a summation equation for the y squared data.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      sum - sum of y squared entries
*
**************************************************************************/
double ysquare_sum_entries()
{
  double sum;
  double y;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    y = current->data[1];
    sum += (y * y);
    current = current->next;
  }

  if (debug_flag)
    printf("y squared: %f\n",sum);

  return sum;
}

/**************************************************************************
*
* Function:    upper_formula
*
* Description: Performs a summation equation.
*
* Parameters:  n     - number of elements.
*              xiyi  - x times y summation
*              xi    - x summation
*              xi    - x summation
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - calculation of equation.
*
**************************************************************************/
double upper_formula(COUNTER n,double xiyi,double xi,double yi)
{
  double result;

  result = 0.0;

  /* FORMULA:   
                                     n           n
                                    ---         ---    ---
                                  n \   x y   - \   x  \   y
                                    /    i i    /    i /    i
                                    ---         ---    ---
                                    i=1         i=1    i=1
  */

  result = (n * xiyi) - (xi * yi);
  
  if (debug_flag)
    printf("Upper equation: %f\n",result);

  return result;
}

/**************************************************************************
*
* Function:    lower_square_root
*
* Description: Performs a square root.
*
* Parameters:  n     - number of elements.
*              x2i   - x squared summation
*              y2i   - y squared summation
*              xi    - x summation
*              xi    - x summation
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - result of the calculation.
*
**************************************************************************/
double lower_square_root(COUNTER n,double x2i,double xi,double y2i,double yi)
{
  double result;
  double temp;

  result = 0.0;
  temp = 0.0;

  /* FORMULA:   
                       ----------------------------------------------------
                      | +-  n           n      -+ +-  n           n      -+
                      | |  ---   2     ---    2 | |  ---   2     ---    2 |
                      | |n \   x   - ( \   x )  | |n \   y   - ( \   y )  | 
                      | |  /    i      /    i   | |  /    i      /    i   | 
                      | |  ---         ---      | |  ---         ---      | 
                     \| |  i=1         i=1      | |  i=1         i=1      |
                        +-                     -+ +-                     -+ 
  */

  temp = ((n * x2i) - (xi *xi)) * ((n * y2i) - (yi * yi));

  /* don't try to take the square root of a negative number */
  if (temp > 0.0)
    result = sqrt(temp);
  else
    result = 0.0;

  if (debug_flag)
    printf("Square root: %f\n",result);

  return result;
}

/**************************************************************************
*
* Function:    open_input_file
*
* Description: Opens a file stream for input (read only) and returns a 
*              file pointer to that stream.
*
* Parameters:  filename - string containing the name of the file to be
*                         opened.
*
* Globals:     none
*
* Locals:      none
*
* Return:      fp - pointer to the stream of the file opened.
*
**************************************************************************/
FILE *open_input_file(char *filename)
{
  FILE *fp;

  fp = fopen(filename,"r");
  if (fp == NULL)
    printf("open_input_file: error opening %s.\n",filename);

  return fp;
}

/**************************************************************************
*
* Function:    load_file_into_list
*
* Description: Loads the data in a file into the linked list structure.
*
* Parameters:  file_ptr - pointer to the stream of the file opened.
*
* Globals:     none
*
* Locals:      none
*
* Return:      none
*
**************************************************************************/
void load_file_into_list(FILE *file_ptr)
{
  char line[MAX_LINE_SIZE]; /* line to be read in from file */
  char *str1;               /* temp string pointer 1 */
  char *str2;               /* temp string pointer 2 */
  ELEMENT *temp_node;       /* temporary element */

  if (debug_flag)
    printf("Reading file...\n");
  /* read the entries and load into linked list */
  while (fgets(line,MAX_LINE_SIZE,file_ptr) != NULL)
  {
    if (line[0] != ';')
    {
      /* create element */
      temp_node = create_list_element();
  
      /* Parse line into seperate numbers and load */
      str1 = strtok(line,",");
      temp_node->data[0] = atof(str1);
      str2 = strtok(NULL,"\n");
      temp_node->data[1] = atof(str2);
  
      /* attach element to list */
      add_element(temp_node);
  
      /* DEBUG */
      if (debug_flag)
      {
        printf("Number 1 = %s (%f)\tNumber 2 = %s (%f)\n",
                str1,temp_node->data[0],
                str2,temp_node->data[1]);
      } /* end of debug */
    } /* end of non-comment line */
  } /* end of getting string from file */
  
  /* close the file */
  fclose(file_ptr);
}
