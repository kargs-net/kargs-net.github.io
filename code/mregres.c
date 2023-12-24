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
*              the the multiple regression calculation for those numbers.
*              Then it determines the 90% an 70% prediction intervals
*              and produces an estimate for the estimates that have been
*              passed in.  The output is the range for the estimates
*              using the UPI and LPI found earlier.
*
* History: 1: 07-Jan-1997: Created File - Steve Karg
*          2: 17-Jan-1997: Added ability to comment data file with ;
*                          in first column. - Steve Karg
*          3: 10-Feb-1997: Converted file linreg.c to estimate.c. - Steve Karg
*          4: 26-Feb-1997: Updated the gamma function to be a better
*                          approximation for gamma(n/2). - Steve Karg
*          5: 27-Jul-1997: Changed name from estimate.c to mregres.c
*                          and made it into the multiple regression
*                          calculator. - Steve Karg
*
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* LOCAL CONTSTANTS */
//#define DEBUG

#define MAX_LINE_SIZE (80)
#define TRUE  (1)
#define FALSE (0)
#define PI    (3.1415926535898)

typedef enum
{
  W_DATA = 0,
  X_DATA = 1,
  Y_DATA = 2,
  Z_DATA = 3,
  MAX_DATA = 4
} DATA_INDEX;

/* structure for linked list */
typedef struct statistics
{
  double data[MAX_DATA];
  struct statistics *next;
} STATISTICS;

/* set up generic element */
typedef struct statistics ELEMENT;

/* set up the start of the linked list */
static ELEMENT *head;

/* set up counter type */
typedef unsigned long int COUNTER;

/* set up degree of freedom type */
typedef long int DOF;

/* FUNCTION PROTOTYPES */
void usage(char *filename);

/* linked list core functions */
ELEMENT *create_list_element(void);
void add_element(ELEMENT *e);
void delete_elements(void);

/* file and loading functions */
FILE *open_input_file(char *filename);
void load_file_into_list(FILE *file_ptr);

/* calculation functions */
void gaussian_method(double array[4][5],double beta[4]);

COUNTER calculate_multiple_regression(double beta[4]);

COUNTER count_entries(void);
double sum_entries(DATA_INDEX index);
double multiply_sum_entries(DATA_INDEX index1,DATA_INDEX index2);
double square_sum_entries(DATA_INDEX index);

double calculate_variance(double beta[4]);

double find_tval(double probability,COUNTER count);
double simpson_integrate(double t_val,COUNTER count);
double t_distribution_function(double x,COUNTER count);
double gamma_n_over_2(DOF dof);

double calculate_range(double stddev,double tval,
  double est[MAX_DATA],COUNTER count);
double sigma_i_avg_sqr_entries(double mean,DATA_INDEX index);


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
  char filename[256] = {""};  /*input filename */
  FILE *file_ptr = NULL;      /* file stream */

  COUNTER count = 0;       /* number of entries */
  double beta[4] = {0.0,0.0,0.0,0.0}; /* return from gauss's */
  double variance = 0.0;     /* variance of the data entries */
  double stddev = 0.0;       /* square root of the variance */
  double tval70 = 0.0;       /* 70 percent t distribution value */
  double tval90 = 0.0;       /* 90 percent t distribution value */
  double range70 = 0.0;      /* 70 percent estimate range */
  double range90 = 0.0;      /* 90 percent estimate range */
  double code_size = 0.0;    /* expected LOC size using lin. reg. values */
  double estimates[MAX_DATA] = {0.0,0.0,0.0,0.0}; /* user estimates */

  /* Decode the user input from the command line */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));
    if (argc > (1+3))
    {
      estimates[W_DATA] = atof(*(argv+(1+1)));
      estimates[X_DATA] = atof(*(argv+(1+2)));
      estimates[Y_DATA] = atof(*(argv+(1+3)));
    }
  }
  else
  {
    usage(*argv);
    return (1);
  }

  /* open the file */
  file_ptr = open_input_file(filename);
  if (file_ptr == NULL)
    return (0);

  /* load the file into linked list */
  load_file_into_list(file_ptr);

  count = calculate_multiple_regression(beta);

  #ifdef DEBUG
  printf("Estimate[W]       = %f\n",estimates[W_DATA]);
  printf("Estimate[X]       = %f\n",estimates[X_DATA]);
  printf("Estimate[Y]       = %f\n",estimates[Y_DATA]);
  #endif

  /* Expected value based on past history */
  code_size = beta[0] +
    (beta[1] * estimates[W_DATA]) +
    (beta[2] * estimates[X_DATA]) +
    (beta[3] * estimates[Y_DATA]);

  /* Calculate the variance and standard deviation */
  variance = calculate_variance(beta);
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
  range70 = calculate_range(stddev,tval70,estimates,count);
  range90 = calculate_range(stddev,tval90,estimates,count);

  /* Print the results */
  printf("Number of entries  = %lu\n",count);
  printf("B0                 = %f\n",beta[0]);
  printf("B1                 = %f\n",beta[1]);
  printf("B2                 = %f\n",beta[2]);
  printf("B3                 = %f\n",beta[3]);
  printf("Variance           = %8.4f\n",variance);
  printf("Standard Deviation = %8.4f\n",stddev);
  printf("t(70 percent)      = %5.3f\n",tval70);
  printf("t(90 percent)      = %5.3f\n",tval90);
  printf("Code Size Estimate = %9.1f\n",code_size);
  printf("Range(70 percent)  = %9.3f, UPI = %9.3f, LPI = %9.3f\n",
         range70,(code_size+range70),(code_size-range70));
  printf("Range(90 percent)  = %9.3f, UPI = %9.3f, LPI = %9.3f\n",
         range90,(code_size+range90),(code_size-range90));



  /* House Keeping */
  delete_elements();

  return (1);
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
  ELEMENT *p = NULL; /* points to new memory */

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
*
* Return:      none
*
**************************************************************************/
void add_element(ELEMENT *e)
{
  ELEMENT *p = NULL; /* holder for list element */

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
*
* Return:      none
*
**************************************************************************/
void delete_elements(void)
{
  ELEMENT *current = head; /* holder for current element */
  ELEMENT *next = NULL;    /* holder for next element */

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
* Locals:      head (IN) start of the linked list.
*
* Return:      number of entries
*
**************************************************************************/
COUNTER count_entries(void)
{
  COUNTER count = 0;      /* number of elements */
  ELEMENT *current = head;/* holds current element */

  while(current != NULL)
  {
    count++;
    current = current->next;
  }
  return (count);
}

/**************************************************************************
*
* Function:    sum_entries
*
* Description: Performs a summation equation for the x data.
*
* Parameters:  index (IN) index of the operand
*
* Globals:     none
*
* Locals:      head (IN) start of the linked list.
*
* Return:      sum of entries
*
**************************************************************************/
double sum_entries(DATA_INDEX index)
{
  double sum = 0.0;          /* return value */
  ELEMENT *p_current = head; /* pointer to our element */

  while(p_current)
  {
    sum += p_current->data[index];
    p_current = p_current->next;
  }
  return (sum);
}

/**************************************************************************
*
* Function:    multiply_sum_entries
*
* Description: Performs a summation equation.
*
* Parameters:  index1 (IN) index of the 1st operand
*              index2 (IN) index of the 2nd operand
* Globals:     none
*
* Locals:      head (IN) start of the linked list.
*
* Return:      sum of equations for sigma.
*
**************************************************************************/
double multiply_sum_entries(DATA_INDEX index1,DATA_INDEX index2)
{
  double sum = 0.0;        /* return value */
  ELEMENT *p_current = head; /* pointer to our element */

  while(p_current)
  {
    sum += (p_current->data[index1] * p_current->data[index2]);
    p_current = p_current->next;
  }
  return (sum);
}

/**************************************************************************
*
* Function:    square_sum_entries
*
* Description: Performs a summation equation.
*
* Parameters:  index (IN) index of the operand
*
* Globals:     none
*
* Locals:      head (IN) start of the linked list.
*
* Return:      sum of equations for sigma.
*
**************************************************************************/
double square_sum_entries(DATA_INDEX index)
{
  double sum = 0.0;        /* return value */
  ELEMENT *p_current = head; /* pointer to our element */

  while(p_current)
  {
    sum += (p_current->data[index] * p_current->data[index]);
    p_current = p_current->next;
  }
  return (sum);
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
  FILE *fp = fopen(filename,"r");

  if (fp == NULL)
  {
    printf("open_input_file: error opening %s.\n",filename);
    exit(-1);
  }

  return (fp);
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
  char line[MAX_LINE_SIZE] = {""}; /* line to be read in from file */
  ELEMENT *temp_node = NULL;       /* temporary element */
  int valid = 0;                   /* return from sscanf */

  #ifdef DEBUG
    printf("Reading file...\n");
  #endif
  /* read the entries and load into linked list */
  while (fgets(line,MAX_LINE_SIZE,file_ptr) != NULL)
  {
    if ((line[0] != ';') &&
        (line[0] != '\n'))
    {
      /* create element */
      temp_node = create_list_element();

      /* Parse line into seperate numbers and load */
      valid = sscanf(line,"%lf , %lf, %lf, %lf",
        &temp_node->data[0],
        &temp_node->data[1],
        &temp_node->data[2],
        &temp_node->data[3]);

      if (valid != 4)
        free(temp_node);
      else
      {
        /* attach element to list */
        add_element(temp_node);

        #ifdef DEBUG
          printf("Line:%s\tNumber 1:%f\t2:%f\t3:%f\t4:%f\n",
                  line,temp_node->data[0],temp_node->data[1],
                  line,temp_node->data[2],temp_node->data[3]);
        #endif
      } /* end of valid sscanf */
    } /* end of non-comment line */
  } /* end of getting string from file */

  /* close the file */
  fclose(file_ptr);

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
* Locals:      head (IN) start of the linked list.
*
* Return:      result - variance of the elements.
*
**************************************************************************/
double calculate_variance(double beta[4])
{
  double sigma = 0.0; /* for intermediate calculation */
  double result = 0.0; /* return value */
  ELEMENT *current = head; /* element of list */
  COUNTER count = 0; /* counts number of elements */


  /* FORMULA:
                        n
                1      ---                                 2
             (-----) * \   (z   - B  - B w  - B x  - B y  )
              n - 4    /     i     0    1 i    2 i    3 i
                       ---
                       i=1
  */

  while(current != NULL)
  {
    sigma += (
              current->data[Z_DATA] -
              beta[0] -
              (beta[1] * (current->data[W_DATA]) )-
              (beta[2] * (current->data[X_DATA]) )-
              (beta[3] * (current->data[Y_DATA]) )
             )
             *
             (
              current->data[Z_DATA] -
              beta[0] -
              (beta[1] * (current->data[W_DATA]) )-
              (beta[2] * (current->data[X_DATA]) )-
              (beta[3] * (current->data[Y_DATA]) )
             );
    count++;
    current = current->next;
  }

  if (count > 4)
    result = sigma/(double)(count - 4);

  return (result);
}

/**************************************************************************
*
* Function:    calculate_multiple_regression
*
* Description: Performs a multiple regression calculation and produces the
*              beta 0, 1, 2, and 3 parameters.
*
* Parameters:  beta0 - the first beta parameter from linear regression.
*              beta1 - the second beta parameter.
*              beta2 - the third beta parameter.
*              beta3 - the fourth beta parameter.
*
* Globals:     none
*
* Locals:      none
*
* Return:      count - number of entries
*
**************************************************************************/
COUNTER calculate_multiple_regression(double Beta[4])
{
  COUNTER count = 0;         /* number of entries */

  double wsum = 0.0;         /* sum of w entries */
  double xsum = 0.0;         /* sum of x entries */
  double ysum = 0.0;         /* sum of y entries */
  double zsum = 0.0;         /* sum of z entries */

  double xysum = 0.0;         /* sum of x*y entries */
  double wxsum = 0.0;         /* sum of w*x entries */
  double wysum = 0.0;         /* sum of w*y entries */
  double wzsum = 0.0;         /* sum of w*z entries */
  double xzsum = 0.0;         /* sum of x*z entries */
  double yzsum = 0.0;         /* sum of y*z entries */

  double x_square_sum = 0.0;          /* sum of x squared entries */
  double y_square_sum = 0.0;          /* sum of x squared entries */
  double w_square_sum = 0.0;          /* sum of x squared entries */

  double array[4][5] =                /* holds simultaneous equations */
  {
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0}
  };

  /* MULTIPLE REGRESSION FORMULA:
                  n           n           n        n
                 ---         ---         ---      ---
     B0 n + B1   \   w  + B2 \   x  + B3 \   y  = \   z
                 /    i      /    i      /    i   /    i
                 ---         ---         ---      ---
                 i=1         i=1         i=1      i=1


         n          n           n             n          n
        ---        ---  2      ---           ---        ---
     B0 \   w + B1 \   w  + B2 \   w x  + B3 \   w y  = \   w z
        /    i     /    i      /    i i      /    i i   /    i i
        ---        ---         ---           ---        ---
        i=1        i=1         i=1           i=1        i=1


         n          n             n           n          n
        ---        ---           ---  2      ---        ---
     B0 \   x + B1 \   w x  + B2 \   x  + B3 \   x y  = \   x z
        /    i     /    i i      /    i      /    i i   /    i i
        ---        ---           ---         ---        ---
        i=1        i=1           i=1         i=1        i=1

         n          n             n             n        n
        ---        ---           ---           ---  2   ---
     B0 \   y + B1 \   w y  + B2 \   x y  + B3 \   y  = \   y z
        /    i     /    i i      /    i i      /    i   /    i i
        ---        ---           ---           ---      ---
        i=1        i=1           i=1           i=1      i=1

  */

  /* get n */
  count = count_entries();

  /* get w,x,y & z sums */
  wsum = sum_entries(W_DATA);
  xsum = sum_entries(X_DATA);
  ysum = sum_entries(Y_DATA);
  zsum = sum_entries(Z_DATA);


  /* get x*y, w*x, w*y w*z x*z and y*z sums */
  xysum = multiply_sum_entries(X_DATA,Y_DATA);
  wxsum = multiply_sum_entries(W_DATA,X_DATA);
  wysum = multiply_sum_entries(W_DATA,Y_DATA);
  wzsum = multiply_sum_entries(W_DATA,Z_DATA);
  xzsum = multiply_sum_entries(X_DATA,Z_DATA);
  yzsum = multiply_sum_entries(Y_DATA,Z_DATA);

  /* get sum of x, y, and w squared entries */
  x_square_sum = square_sum_entries(X_DATA);
  y_square_sum = square_sum_entries(Y_DATA);
  w_square_sum = square_sum_entries(W_DATA);

  /* Load our array to get B0,B1,B2 & B3 */
  /*   n   w   x   y   z
       w   w2  wx  wy  wz
       x   wx  x2  xy  xz
       y   wy  xy  y2  yz
  */
  array[0][0] = count;
  array[0][1] = wsum;
  array[0][2] = xsum;
  array[0][3] = ysum;
  array[0][4] = zsum;

  array[1][0] = wsum;
  array[1][1] = w_square_sum;
  array[1][2] = wxsum;
  array[1][3] = wysum;
  array[1][4] = wzsum;

  array[2][0] = xsum;
  array[2][1] = wxsum;
  array[2][2] = x_square_sum;
  array[2][3] = xysum;
  array[2][4] = xzsum;

  array[3][0] = ysum;
  array[3][1] = wysum;
  array[3][2] = xysum;
  array[3][3] = y_square_sum;
  array[3][4] = yzsum;

  /* solve the simultaneous equation */
  gaussian_method(array,Beta);

  return (count);
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
  unsigned char continue_flag = TRUE;  /* flag to encourage integration */
  COUNTER segments = 20;    /* number of iterations */
  COUNTER n = 0;            /* segment index */
  double width = 0.0;       /* width of each segment */
  double old_result = 0.0;  /* previous answer */
  double term = 0.0;        /* answer from integrating */
  double result = 0.0;      /* sum of terms */
  double error = 0.0;       /* difference of this result from past results */
  double max_error = 0.000001; /* error must be less than this to be ok */
  double x_i = 0.0;         /* number that the function will be evaluated at */
  double x_high = 0.0;      /* integration limit. */
  double probability = 0.0; /* alpha value that gets return */

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

  return (probability);
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
  double exponent;     /* exponent part of equation */
  double gamma_upper;  /* gamma in upper part of equation */
  double gamma_lower;  /* gamma in lower part of equation */
  double sqrt_lower;   /* part of equation */
  double value = 0.0;  /* part of end term in equation */
  double endterm = 0.0;/* part of equation */
  double dof = 0.0;    /* degrees of freedom */
  double result = 0.0; /* return value */

  /* FORMULA FOR t DISTRIBUTION:


                      n+1                2
                    C(---)             x
                       2                i  -(n+1)/2
           F(x ) = -------------- (1 + ---)
              i     +-----              n
                   \|n PI  C(n/2)

     where C = gamma function
           n = number of degrees of freedom = number of data points - 4
  */

  dof = (double)(count - 4);

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

  return (result);
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
double calculate_range(double stddev,double tval,double est[MAX_DATA],
  COUNTER count)
{
  double wsum = 0.0;      /* sum of all w's */
  double xsum = 0.0;      /* sum of all x's */
  double ysum = 0.0;      /* sum of all y's */
  double wmean = 0.0;      /* average for w */
  double xmean = 0.0;      /* average for x */
  double ymean = 0.0;      /* average for y */
  double root = 0.0;       /* temp for square root */
  double wsigma = 0.0;     /* intermediate summation */
  double xsigma = 0.0;     /* intermediate summation */
  double ysigma = 0.0;     /* intermediate summation */
  double under_sqrt = 0.0;   /* total for stuff under the root */
  double result = 0.0;     /* return value */

  /* FORMULA FOR RANGE:
                  ---------------------------------------------------------
                |                    2                2                2
                |           (w  - w )        (x  - x )        (y  - y )
                |      1      k    avg         k    avg         k    avg
  t(a/2,n-4) s  | 1 + --- + -------------- + -------------- + --------------
                |      n      n                n                n
            \   |           ---         2    ---         2    ---         2
             \  |           \   (w  - w)     \   (x  - x)     \   (y  - y)
              \ |           /     i    avg   /     i    avg   /     i    avg
               \|           ---              ---              ---
                            i=1              i=1              i=1
  */



  /* protect against divide by zero */
  if (count == 0)
    return 0.0;

  /* get the sums */
  wsum = sum_entries(W_DATA);
  xsum = sum_entries(X_DATA);
  ysum = sum_entries(Y_DATA);

  /* calculate the average */
  wmean = wsum / count;
  xmean = xsum / count;
  ymean = ysum / count;

  /* calculate the individual sigmas under the root */
  wsigma = sigma_i_avg_sqr_entries(wmean,W_DATA);
  xsigma = sigma_i_avg_sqr_entries(xmean,X_DATA);
  ysigma = sigma_i_avg_sqr_entries(ymean,Y_DATA);

  /* protect against divide by zero */
  if ((wsigma == 0.0) || (xsigma == 0.0) || (ysigma == 0.0))
    return (0.0);

  /* total terms under the root */
  under_sqrt = 1.0 + (1.0/(double)count) +
               ( ((est[W_DATA]-wmean)*(est[W_DATA]-wmean)) / (wsigma) ) +
               ( ((est[X_DATA]-xmean)*(est[X_DATA]-xmean)) / (xsigma) ) +
               ( ((est[Y_DATA]-ymean)*(est[Y_DATA]-ymean)) / (ysigma) );

  /* protect against square root of negative number */
  if (under_sqrt < 0.0)
    return 0.0;

  root = sqrt(under_sqrt);

  /* finally! */
  result = tval * stddev * root;

  #ifdef DEBUG
    printf("Calc Range: wsum=%5.3f count=%lu wmean=%5.3f wsigma=%5.3f\n",
            wsum,count,wmean,wsigma);
    printf("Calc Range: xsum=%5.3f count=%lu xmean=%5.3f xsigma=%5.3f\n",
            xsum,count,xmean,xsigma);
    printf("Calc Range: ysum=%5.3f count=%lu ymean=%5.3f ysigma=%5.3f\n",
            ysum,count,ymean,ysigma);

    printf("under sqrt=%5.3f root=%5.3f tval=%5.3f stddev=%5.3f\n",
            under_sqrt,root,tval,stddev);
  #endif

  return (result);
}

/**************************************************************************
*
* Function:    sigma_i_avg_sqr_entries
*
* Description: Performs a summation equation.
*
* Parameters:  mean (IN) the mean of the x entries.
*              index (IN) index into data struct of element
*
* Globals:     none
*
* Locals:      head (IN) head of the list
*
* Return:      sum of equations for sigma.
*
**************************************************************************/
double sigma_i_avg_sqr_entries(double mean,DATA_INDEX index)
{
  double sigma = 0.0;        /* return value */
  ELEMENT *current = head;   /* holds current element */

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
    sigma += pow(((current->data[index]) - mean),2);
    current = current->next;
  }
  return (sigma);
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
  unsigned char continue_flag = TRUE;  /* flag to encourage more guessing */
  double t_guess = 0.0;     /* t distribution guess */
  double result = 0.0;      /* sum of terms */
  double error = 0.0;       /* difference of this result from needed number */
  double max_error = 0.00001; /* error must be less than this to be ok */
  double low_t = 0.253;     /* number from chart in appendix A  */
  double high_t = 63.657;   /* lowest and highest t guess allowed */


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

  return (t_guess);
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
  double result = 1.0; /* return value */

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

  return (result);
}

/**************************************************************************
*
* Function:    gaussian_method
*
* Description: Gauss's method for solving a set of simultaneous equations
*              with three variables and four unknown parameters.
*
* Parameters:  array - array of terms in the simultaneous equations.
*              beta - beta return values.
*
* Globals:     none
*
* Locals:      none
*
* Return:      none
*
**************************************************************************/
void gaussian_method(double array[4][5],double beta[4])
{
  int i = 0,j = 0; /* counters */
  double diag1[3][5] =   /* temp for first eval */
  {
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0}
  };
  double diag2[2][5] =   /* temp for second eval */
  {
    {0.0,0.0,0.0,0.0,0.0},
    {0.0,0.0,0.0,0.0,0.0}
  };
  double diag3[5] = {0.0,0.0,0.0,0.0,0.0}; /* temp for third eval */


  /* Multiply every term of the first equation by the ratio of
     the beta0 terms from the first two equations.
     The new equation is then subtracted, term by term,
     from the second equation, giving a new equation
     The same procedure is followed to eliminate B0 from
     the third equation */
  #ifdef DEBUG
  printf("First Diagonal\n");
  #endif
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 5; j++)
    {
      diag1[i][j] = array[i+1][j] -
        (array[0][j] * (array[i+1][0] / array[0][0]));
      #ifdef DEBUG
      printf("[%d][%d] = %.1f\n",i,j,diag1[i][j]);
      #endif
    }
  }

  /* Multiply every term of the first equation by the ratio of
     the beta0 terms from the first two equations.
     The new equation is then subtracted, term by term,
     from the second equation, giving a new equation
     The same procedure is followed to eliminate B0 from
     the third equation */
  #ifdef DEBUG
  printf("Second Diagonal\n");
  #endif
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 5; j++)
    {
      diag2[i][j] =
        diag1[i+1][j] - (diag1[0][j] * (diag1[i+1][1] / diag1[0][1]));
      #ifdef DEBUG
      printf("[%d][%d] = %.1f\n",i,j,diag2[i][j]);
      # endif
    }
  }

  #ifdef DEBUG
  printf("Third Diagonal\n");
  #endif
  for (j = 0; j < 5; j++)
  {
    diag3[j] =
      diag2[1][j] - (diag2[0][j] * (diag2[1][2] / diag2[0][2]));
    #ifdef DEBUG
    printf("[%d] = %.1f\n",j,diag3[j]);
    # endif
  }

  /* now the unknowns can be obtained by first solving for B3 in the
     last equation.  This value is then substituted into the other
     three equations. */
  beta[3] = diag3[4]/diag3[3];
  beta[2] = (diag2[0][4] - (diag2[0][3]*beta[3])) / diag2[0][2];
  beta[1] = (diag1[0][4] -
    ((diag1[0][2]*beta[2]) +
     (diag1[0][3]*beta[3]))) / diag1[0][1];
  beta[0] = (array[0][4] -
    ((array[0][1]*beta[1]) +
     (array[0][2]*beta[2]) +
     (array[0][3]*beta[3]))) / array[0][0];

  return;
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
  char *str = NULL; /* temp for part of string */
  char *name = NULL; /* holds name */

  /* Find the last token - that is the actual filename */
  str = strtok(filename,":\\");
  while (str != NULL)
  {
    name = str;
    str = strtok(NULL,":\\");
  }

  printf("Prediction Interval for Size Estimates\n");
  printf("using multiple regression.\n");
  printf("written by Steve Karg\n");
  printf("27-Jul-1998\n");
  printf("\n");
  printf("Place the data in a file with four columns w, x, y, and z\n");
  printf("seperated by commas.  Place the filename and the four\n");
  printf("estimates on the command line.\n");
  printf("Usage:\n");
  if (name != NULL)
    printf("      %s filename w x y\n",name);
  else
    printf("      MREGRES filename w x y\n");

  return;
}

