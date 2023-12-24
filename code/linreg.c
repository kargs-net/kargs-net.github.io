/**************************************************************************
*
* Copyright (c) 1996, 1997 Stephen Karg
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
*              the the linear regression calculation for those numbers.
*
* History: 1: 07-Jan-1996: Created File - Steve Karg
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

/* FUNCTION PROTOTYPES */
ELEMENT *create_list_element();
void add_element(ELEMENT *e);
void delete_elements();
unsigned long count_entries();
double xsum_entries();
double ysum_entries();
double upper_sigma_entries(double xmean,double ymean,unsigned long count);
double lower_sigma_entries(double xmean,unsigned long count);
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
  unsigned long count; /* number of entries */
  double xsum;         /* sum of x entries */
  double ysum;         /* sum of y entries */
  double xmean;        /* average of the x entries */
  double ymean;        /* average of the y entries */
  double upper_sigma;  /* top summation - see formula */
  double lower_sigma;  /* bottom summation - see formula */
  double beta0;        /* first linear regression parameter */
  double beta1;        /* second linear regression parameter */
  double estimate;     /* user entered parameter */
  double result;       /* result of calculation using entered parameter */
  unsigned char calc_flag;  /* flag to do calc */
  FILE *file_ptr;      /* file stream */
  char debug_string[256]; /* used to decode the debug string */

  debug_flag = FALSE;
  calc_flag = FALSE;
  
  /* prompt for filename */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));
    if (argc > 2)
    {
      strcpy(debug_string,*(argv+2));
      if (debug_string[0] == 'd')
        debug_flag = TRUE;
      else
      {
        estimate = atof(*(argv+2));
        calc_flag = TRUE;
      }

      if (argc > 3)
      {
        strcpy(debug_string,*(argv+2));
        if (debug_string[0] == 'd')
          debug_flag = TRUE;
      }
    }
  }
  else
  {
    printf("\nLinear Regression for Size Estimates");
    printf("\nwritten by Steve Karg");
    printf("\n07-Jan-1996");
    printf("\n");
    printf("\nThis program computes linear regression size estimating");
    printf("\nparameters for a set of n programs where historical function");
    printf("\nLOC and new and changed LOC data are available.");
    printf("\n");
    printf("\nPlace the data in a file with two columns x and y seperated");
    printf("\nby a comma.");
    printf("\n");
    printf("\nUsage:");
    printf("\n      %s [filename] [estimate] [[d]ebug]",*(argv+0));
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

  /* LINEAR REGRESSION FORMULA 1:   
                        n
                       ---
                       \   x y   - n x     y
                       /    i i       avg   avg
                       --- 
                       i=1
              beta1 = ------------------------
                        n
                       ---   2           2
                       \   x     - n x    
                       /    i         avg
                       --- 
                       i=1
  */

  /* get n */
  count = count_entries();

  /* get x and y average */
  xsum = xsum_entries();
  ysum = ysum_entries();

  /* calculate the average and protect against divide by zero */
  if (count != 0)
  {
    xmean = xsum / count;
    ymean = ysum / count;
  }
  else
  {
    xmean = 0.0;
    ymean = 0.0;
  }

  /* get top sigma */
  upper_sigma = upper_sigma_entries(xmean,ymean,count);
  
  /* get bottom sigma */
  lower_sigma = lower_sigma_entries(xmean,count);

  /* calculate beta1 and protect against divide by zero */
  if (lower_sigma == 0.0)
    beta1 = 0.0;
  else
    beta1 = upper_sigma / lower_sigma;

  /* LINEAR REGRESSION FORMULA 2:

           beta0 = y    - beta1 x
                    avg          avg
  */
  beta0 = ymean - (beta1*xmean);

  /* if user entered a parameter, plug it in. */
  if (calc_flag)
  {
    result = beta0 + beta1 * estimate;
  }

  /* Print the results */
  printf("\nx sum=%8f  x mean=%8f",xsum,xmean);
  printf("\ny sum=%8f  y mean=%8f",ysum,ymean);
  printf("\nNumber of entries=%Lu",count);
  printf("\nB0=%f",beta0);
  printf("\nB1=%f",beta1);
  if (calc_flag)
    printf("\nEstimate In=%f Estimate Out=%f",estimate,result);
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
    printf("\ncreate_list_element: malloc failed.\n");
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
unsigned long count_entries()
{
  unsigned long count;
  ELEMENT *current;

  count = 0;
  current = head;

  while(current != NULL)
  {
    count++;
    current = current->next;
  }
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
  return sum;
}

/**************************************************************************
*
* Function:    upper_sigma_entries
*
* Description: Performs a summation equation.
*
* Parameters:  xmean - the mean of the x entries.
*              ymean - the mean of the y entries.
*              count - number of entries.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - sum of equations for sigma.
*
**************************************************************************/
double upper_sigma_entries(double xmean,double ymean,unsigned long count)
{
  double sigma;
  double result;
  ELEMENT *current;

  sigma = 0.0;
  result = 0.0;
  current = head;

  /* FORMULA:   
                      n
                     ---
                     \   x y   - n x     y
                     /    i i       avg   avg
                     --- 
                     i=1
  */
  while(current != NULL)
  {
    sigma += ((current->data[0]) * (current->data[1]));
    current = current->next;
  }

  result = sigma - (count * xmean * ymean);
  return result;
}

/**************************************************************************
*
* Function:    lower_sigma_entries
*
* Description: Performs a summation equation.
*
* Parameters:  xmean - the mean of the x entries.
*              count - number of entries.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - sum of equations for sigma.
*
**************************************************************************/
double lower_sigma_entries(double xmean,unsigned long count)
{
  double sigma;
  double result;
  ELEMENT *current;

  sigma = 0.0;
  result = 0.0;
  current = head;

  while(current != NULL)
  {
    /* FORMULA:   
                        n
                       ---   2           2
                       \   x     - n x    
                       /    i         avg
                       --- 
                       i=1
    */
    sigma += ((current->data[0]) * (current->data[0]));
    current = current->next;
  }
  result = sigma - (count * xmean * xmean);
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
    printf("\nReading file...");
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
        printf("\nNumber 1 = %s (%f)\tNumber 2 = %s (%f)",
                str1,temp_node->data[0],
                str2,temp_node->data[1]);
      } /* end of debug */
    } /* end of non-comment line */
  } /* end of getting string from file */
  
  /* close the file */
  fclose(file_ptr);
}
