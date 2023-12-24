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
*              a chi squared test on them.
*
* History:  : 14-May-1997: Created sortlink file. - Steve Karg
*          1: 30-Jun-1997: Added chi squared test. - Steve Karg
*          2: 10-Jul-1997: Corrected during test. - Steve Karg
*
**************************************************************************/
static char version_name[]  = {"Chi Squared Test"};
static char version_author[]= {"Steve Karg"};
static char version_date[]  = {"10-Jul-1997"};
static char version_number[]= {"1.2"};

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

/* LOCAL CONSTANTS */
#define MAX_LINE_SIZE (80)
#define TRUE  (1)
#define FALSE (0)
#define PI    (3.1415926535898)
#define MAX_COLUMN (2)

/* LOCAL TYPEDEFS */
/* structure for linked list */
typedef struct statistics
{
  double data_x;
  double data_y;
  double normal_form;
  double cumulative_fraction;
  struct statistics *next;
} STATISTICS;

/* set up generic element */
typedef struct statistics ELEMENT;

/* set up counter type */
typedef unsigned long int COUNTER;

/* set up degree of freedom type */
typedef long int DOF;

/* LOCAL VARIABLES */
/* set up the start of the linked list */
static ELEMENT *head = NULL;

/* set up debug */
static unsigned char debug_flag = FALSE;
char debug_string[256] = {""};

/* FUNCTION PROTOTYPES */
/* Linked list general functions */
ELEMENT *create_list_element(void);
void add_element(ELEMENT *e);
void delete_elements(void);
void load_file_into_list(FILE *file_ptr);

/* Linked list special functions */
double xsum_entries(void);
COUNTER count_entries(void);
double calculate_variance(double xavg);
void calculate_normal_form(double stddev,double xavg);
COUNTER determine_maximum_segments(COUNTER count);
void compute_normal_table(COUNTER count);
double calculate_q_value(COUNTER segments,COUNTER count);

/* File functions */
FILE *open_input_file(char *filename);

/* General functions */
void interpret_arguments(char *argument);
void usage(char *filename);
void print_results(double q,double p,COUNTER s,COUNTER n);

/* Sort functions */
void sort_data_x_elements(void);
void swap_elements(ELEMENT *prev,ELEMENT *current,ELEMENT *next);

/* Chi squared functions */
double simpson_integrate_chi(double val,double dof);
double chi_distribution_function(double x,double dof);
double gamma_n_over_2(DOF dof);

/* Normal distribution functions */
double find_nval(double probability,COUNTER count);
double simpson_integrate_normal(double n_val,COUNTER count);
double normal_distribution_function(double x);

/**************************************************************************
*
* Function:    main   
*
* Description: Main task for performing a chi square test.
*
* Parameters:  arcc - number of arguments passed into the program when
*                     run from the command line.
*              argv - a pointer to each of the arguments passed into
*                     the program from the command line.
*
* Return:      TRUE - failure
*              FALSE - success
*
**************************************************************************/
int main(int argc,char *argv[])
{
  char filename[256] = {""};      /* input filename */
  FILE *file_ptr = NULL;          /* input file stream */
  int argc_index = 0;             /* for looping all the arguments passed */

  COUNTER count = 0;              /* number of entries */
  COUNTER segments = 0;           /* number of segments for Q evaluation */
  double xsum = 0.0;              /* sum of x entries */
  double xmean = 0.0;             /* average of the x entries */
  double variance = 0.0;          /* variance of the data entries */
  double stddev = 0.0;            /* square root of the variance */
  double q_val = 0.0;             /* q value from segment table */
  DOF dof = 0;                    /* degrees of freedom */
  double probability = 0.0;       /* chi squared probability of Q */
  
  /* retrieve filename */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));

    if (argc > 2)
    {
      argc_index = 2;
      while (argc > argc_index)
      {
        interpret_arguments(*(argv+argc_index));
        argc_index++;
      }
    } /* end of 2+ args */
  } /* end of arg 1 */
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

  /* get n */
  count = count_entries();

  /* add up all x entries */
  xsum = xsum_entries();

  /* calculate the average and protect against divide by zero */
  if (count != 0)
    xmean = xsum / count;
  else
    xmean = 0.0;

  /* get the variance so we can calculate the standard deviation */
  variance = calculate_variance(xmean);

  /* calculate the standard deviation and 
     protect against negative square roots */
  if (variance > 0.0)
    stddev = sqrt(variance);
  else 
    stddev = 0.0;

  /* Put the normal form into the linked list */
  calculate_normal_form(stddev,xmean);

  /* Put the cumulative fraction into the linked list */
  compute_normal_table(count);

  /* Calculate the number of segments */
  segments = determine_maximum_segments(count);
  
  /* Get the Q value. */
  q_val = calculate_q_value(segments,count);

  /* Get the probability */
  dof = segments - 1;
  probability = simpson_integrate_chi(q_val,(double)dof);

  /* Print results */
  print_results(q_val,probability,segments,count);

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
COUNTER count_entries(void)
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
double xsum_entries(void)
{
  double sum;
  ELEMENT *current;

  sum = 0.0;
  current = head;

  while(current != NULL)
  {
    sum += current->data_x;
    current = current->next;
  }
  return sum;
}

/**************************************************************************
*
* Function:    calculate_normal_form
*
* Description: Calculates the normal form for a linked list of data.
*
* Parameters:  stddev - standard deviation of the data
*              xavg - the average value of the data
*
* Globals:     none
*
* Locals:      head - start of the linked list
*
* Return:      none
*
**************************************************************************/
void calculate_normal_form(double stddev,double xavg)
{
  ELEMENT *current;

  current = head;

  while(current != NULL)
  {
    current->normal_form = (current->data_x - xavg) / stddev;
    current = current->next;
  }
  return;
}

/**************************************************************************
*
* Function:    compute_normal_table
*
* Description: Calculates the cumulative fraction for a linked list of data.
*
* Parameters:  count - number of data elements
*
* Globals:     none
*
* Locals:      head - start of the linked list
*
* Return:      none
*
**************************************************************************/
void compute_normal_table(COUNTER count)
{
  COUNTER index;
  ELEMENT *current;

  /* initialize */
  current = head;
  index = 1;

  /* Sort the linked list prior to calculating the cumalative fraction */
  sort_data_x_elements();

  while(current != NULL)
  {
    current->cumulative_fraction = (double)index / (double)count;

    if (debug_flag)
      printf("Item:%lu CF:%f NF:%f x:%f\n",
             index,
             current->cumulative_fraction,
             current->normal_form,
             current->data_x);

    index++;
    current = current->next;
  }

  return;
}

/**************************************************************************
*
* Function:    calculate_variance
*
* Description: Performs a summation equation for variance.
*
* Parameters:  xavg - average of all x entries in linked list.
*
* Globals:     none
*
* Locals:      head - start of the linked list
*
* Return:      result - variance of the elements.
*
**************************************************************************/
double calculate_variance(double xavg)
{
  double sigma;
  double result;
  ELEMENT *current;
  COUNTER count;
  
  sigma = 0.0;
  result = 0.0;
  current = head;
  count = 0;

  /* FORMULA:   
              n
             ---             2
             \   (x   - x   )
             /     i     avg
             --- 
             i=1
            ------------------
                  (n - 1)
  */

  while(current != NULL)
  {
    sigma += pow((current->data_x - xavg),2);
    count++;
    current = current->next;
  }

  if (count > 1)
    result = sigma / (double)(count - 1);

  return result;
}

/**************************************************************************
*
* Function:    sort_data_x_elements
*
* Description: Sorts the data x entries in a linked list.
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
void sort_data_x_elements(void)
{
  ELEMENT *prev = {NULL};
  ELEMENT *current = {NULL};
  ELEMENT *next = {NULL};
  unsigned char finished;
  
  finished = FALSE;

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
      if (next->data_x < current->data_x)
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
  COUNTER index = 0;
  /*
  **if (debug_flag)
  **  printf("Reading file...\n");
  */
  
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
      temp_node->data_x = atof(str1);
      str2 = strtok(NULL,"\n");
      temp_node->data_y = atof(str2);
  
      /* attach element to list */
      add_element(temp_node);
  
      /* DEBUG */
      if (debug_flag)
      {
        printf("Number 1 = %s (%f)\tNumber 2 = %s (%f)\t#%lu\n",
                str1,temp_node->data_x,
                str2,temp_node->data_y,
                index);
      }

      index++;      
    } /* end of non-comment line */
  } /* end of getting string from file */
  
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
  printf("This program reads in a list of numbers from a file,\n");
  printf("and performs a chi squared test on them.\n");
  printf("\n");
  printf("Place the data in a file with two columns x and y seperated\n");
  printf("by a comma.  The program will only use the x column.\n");
  printf("\n");
  printf("SYNTAX:\n");
  if (name != NULL)
    printf("%s filename [-d]\n",name);
  else
    printf("CHITEST filename [-d]\n");
  printf("  filename  name of the file with the data.\n");
  printf("  -d        turn on debug.\n");
  printf("\n");

  return;
}

/**************************************************************************
*
* Function:    simpson_integrate_chi
*
* Description: Numerically integtrates a chi distribtion function based on 
*              Simpson's Rules.
*              
* Parameters:  val - chi squared distribution value to integrate to
*              dof - degrees of freedom
*
* Globals:     none
*
* Locals:      none
*
* Return:      probability - valuation of the integration process.
*
**************************************************************************/
double simpson_integrate_chi(double val,double dof)
{
  unsigned char continue_flag;  /* flag to encourage integration */
  COUNTER segments;   /* number of iterations */
  double old_result;  /* previous answer */
  double result;      /* sum of terms */
  double max_error;   /* error must be less than this number to be ok */
  double x_low;       /* integration limit */
  double x_high;      /* integration limit. */

  double probability = 0.0; /* alpha value that gets return */
  double width = 0.0;       /* width of each segment */
  COUNTER n = 0;            /* segment index */
  double term = 0.0;        /* answer from integrating */
  double error = 0.0;       /* difference of this result from past results */
  double x_i = 0.0;         /* number that the function will be evaluated at */
 
  /* Initialize */
  continue_flag = TRUE;
  segments = 20;
  old_result = 0.0;
  result = 0.0;
  max_error = 0.000001;
  x_low = 0.0;
  
  /* Determine Input for Integration */
  /* No adjustment necessary for Chi square - always positive input */
  x_high = val;

  /* Integrate until the error is smaller than max_error */
  while (continue_flag)
  {
    width = x_high / (double)segments;

    for (n=0; n <= segments; n++)
    {
      x_i = (double)n * width;

      /* first segment */
      if (n == 0)
        term = chi_distribution_function(x_i,dof) * (width / 3.0);
      /* last segment */
      else if (n == segments)
        term = chi_distribution_function(x_i,dof) * (width / 3.0);
      /* even segments */
      else if ( (n%2) == 0 )
        term = 2.0 * chi_distribution_function(x_i,dof) * (width / 3.0);
      else
        term = 4.0 * chi_distribution_function(x_i,dof) * (width / 3.0);

      /*
      **if (debug_flag)
      **  printf("term:%f\n",term);
      */

      result += term;
      
    } /* end of segment loop integration */

    /* calculate absolute value of the error */
    error = result - old_result;
    if (error < 0.0)
      error = error * (-1.0);

    /*
    **if (debug_flag)
    **  printf("segments:%lu error:%f total:%f\n",segments,error,result);
    */

    /* determine if the answer is good enough */
    if (error > max_error)
    {
      old_result = result;
      segments = segments * 2;
      result = 0.0;
    }       
    else
      continue_flag = FALSE;
  } /* end of integrate while error is large */

  /* Result for Chi Square does not need modified using Simpson's Rule */
  probability = result;
  
  return probability;
}

/**************************************************************************
*
* Function:    chi_distribution_function
*
* Description: Computes the result of a chi distribution function.
*              
* Parameters:  x - value to compute to.
*              dof - degrees of freedom - double instead of 
*                    DOF typedef for ease of use in calculations
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - computed result of chi distribution.
*
**************************************************************************/
double chi_distribution_function(double x,double dof)
{
  double exponential = 0.0;
  double exponent = 0.0;
  double gamma = 0.0;
  double lower = 0.0;
  double value = 0.0;
  double result = 0.0;

  /* FORMULA FOR CHI SQUARED DISTRIBUTION:   

                         
                                           
                                             -x /2
                         1          (n/2)-1    i   
           F(x ) = -------------- x         e   
              i     (n/2)          i    n
                   2      C(n/2)
                 
     where C = gamma function
           n = number of degrees of freedom = number of data points - 2
  */

  if (dof > 0.0)
  {

    gamma = gamma_n_over_2(dof);
    lower = pow(2.0 , dof / 2.0);
  
    value = pow(x , (dof / 2.0) - 1.0);

    exponent = (-1.0 * x) / 2.0;
    exponential = exp(exponent);
  
    result = (1.0 / ( gamma * lower)) * value * exponential;

    /*
    **if (debug_flag)
    **  printf("x:%f value:%f exp:%f result:%f ",
    **          x,value,exponential,result);
    */
  }

  return result;
}

/**************************************************************************
*
* Function:    gamma_n_over_2
*
* Description: Calculates the gamma value for positive n/2 using a
*              factorial equation.
*
* Parameters:  dof - degrees of freedom.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - gamma value.
*
**************************************************************************/
double gamma_n_over_2(DOF dof)
{
  double result = {1.0};

  dof--;
  dof--;

  while (dof > 0)
  {
    result *= ((double)dof / 2.0);
    dof--;
    dof--;
  }

  /* for odd DOF, add this */
  if (dof < 0)
    result *= sqrt(PI);
  
  return result;
}

/**************************************************************************
*
* Function:    simpson_integrate_normal
*
* Description: Numerically integtrates a normal distribution funtion based 
*              on Simpson's Rules.
*              
* Parameters:  n_val - normal distribution value to integrate to
*              count - number of data points
*
* Globals:     none
*
* Locals:      none
*
* Return:      probability - valuation of the integration process.
*
**************************************************************************/
double simpson_integrate_normal(double n_val,COUNTER count)
{
  unsigned char continue_flag;  /* flag to encourage integration */
  COUNTER segments;   /* number of iterations */
  double old_result;  /* previous answer */
  double result;      /* sum of terms */
  double max_error;   /* error must be less than this number to be ok */
  double x_low;       /* integration limit */
  double probability; /* alpha value that gets return */

  double x_high = 0.0;      /* integration limit. */
  double x_i = 0.0;         /* number that the function will be evaluated at */
  double error = 0.0;       /* difference of this result from past results */
  COUNTER n = 0;            /* segment index */
  double width = 0.0;       /* width of each segment */
  double term = 0.0;        /* answer from integrating */

  continue_flag = TRUE;
  segments = 20;
  old_result = 0.0;
  result = 0.0;
  probability = 0.0;
  max_error = 0.000001;
  x_low = 0.0;
  
  /* Determine Input for Integration */
  /* No adjustment necessary for positive input */
  if (n_val > 0.0)
    x_high = n_val;
  /* Keep the input positive and adjust after integration */
  else if (n_val < 0.0)
    x_high = n_val * (-1.0);
  /* No need to integrate if input is 0.0 */
  else
    continue_flag = FALSE;

  /* Integrate until the error is smaller than max_error */
  while (continue_flag)
  {
    width = x_high / (double)segments;

    for (n=0; n <= segments; n++)
    {
      x_i = (double)n * width;

      /* first segment */
      if (n == 0)
        term = normal_distribution_function(x_i) * (width / 3.0);
      /* last segment */
      else if (n == segments)
        term = normal_distribution_function(x_i) * (width / 3.0);
      /* even segments */
      else if ( (n%2) == 0 )
        term = 2.0 * normal_distribution_function(x_i) * (width / 3.0);
      else
        term = 4.0 * normal_distribution_function(x_i) * (width / 3.0);

      result += term;
      
    } /* end of segment loop integration */

    /* calculate absolute value of the error */
    error = result - old_result;
    if (error < 0.0)
      error = error * (-1.0);

    /* determine if the answer is good enough */
    if (error > max_error)
    {
      old_result = result;
      segments = segments * 2;
      result = 0.0;
    }       
    else
      continue_flag = FALSE;
  } /* end of integrate while error is large */

  /* Modify result using Simpson's Rule */
  if (n_val > 0.0)
    probability = 0.5 + result;
  else if (n_val < 0.0)
    probability = 0.5 - result;
  else
    probability = 0.5;

  return probability;
}

/**************************************************************************
*
* Function:    normal_distribution_function
*
* Description: Computes the result of a normal distribution function.
*              
* Parameters:  x - value to compute to.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - computed result of normal distribution.
*
**************************************************************************/
double normal_distribution_function(double x)
{
  double exponent = 0.0;
  double exponential = 0.0;
  double result = 0.0;

  /* FORMULA FOR NORMAL DISTRIBUTION:   
                                   1    2
                                - --- x 
                        1          2   i
           F(x ) = ----------- e 
              i     +--------
                   \|  2 PI
                 
  */
  exponent = -1.0 * ( (x * x) / 2.0 );
  exponential = exp(exponent);
  result = 0.39894 * exponential;

  return result;
}

/**************************************************************************
*
* Function:    find_nval
*
* Description: Calculates an n distribution value by integrating from 0 to
*              a trial valu of n.  The correct value of n is determined by
*              successively adjusting the trial value of n up or down until
*              the probability value is within an acceptable error.
*
* Parameters:  probability - value of probability we want.
*              count - number to calculate the degrees of freedom.
*
* Globals:     none
*
* Locals:      none
*
* Return:      n_guess - n distribution value.
*
**************************************************************************/
double find_nval(double probability,COUNTER count)
{
  unsigned char continue_flag;  /* flag to encourage more guessing */
  double result;      /* sum of terms */
  double max_error;   /* error must be less than this number to be ok */
  double low_n;       /* lowest n guess allowed */
  double high_n;      /* highest n guess allowed */
  double n_guess;     /* n distribution guess */
  double error = 0.0; /* difference of this result from needed number */
 
  continue_flag = TRUE;
  result = 0.0;
  max_error = 0.00001;
  low_n = -4.0;      /* number from chart in appendix A */
  high_n = 4.0;      /* number from chart in appendix A */

  /* middle guess */
  n_guess = low_n + ((high_n - low_n)/2.0);       
  
  /* Guess until the error is smaller than max_error */
  while (continue_flag)
  {
    result = simpson_integrate_normal(n_guess,count);

    /* calculate absolute value of the error */
    error = result - probability;
    if (error < 0.0)
      error = error * (-1.0);

    /* determine if the answer is good enough */
    if (error > max_error)
    {
      /* Result is too big - guess lower */
      if ((result - probability) > 0.0)
      {
        /* adjust maximum */
        high_n = n_guess;
        /* pick a number in the middle */
        n_guess = low_n + ((high_n - low_n)/2.0);       
      }

      /* Result is too small - guess higher */
      else
      {
        /* adjust minimum */
        low_n = n_guess;
        /* pick a number in the middle */
        n_guess = low_n + ((high_n - low_n)/2.0);       
      }
    }       

    /* Answer is within acceptable error range - stop */
    else
      continue_flag = FALSE;

    /* if we are at our boundary, then stop */
    if ((n_guess == high_n) || (n_guess == low_n))
      continue_flag = FALSE;

    /*
    **  if (debug_flag)
    **printf("Low:%f High:%f result:%f error:%f guess:%f\n",
    **        low_n,high_n,result,error,n_guess);
    */
  } /* end of integrate while error is large */

  return n_guess;
}

/**************************************************************************
*
* Function:    determine_maximum_segments
*
* Description: Calculates the max segments for a linked list of data.
*
* Parameters:  count - number of data elements
*
* Globals:     none
*
* Locals:      head - start of the linked list
*
* Return:      none
*
**************************************************************************/
COUNTER determine_maximum_segments(COUNTER count)
{
  COUNTER s = 0;

  s = count / 5;

  /* Adjust if necessary - per rules */
  if (s <= 3)
    s = 4;
  else if ( (s*s) < count)
    s = sqrt((double)count);

  return s;
}

/**************************************************************************
*
* Function:    determine_maximum_segments
*
* Description: Calculates the max segments for a linked list of data.
*
* Parameters:  count - number of data elements
*
* Globals:     none
*
* Locals:      head - start of the linked list
*
* Return:      none
*
**************************************************************************/
double calculate_q_value(COUNTER segments,COUNTER count)
{
  COUNTER n = 0;
  double increment = 0.0;
  double probability = 0.0;
  double low = 0.0;
  double high = 0.0;
  double q = 0.0;
  double term = 0.0;
  COUNTER x = 0;
  COUNTER k = 0;
  ELEMENT *current = NULL;

  /* Initialize */
  n = count / segments;
  increment = 1.0 / (double)segments;
  probability = increment;
  low = -DBL_MAX;
  high = find_nval(probability,count);
  q = 0.0;

  /* Visit each segment to find out how many 
     values exist in each segment */
  for (x = 0;x < segments;x++)
  {
    /* Initialize */
    k = 0;
    current = head;

    /* Loop through list */
    while(current != NULL)
    {
      /* find values that are within this segment */
      if ((current->normal_form >= low) &&
          (current->normal_form <  high))
        k++;
      current = current->next;
    }

    /* Calculate Q */
    term = (double)((n - k) * (n - k)) / (double)n;
    q += term;

    if (debug_flag)
      printf("n:%lu k:%lu low:%f high:%f term:%f q:%f\n",
              n,k,low,high,term,q);

    /* Set up the next segment boundaries */
    low = high;
    probability += increment;
    if (x == (segments-1))
      high = DBL_MAX;
    else
      high = find_nval(probability,count);
  }
  return q;
}

/**************************************************************************
*
* Function:    print_results
*
* Description: Prints the results of the chi squared test
*
* Parameters:  q - the Q value
*              p - probability of Q
*              s - number of segments
*              n - number of elements
*
* Globals:     none
*
* Locals:      none
*
* Return:      none
*
**************************************************************************/
void print_results(double q,double p,COUNTER s,COUNTER n)
{
  double tail = 0.0;

  tail = 1.0 - p;

  printf("Q = %f\n",q);
  printf("P = %f\n",p);
  printf("S = %lu\n",s);
  printf("n = %lu\n",n);
  printf("tail = %f\n",tail);

  if (tail < 0.050)
    printf("Fit rejected! (tail < 0.050)\n");
  else if (tail > 0.20)
    printf("Fit accepted! (tail > 0.20)\n");
  else
    printf("Intermediate fit! (tail between 0.20 and 0.050)\n");

  return;
}

