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
* Description: Reads in a list of numbers from a file, and calculates
*              the mean and standard deviation for those numbers.
*
* History: 1: 22-Nov-1996: Created File - Steve Karg
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

/* structure for linked list */
typedef struct statistics
{
  double data;
  struct statistics *next;
} STATISTICS;

/* set up generic element */
typedef struct statistics ELEMENT;

/* set up the start of the linked list */
static ELEMENT *head;

/* FUNCTION PROTOTYPES */
ELEMENT *create_list_element();
void add_element(ELEMENT *e);
void delete_elements();
unsigned long count_entries();
double sum_entries();
double sigma_entries();
FILE *open_file(char *filename);

/* MAIN */
void main(void)
{
  char filename[256];  /*input filename */
  ELEMENT *temp_node;  /* temporary element */
  unsigned long count; /* number of entries */
  double sum;          /* sum of entries */
  double mean;         /* average of the entries */
  double sigma;        /* sum of entries - mean */
  double stddev;       /* standard deviation */
  double temp;
  FILE *file_ptr;      /* file stream */
  char line[MAX_LINE_SIZE]; /* line to be read in from file */

  /* prompt for filename */
  printf("Enter the filename: ");
  scanf("%s",filename);

  /* open the file */
  file_ptr = open_file(filename);
  if (file_ptr == NULL)
    return;

  /* read the entries and load into linked list */
  while (fgets(line,MAX_LINE_SIZE,file_ptr) != NULL)
  {
    if (line[0] != ';')
    {
      /* create element and load list */
      temp_node = create_list_element();
      temp_node->data = atof(line);
      add_element(temp_node);
    } /* end of non-comment line */
  } /* end of getting string from file */
  /* close the file */
  fclose(file_ptr);

  /* Determine the MEAN */
  count = count_entries();
  sum = sum_entries();
  if (count != 0)
    mean = sum / count;
  else
    mean = 0.0;

  /* Determine the Standard Deviation */
  sigma = sigma_entries(mean);
  if (count > 1)
  {
    temp = sigma/(count-1);
    if (temp > 0.0)
      stddev = sqrt(temp);
    else
      stddev = 0.0;
  }
  else
    stddev = 0.0;

  /* Print the results */
  printf("\nThe MEAN is: %f",mean);
  printf("\nThe STANDARD DEVIATION is: %f",stddev);
  printf("\n=====================================\n");

  /* House Keeping */
  delete_elements();

  return;
}

/* Functions & Methods */
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

double sum_entries()
{
  double sum;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    sum += current->data;
    current = current->next;
  }
  return sum;
}

double sigma_entries(double mean)
{
  double sigma;
  ELEMENT *current;

  sigma = 0.0;
  current = head;

  while(current != NULL)
  {
    /* sum of all entry-avg squared */
    sigma += ((current->data-mean)*(current->data-mean));
    current = current->next;
  }
  return sigma;
}

FILE *open_file(char *filename)
{
  FILE *fp;
  char name[256];

  sprintf(name,":cfg:%s",filename);

  fp = fopen(name,"r");
  if (fp == NULL)
    printf("\nopen_file: error opening %s.\n",name);

  return fp;
}

