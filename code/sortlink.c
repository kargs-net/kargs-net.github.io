/**************************************************************************
*
* Filename:    sortlink.c
*
* Description: Reads in a list of numbers from a file, and sorts them.
*
* History: 1: 14-May-1997: Created File - Steve Karg
*
**************************************************************************/
static char version_name[]  = {"Linked List Sort"};
static char version_author[]= {"Steve Karg"};
static char version_date[]  = {"14-May-1997"};
static char version_number[]= {"1.0"};

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* LOCAL CONSTANTS */
#define MAX_LINE_SIZE (80)
#define TRUE  (1)
#define FALSE (0)
#define MAX_COLUMN (2)

/* LOCAL TYPEDEFS */
/* structure for linked list */
typedef struct statistics
{
  double data[MAX_COLUMN];
  struct statistics *next;
} STATISTICS;

/* set up generic element */
typedef struct statistics ELEMENT;

/* set up counter type */
typedef unsigned long int COUNTER;

/* LOCAL VARIABLES */
/* set up the start of the linked list */
static ELEMENT *head = NULL;

/* set up debug */
static unsigned char debug_flag = FALSE;

/* FUNCTION PROTOTYPES */
ELEMENT *create_list_element(void);
void add_element(ELEMENT *e);
void delete_elements(void);
FILE *open_input_file(char *filename);
void load_file_into_list(FILE *file_ptr);
void interpret_arguments(char *argument);
void usage(char *filename);
FILE *open_output_file(char *filename);

void sort_elements(unsigned short sort_column);
void swap_elements(ELEMENT *prev,ELEMENT *current,ELEMENT *next);
void print_elements_to_file(char *filename);

/**************************************************************************
*
* Function:    main   
*
* Description: Main task for sorting a linked list.
*
* Parameters:  arcc - number of arguments passed into the program when
*                     run from the command line.
*              argv - a pointer to each of the arguments passed into
*                     the program from the command line.
*
* Return:      none
*
**************************************************************************/
int main(int argc,char *argv[])
{
  char filename[256] = {""};      /* input filename */
  FILE *file_ptr = NULL;          /* input file stream */
  int argc_index = 0;             /* for looping all the arguments passed */
  unsigned short sort_column = 0; /* column number to sort on */
  
  /* retrieve filename */
  if (argc > 2)
  {
    strcpy(filename,*(argv+1));
    sort_column = atoi(*(argv+2));

    /* adjust the column index to be an array index */
    if (sort_column != 0)
      sort_column--;
    
    if (argc > 3)
    {
      argc_index = 3;
      while (argc > argc_index)
      {
        interpret_arguments(*(argv+argc_index));
        argc_index++;
      }
    } /* end of 3+ args */
  } /* end of arg 1 & 2 */
  /* Improper minimum number of parameters entered - display help screen */
  else
  {
    usage(*(argv));
    return 0;
  }

  /* open the input file */
  file_ptr = open_input_file(filename);
  if (file_ptr == NULL)
    return 1;

  /* load the file into linked list and close file */
  load_file_into_list(file_ptr);

  /* sort the list */
  sort_elements(sort_column);

  /* Print the results to a file */
  print_elements_to_file(filename);

  /* House Keeping */
  delete_elements();

  return 0;
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
ELEMENT *create_list_element(void)
{
  ELEMENT *p = NULL;

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
  ELEMENT *p = NULL;

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
void delete_elements(void)
{
  ELEMENT *current = NULL;
  ELEMENT *next = NULL;

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
* Function:    sort_elements
*
* Description: Sorts the entries in a linked list.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      head - the first element in the linked list
*
* Return:      none
*
**************************************************************************/
void sort_elements(unsigned short sort_column)
{
  ELEMENT *prev = {NULL};
  ELEMENT *current = {NULL};
  ELEMENT *next = {NULL};
  unsigned char finished;
  
  finished = FALSE;

  if (sort_column >= MAX_COLUMN)
    sort_column = MAX_COLUMN - 1;

  while (!finished)
  {
    /* start at the beginning of the list */
    prev = NULL;
    current = head;
    next = current->next;

    /* assume list is sorted */
    finished = TRUE;
    
    while((current != NULL) && (next != NULL))
    {
      /* Perform bubble sort */
      if (next->data[sort_column] < current->data[sort_column])
      {
        swap_elements(prev,current,next);
        /* flag it to attempt a re-sort - if none need switched, then
           we are finished */
        finished = FALSE;        
        /* since next is now current, and current is already next... */
        prev = next;
        next = current->next;
      }
      else
      {
        prev = current;
        current = next;
        next = current->next;
      }
    }
  }
  
  return;
}

/**************************************************************************
*
* Function:    swap_elements
*
* Description: Swaps two entries in a linked list.
*              
* Parameters:  none
*
* Globals:     none
*
* Locals:      none
*
* Return:      none
*
**************************************************************************/
void swap_elements(ELEMENT *prev,ELEMENT *current,ELEMENT *next)
{
  /* bad parameters! */
  if ((current == NULL) || (next == NULL))
  {
    printf("swap_elements: bad current or next arguments.\n");
  }

  /* first element pair */
  else if (prev == NULL)
  {
    if (current->next != next)
      printf("swap_elements: current not pointing to next!\n");
    
    current->next = next->next;
    next->next = current;
    if (current == head)
      head = next;
  }

  /* normal swap */
  else
  {
    if (prev->next != current)
      printf("swap_elements: prev not pointing to current!\n");
    if (current->next != next)
      printf("swap_elements: current not pointing to next!\n");
    prev->next = next;
    current->next = next->next;
    next->next = current;
    if (current == head)
      head = next;
  }

  return;
}

/**************************************************************************
* Function:    open_output_file
*
* Description: Opens a file stream for output (write only) and returns a 
*              file pointer to that stream.
*
* Parameters:  filename - string containing the name of the input file.
*
* Globals:     none
*
* Locals:      none
*
* Return:      fp - pointer to the stream of the file opened.
*
**************************************************************************/
FILE *open_output_file(char *filename)
{
  FILE *fp = NULL;
  char output_filename[256] = {""};
  char *str = NULL;
  char *name = NULL;
  
  /* Assume the same filename with OUT extension */
  /* Find the last token - that is the actual filename */
  strcpy(output_filename,filename);
  /* Add the extension */
  str = strrchr(output_filename, '.');
  if (str != NULL)
  {
    *str = 0;
    strcat(str,".out");
  }
  else
    strcat(output_filename,".out");

  fp = fopen(output_filename,"wt");
  if (fp == NULL)
    printf("open_output_file: error opening %s.\n",output_filename);
  else
    printf("Opened %s for output.\n",output_filename);

  return fp;
}

/**************************************************************************
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
  FILE *fp = NULL;

  fp = fopen(filename,"rt");
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
* Locals:      debug_flag - flag used for debugging the program.
*
* Return:      none
*
**************************************************************************/
void load_file_into_list(FILE *file_ptr)
{
  char line[MAX_LINE_SIZE] = {""}; /* line to be read in from file */
  char *str1 = NULL;               /* temp string pointer 1 */
  char *str2 = NULL;               /* temp string pointer 2 */
  ELEMENT *temp_node = NULL;       /* temporary element */

  if (debug_flag)
    printf("Reading file...\n");
  /* read the entries and load into linked list */
  while (fgets(line,MAX_LINE_SIZE,file_ptr) != NULL)
  {
    /* skip comments - denoted by a semicolon in column 0 */
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
  return;
}

/**************************************************************************
*
* Function:    print_elements_to_file
*
* Description: Loads the linked list structure to data in a file.
*
* Parameters:  filename - name of the input file, which will be modified
*              to be the output file.
*
* Globals:     none
*
* Locals:      debug_flag - flag used for debugging the program.
*              head - first element in the list.
*
* Return:      none
*
**************************************************************************/
void print_elements_to_file(char *filename)
{
  ELEMENT *current = NULL;
  FILE *file_ptr = NULL;           /* input file stream */

  /* open the output file using the input filename */
  file_ptr = open_output_file(filename);
  if (file_ptr == NULL)
    return;

  /* write a header to the file */
  if (debug_flag)
  {
    printf(";Sorted list of data\n");
  }

  fprintf(file_ptr,";Sorted list of data\n");

  /* write the entries from linked list to the file */
  current = head;
  while(current != NULL)
  {
    fprintf(file_ptr,"%f,%f\n",current->data[0],current->data[1]);
    if (debug_flag)
      printf("%f,%f\n",current->data[0],current->data[1]);

    current = current->next;
  }
  
  /* close the file */
  fclose(file_ptr);
  return;
}

/**************************************************************************
*
* Function:    interpret_arguments
*
* Description: Takes one of the arguments passed by the main function
*              and sets flags if it matches one of the predefined args.
*
* Parameters:  argument - pointer to a string containing the argument.
*
* Locals:      debug_flag
*
* Return:      none
*
**************************************************************************/
void interpret_arguments(char *argument)
{
  char work_string[256] = {""};
  int str_len = 0;

  /* safety net - cap the string if it's too big. */
  str_len = strlen(argument);
  if (str_len > 256)
    argument[256] = 0;

  strcpy(work_string,argument);
  if (work_string[0] == '-')
  {
    switch(work_string[1])
    {
      case 'd':
      case 'D':
        debug_flag = TRUE;
        break;
      default:
        break;
    }
  }
}

/**************************************************************************
*
* Function:    usage
*
* Description: Prints the help and usage information to the user.
*
* Parameters:  filename - name of the file that they typed.
*
* Locals:      version_number - version of this program
*              version_date - date of last change
*
* Return:      none
*
**************************************************************************/
void usage(char *filename)
{
  char *str = NULL;
  char *name = NULL;
  
  /* Find the last token - that is the actual filename */
  str = strtok(filename,":/");
  while (str != NULL)
  {
    name = str;
    str = strtok(NULL,":/");
  }
  
  printf("%s\n",version_name);
  printf("written by %s\n",version_author);
  printf("Version %s last modified on %s.\n",version_number,version_date);
  printf("\n");
  printf("This program sorts a file containing columns of numbers.\n");
  printf("You can choose which column to sort on.\n");
  printf("\n");
  printf("Place the data in a file with two columns x and y seperated\n");
  printf("by a comma.\n");
  printf("\n");
  printf("SYNTAX:\n");
  if (name != NULL)
    printf("%s filename column [-d]\n",name);
  else
    printf("SORTLINK filename column [-d]\n");
  printf("  filename  name of the file with the data.\n");
  printf("  column    column number to sort on, first column is 1.\n");
  printf("  -d        turn on debug.\n");
  printf("\n");

  return;
}
