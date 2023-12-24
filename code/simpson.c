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
* Description: Calculates a normal distribution number using simpson
*              rule integration.
*
* History: 1: 22-Jan-1997: Created File - Steve Karg
*
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* LOCAL CONSTANTS */
#define TRUE (1)
#define FALSE (0)

/* set up debug */
unsigned char debug_flag;
char debug_string[256];

/* FUNCTION PROTOTYPES */
double simpson_integrate(double x_high);
double normal_distribution_function(double x);

/**************************************************************************
*
* Function:    main   
*
* Description: Main task for Simpson Rule Integration.
*
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
  double user_input;   /* x high that user entered */
  double result;

  user_input = 0.0;
  result = 0.0;
  debug_flag = FALSE;
  
  /* interprete the arguments */
  if (argc > 1)
  {
    user_input = atof(*(argv+1));
    if (argc > 2)
    {
      strcpy(debug_string,*(argv+2));
      if (debug_string[0] == 'd')
        debug_flag = TRUE;
    }
  }
  else
  {
    printf("Numerical Integrator using Simpson's Rule\n");
    printf("written by Steve Karg\n");
    printf("22-Jan-1997\n");
    printf("\n");
    printf("This program numerically integrates using Simpson's Rule\n");
    printf("for a paramenter x that is integrated from minus infinity to x.\n");
    printf("\n");
    printf("Usage:\n");
    printf("      %s [x]\n",*(argv+0));
    printf("\n");
    printf("\n");
    return;
  }

  /* Determine Rule for Normal Distribution */
  if (user_input > 0.0)
    result = simpson_integrate(user_input) + 0.5;
  else if (user_input < 0.0)
    result = 0.5 - simpson_integrate(user_input * (-1.0));
  else
    result = 0.5;

  /* Print the results */
  printf("                Normal\n");
  printf("     x       Distribution\n");
  printf("   Input        Result\n");
  printf("============ ============\n");
  printf("%12f %12f\n",user_input,result);
  printf("\n");

  return;
}

/* === Functions & Methods === */
/**************************************************************************
*
* Function:    simpson_integrate
*
* Description: Numerically integtrates a funtion based on Simpson's Rules.
*              
* Parameters:  x_high - integration limit.
*
* Globals:     none
*
* Locals:      none
*
* Return:      result - valuation of the integration process.
*
**************************************************************************/
double simpson_integrate(double x_high)
{
  unsigned char continue_flag;  /* flag to encourage integration */
  unsigned long int segments; /* number of iterations */
  unsigned long int n; /* segment index */
  double width;       /* width of each segment */
  double old_result;  /* previous answer */
  double result;      /* answer from integrating */
  double error;       /* difference of this result from past results */
  double max_error;   /* error must be less than this number to be ok */
  double x_low;       /* integration limit */
  double x_i;         /* number that the function will be evaluated at */
  

  continue_flag = TRUE;
  segments = 20;
  old_result = 0.0;
  result = 0.0;
  max_error = 0.000001;
  x_low = 0.0;

  while (continue_flag)
  {
    width = x_high / (double)segments;

    for (n=0; n <= segments; n++)
    {
      x_i = (double)n * width;

      /* first segment */
      if (n == 0)
        result += normal_distribution_function(x_i) * (width / 3);
      /* last segment */
      else if (n == segments)
        result += normal_distribution_function(x_i) * (width / 3);
      /* even segments */
      else if ( (n%2) == 0 )
        result += 2 * normal_distribution_function(x_i) * (width / 3);
      else
        result += 4 * normal_distribution_function(x_i) * (width / 3);

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

  return result;
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
  double exponent;
  double exponential;
  double result;

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
