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
* Description: Reads in a list of number pairs from a file, and performs
*              the the linear regression calculation for those numbers.
*              Then it determines the 90% an 70% prediction intervals
*              and produces an estimate for a number that has been
*              passed in.  The output is the range for the estimate
*              using the UPI and LPI found earlier.
*
* History: 1: 07-Jan-1997: Created File - Steve Karg
*          2: 17-Jan-1997: Added ability to comment data file with ;
*                          in first column. - Steve Karg
*          3: 10-Feb-1997: Converted file linreg.c to estimate.c. - Steve Karg
*          4: 26-Feb-1997: Updated the gamma function to be a better
*                          approximation for gamma(n/2). - Steve Karg
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
#define TRUE  (1)
#define FALSE (0)
#define PI    (3.1415926535898)

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
char debug_string[256];

/* set up counter type */
typedef unsigned long int COUNTER;

/* set up degree of freedom type */
typedef long int DOF;

/* FUNCTION PROTOTYPES */
ELEMENT *create_list_element(void);
void add_element(ELEMENT *e);
void delete_elements(void);
FILE *open_input_file(char *filename);
void load_file_into_list(FILE *file_ptr);
void usage(char *filename);

COUNTER count_entries(void);
double xsum_entries(void);
double ysum_entries(void);
double upper_sigma_entries(double xmean,double ymean,COUNTER count);
double lower_sigma_entries(double xmean,COUNTER count);
double calculate_variance(double beta0,double beta1);
COUNTER calculate_linear_regression(double *beta0,double *beta1);
double simpson_integrate(double t_val,COUNTER count);
double t_distribution_function(double x,COUNTER count);
double gamma_n_over_2(DOF dof);
double sigma_xi_xavg_sqr_entries(double xmean);
double calculate_range(double stddev,double tval,double est,COUNTER count);
double find_tval(double probability,COUNTER count);

/**************************************************************************
*
* Function:    main   
*
* Description: Main task for prediction interval program.
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
  char filename[256];  /*input filename */
  FILE *file_ptr;      /* file stream */

  COUNTER count;       /* number of entries */
  double beta0;        /* first linear regression parameter */
  double beta1;        /* second linear regression parameter */
  double estimate;     /* user entered parameter */
  double variance;     /* variance of the data entries */
  double stddev;       /* square root of the variance */
  double tval70;       /* 70 percent t distribution value */
  double tval90;       /* 90 percent t distribution value */
  double range70;      /* 70 percent estimate range */
  double range90;      /* 90 percent estimate range */
  double code_size;    /* expected LOC size using lin. reg. values */

  debug_flag = FALSE;

  /* Decode the user input from the command line */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));
    if (argc > 2)
    {
      estimate = atof(*(argv+2));

      if (argc > 3)
      {
        strcpy(debug_string,*(argv+3));
        if (debug_string[0] == 'd')
          debug_flag = TRUE;
      }
    }
  }
  else
  {
    usage(*argv);
    return 1;
  }

  /* open the file */
  file_ptr = open_input_file(filename);
  if (file_ptr == NULL)
    return 0;

  /* load the file into linked list */
  load_file_into_list(file_ptr);

  count = calculate_linear_regression(&beta0,&beta1);
  /* Expected value based on past history */
  code_size = beta0 + beta1 * estimate;

  /* Calculate the variance and standard deviation */
  variance = calculate_variance(beta0,beta1);
  if (variance > 0.0)
    stddev = sqrt(variance);
  else
    stddev = 0.0;

  /* Calculate the t-distribution t value for 70% using the single
     sided value 85% */
  tval70 = find_tval(0.85,count);

  /* Calculate the t-distribution t value for 90% using the single
     sided value 95% */
  tval90 = find_tval(0.95,count);

  /* Calculate the range */
  range70 = calculate_range(stddev,tval70,estimate,count);
  range90 = calculate_range(stddev,tval90,estimate,count);

  /* Print the results */
  printf("Number of entries=%lu\n",count);
  printf("B0=%f\n",beta0);
  printf("B1=%f\n",beta1);
  printf("Variance = %8.4f\n",variance);
  printf("Standard Deviation = %8.4f\n",stddev);
  printf("t(70 percent) = %5.3f\n",tval70);
  printf("t(90 percent) = %5.3f\n",tval90);
  printf("Adjusted Estimate: %10.1f\n",code_size);
  printf("Prediction: 70 percent 90 percent\n");
  printf("            ========== ==========\n");
  printf("Range       %10.1f %10.1f\n",range70,range90);
  printf("UPI         %10.1f %10.1f\n",(code_size+range70),
                                       (code_size+range90));
  printf("LPI         %10.1f %10.1f\n",(code_size-range70),
                                       (code_size-range90));

  /* House Keeping */
  delete_elements();

  return 1;
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
void delete_elements(void)
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
double ysum_entries(void)
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
double upper_sigma_entries(double xmean,double ymean,COUNTER count)
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
double lower_sigma_entries(double xmean,COUNTER count)
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
      sscanf(line,"%lf , %lf",&temp_node->data[0],&temp_node->data[1]);

      /* attach element to list */
      add_element(temp_node);

      if (debug_flag)
      {
        printf("Line:%s\tNumber 1:%f\tNumber 2:%f\n",
                line,temp_node->data[0],temp_node->data[1]);
      } /* end of debug */
    } /* end of non-comment line */
  } /* end of getting string from file */

  /* close the file */
  fclose(file_ptr);
}

/**************************************************************************
*
* Function:    usage
*
* Description: Prints a help message to the user about how to use this
*              program.
*
* Parameters:  filename - name entered to get this program to run.
*
* Globals:     none
*
* Locals:      none
*
* Return:      none
*
**************************************************************************/
void usage(char *filename)
{
  char *str;
  char *name;

  /* Find the last token - that is the actual filename */
  str = strtok(filename,":\\");
  while (str != NULL)
  {
    name = str;
    str = strtok(NULL,":\\");
  }

  printf("Prediction Interval for Size Estimates\n");
  printf("written by Steve Karg\n");
  printf("10-Feb-1997\n");
  printf("\n");
  printf("This program computes the prediction interval size estimating\n");
  printf("parameters for a set of n programs where historical function\n");
  printf("LOC and new and changed LOC data are available.\n");
  printf("\n");
  printf("Place the data in a file with two columns x and y seperated\n");
  printf("by a comma.\n");
  printf("\n");
  printf("Usage:\n");
  if (name != NULL)
    printf("      %s filename estimate [[d]ebug]\n",name);
  else
    printf("      ESTIMATE filename estimate [[d]ebug]\n");

    return;
}

/**************************************************************************
*
* Function:    calculate_variance
*
* Description: Performs a summation equation for variance.
*
* Parameters:  count - number of entries.
*              beta0 - the first beta parameter from linear regression.
*              beta0 - the second beta parameter.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - variance of the elements.
*
**************************************************************************/
double calculate_variance(double beta0,double beta1)
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
                1      ---                  2
             (-----) * \   (y   - B  - B x )
              n - 2    /     i     0    1 i
                       --- 
                       i=1
  */

  while(current != NULL)
  {
    sigma += (
              current->data[1] -
              beta0 -
              (beta1 * (current->data[0]))
             )
             *
             (
              current->data[1] -
              beta0 -
              (beta1 * (current->data[0]))
             );
    count++;
    current = current->next;
  }

  if (count > 2)
    result = sigma/(double)(count - 2);

  return result;
}

/**************************************************************************
*
* Function:    calculate_linear_regression
*
* Description: Performs a linear regression calculation and produces the
*              beta 0 and beta 1 parameters.
*
* Parameters:  beta0 - the first beta parameter from linear regression.
*              beta1 - the second beta parameter.
*
* Globals:     none
*
* Locals:      none
*
* Return:      count - number of entries
*              
**************************************************************************/
COUNTER calculate_linear_regression(double *beta0,double *beta1)
{
  COUNTER count;       /* number of entries */
  double xsum;         /* sum of x entries */
  double ysum;         /* sum of y entries */
  double xmean;        /* average of the x entries */
  double ymean;        /* average of the y entries */
  double upper_sigma;  /* top summation - see formula */
  double lower_sigma;  /* bottom summation - see formula */

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
    *beta1 = 0.0;
  else
    *beta1 = upper_sigma / lower_sigma;

  /* LINEAR REGRESSION FORMULA 2:

           beta0 = y    - beta1 x
                    avg          avg
  */
  *beta0 = ymean - ((*beta1) * xmean);

  return count;
}

/**************************************************************************
*
* Function:    simpson_integrate
*
* Description: Numerically integtrates a funtion based on Simpson's Rules.
*              
* Parameters:  t_val - t distribution value to integrate to
*              count - number of data points
*
* Globals:     none
*
* Locals:      none
*
* Return:      probability - valuation of the integration process.
*
**************************************************************************/
double simpson_integrate(double t_val,COUNTER count)
{
  unsigned char continue_flag;  /* flag to encourage integration */
  COUNTER segments;   /* number of iterations */
  COUNTER n;          /* segment index */
  double width;       /* width of each segment */
  double old_result;  /* previous answer */
  double term;        /* answer from integrating */
  double result;      /* sum of terms */
  double error;       /* difference of this result from past results */
  double max_error;   /* error must be less than this number to be ok */
  double x_i;         /* number that the function will be evaluated at */
  double x_high;      /* integration limit. */
  double probability; /* alpha value that gets return */

  continue_flag = TRUE;
  segments = 20;
  old_result = 0.0;
  result = 0.0;
  max_error = 0.000001;

  /* Determine Input for Integration */
  /* No adjustment necessary for positive input */
  if (t_val > 0.0)
    x_high = t_val;
  /* Keep the input positive and adjust after integration */
  else if (t_val < 0.0)
    x_high = t_val * (-1.0);
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
        term = t_distribution_function(x_i,count) * (width / 3.0);
      /* last segment */
      else if (n == segments)
        term = t_distribution_function(x_i,count) * (width / 3.0);
      /* even segments */
      else if ( (n%2) == 0 )
        term = 2.0 * t_distribution_function(x_i,count) * (width / 3.0);
      else
        term = 4.0 * t_distribution_function(x_i,count) * (width / 3.0);

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
  if (t_val > 0.0)
    probability = 0.5 + result;
  else if (t_val < 0.0)
    probability = 0.5 - result;
  else
    probability = 0.5;

  return probability;
}

/**************************************************************************
*
* Function:    t_distribution_function
*
* Description: Computes the result of a t distribution function.
*              
* Parameters:  x - value to compute to.
*              count - number of data points
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - computed result of t distribution.
*
**************************************************************************/
double t_distribution_function(double x,COUNTER count)
{
  double exponent;
  double gamma_upper;
  double gamma_lower;
  double sqrt_lower;
  double value;
  double endterm;
  double dof;        /* degrees of freedom */
  double result;

  /* FORMULA FOR t DISTRIBUTION:   

                         
                      n+1                2
                    C(---)             x 
                       2                i  -(n+1)/2
           F(x ) = -------------- (1 + ---)
              i     +-----              n
                   \|n PI  C(n/2)

     where C = gamma function
           n = number of degrees of freedom = number of data points - 2
  */

  result = 0.0;
  dof = (double)(count - 2);

  if (dof > 0.0)
  {

    gamma_upper = gamma_n_over_2(dof + 1.0);
    gamma_lower = gamma_n_over_2(dof);

    sqrt_lower = sqrt(dof * PI );
    value = (1.0 + ( ( x * x ) / dof ) );
    exponent = -1.0 * ( (dof + 1.0 ) / 2.0 );
  
    endterm = pow(value,exponent);

    result = ( gamma_upper / ( sqrt_lower * gamma_lower ) ) * endterm;
  }

  return result;
}

/**************************************************************************
*
* Function:    calculate_range
*
* Description: Performs a range equation.
*
* Parameters:  stddev - the standard deviation of the entries.
*              tval - the t distribution value.
*              est - user input line of code estimate.
*              count - number of entries.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - range for the estimation based on inputs.
*
**************************************************************************/
double calculate_range(double stddev,double tval,double est,COUNTER count)
{
  double xsum;
  double xmean;
  double root;
  double sigma;
  double under_sqrt;
  double result = {0.0};

  /* FORMULA FOR RANGE:
                      ----------------------------
                     |                      2
                     |           (x  - x   )
                     |      1      k    avg
                     | 1 + --- + ----------------
                     |      n      n
                 \   |           ---            2
     t(a/2,n-2)s  \  |           \   (x  - x   )
                   \ |           /     i    avg
                    \|           ---
                                 i=1
  */


  /* get summation of x entries */
  xsum = xsum_entries();

  /* protect against divide by zero */
  if (count == 0)
    return 0.0;

  /* calculate the average */
  xmean = xsum / count;

  sigma = sigma_xi_xavg_sqr_entries(xmean);

  /* protect against divide by zero */
  if (sigma == 0.0)
    return 0.0;

  under_sqrt = 1.0 + (1.0/(double)count) +
               ( ((est-xmean)*(est-xmean)) / (sigma) );

  /* protect against square root of negative number */
  if (under_sqrt < 0.0)
    return 0.0;

  root = sqrt(under_sqrt);

  result = tval * stddev * root;

  if (debug_flag)
  {
    printf("Calc Range: xsum=%5.3f count=%lu xmean=%5.3f sigma=%5.3f\n",
            xsum,count,xmean,sigma);
    printf("under sqrt=%5.3f root=%5.3f tval=%5.3f stddev=%5.3f est=%4.1f\n",
            under_sqrt,root,tval,stddev,est);
  }

  return result;
}

/**************************************************************************
*
* Function:    sigma_xi_xavg_sqr_entries
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
* Return:      sigma - sum of equations for sigma.
*
**************************************************************************/
double sigma_xi_xavg_sqr_entries(double xmean)
{
  double sigma;
  ELEMENT *current;

  sigma = 0.0;
  current = head;

  while(current != NULL)
  {
    /* FORMULA:   
                        n
                       ---            2
                       \   (x  - x   )    
                       /     i    avg
                       --- 
                       i=1
    */
    sigma += pow(((current->data[0]) - xmean),2);
    current = current->next;
  }
  return sigma;
}

/**************************************************************************
*
* Function:    find_tval
*
* Description: Calculates a t distribution value by integrating from 0 to
*              a trial valu of t.  The correct value of t is determined by
*              successively adjusting the trial value of t up or down until
*              the probability value is within an acceptable error.
*
* Parameters:  probability - value of probability we want.
*              count - number to calculate the degrees of freedom.
*
* Globals:     none
*
* Locals:      none
*
* Return:      t_guess - t distribution value.
*
**************************************************************************/
double find_tval(double probability,COUNTER count)
{
  unsigned char continue_flag;  /* flag to encourage more guessing */
  double t_guess;     /* t distribution guess */
  double result;      /* sum of terms */
  double error;       /* difference of this result from needed number */
  double max_error;   /* error must be less than this number to be ok */
  double low_t;       /* lowest t guess allowed */
  double high_t;      /* highest t guess allowed */

  continue_flag = TRUE;
  result = 0.0;
  max_error = 0.00001;
  low_t = 0.253;       /* number from chart in appendix A */
  high_t = 63.657;     /* number from chart in appendix A */

  /* middle guess */
  t_guess = low_t + ((high_t - low_t)/2.0);       
  
  /* Guess until the error is smaller than max_error */
  while (continue_flag)
  {
    result = simpson_integrate(t_guess,count);

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
        high_t = t_guess;
        /* pick a number in the middle */
        t_guess = low_t + ((high_t - low_t)/2.0);       
      }

      /* Result is too small - guess higher */
      else
      {
        /* adjust minimum */
        low_t = t_guess;
        /* pick a number in the middle */
        t_guess = low_t + ((high_t - low_t)/2.0);       
      }
    }       
    else
      continue_flag = FALSE;
  } /* end of integrate while error is large */

  return t_guess;
}

/**************************************************************************
*
* Function:    gamma
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
