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
* Description: Reads in a C program file and counts the number of 
*              logical lines of code.  
*
* History: 1: 05-Dec-1996: Copied loc.c to create file - Steve Karg
*          2: 09-Jan-1997: Corrected flaw.  The single quote when used
*                          with control codes would skip if the control
*                          code was a \'. - Steve Karg
*          3: 28-Aug-1997: Modified the parser to accept # in any column
*                          and changed the %Lu to %lu in printf statements.
*                          - Steve Karg
*                          
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

/* LOCAL CONTSTANTS */
#define MAX_LINE_SIZE (80)
#define TRUE (1)
#define FALSE (0)
#define MAX_KEYWORDS (43)

/* set up counter type */
typedef unsigned long int COUNTER;

/* structure for keyword list */
typedef struct keyword
{
  char data[MAX_LINE_SIZE];  /* string containing the key word */
  unsigned char valid_flag;  /* is this used for LOC count? */
} KEYWORD;

/* array for keywords */
KEYWORD c_keywords[MAX_KEYWORDS] = 
{
  /* ANSI C LOC countable reserved words and symbols (15) */
  "case",     TRUE,  "default",  TRUE,  "do",       TRUE,
  "else",     TRUE,  "enum",     TRUE,  "for",      TRUE,
  "if",       TRUE,  "struct",   TRUE,  "switch",   TRUE,
  "union",    TRUE,  "while",    TRUE,  "#",        TRUE,
  ";",        TRUE,  ",",        TRUE,  "}",        TRUE,

  /* ANSI C Reserved words that are not LOC countable (28) */
  "asm",      FALSE,  "auto",     FALSE,  "break",    FALSE,
  "char",     FALSE,  "const",    FALSE,  "continue", FALSE,
  "do",       FALSE,  "double",   FALSE,  "entry",    FALSE,
  "enum",     FALSE,  "extern",   FALSE,  "float",    FALSE,
  "fortran",  FALSE,  "goto",     FALSE,  "int",      FALSE,
  "long",     FALSE,  "register", FALSE,  "return",   FALSE,
  "short",    FALSE,  "signed",   FALSE,  "sizeof",   FALSE,
  "static",   FALSE,  "unsigned", FALSE,  "void",     FALSE,
  "volatile", FALSE
};

/* FUNCTION PROTOTYPES */
FILE *open_input_file(char *filename);
void check_token(char *token,unsigned long int *counter);
int keyword_compare(char *word);
void keyword_print(void);

/**************************************************************************
*
* Function:    main   
*
* Description: Reads in a C program file and counts the number of 
*              logical lines of code.  
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
  COUNTER loc_count; /* number of logical lines of code */

  char filename[256];  /* C program to be counted filename */
  FILE *file_ptr;      /* C program file stream */

  char last_char;      /* the previous char read in from file */
  char new_char;       /* the current char read in from file */

  char token[MAX_LINE_SIZE]; /* word in file */
  char token2[MAX_LINE_SIZE];/* another word in a file. */
  unsigned short token_len;  /* length of the token string */

  unsigned char comment;     /* flag used for skipping stuff inside comments */
  unsigned char quotation;   /* flag used for skipping stuff inside quotes */
  unsigned char single_quote;/* skip stuff inside single quotes */
  unsigned char precompiler; /* skip pre-compiler defines */
  unsigned char control_code;/* skip control codes started with \ */
 
  /* INITIALIZE */
  new_char = 0;
  last_char = 0;

  token[0] = 0;
  token2[0] = 0;

  comment = FALSE;
  quotation = FALSE;
  single_quote = FALSE;
  precompiler = FALSE;
  control_code = FALSE;

  loc_count = 0;
  token_len = 0;

  /* === COUNT LOGICAL LOC === */
  /* prompt for C program filename if not line entered */
  if (argc > 1)
  {
    strcpy(filename,*(argv+1));
  }
  else
  {
    printf("\nLogical Line of Code Counter for ANSI C");
    printf("\nwritten by Steve Karg");
    printf("\n05-Dec-1996");
    printf("\n");
    printf("\nThis program counts the following key words while");
    printf("\nignoring blank lines and comments.  Also ignores");
    printf("\nthings in quotes and pre-compiler options.");
    printf("\n");
    printf("\n");
    keyword_print();
    printf("\nUsage:");
    printf("\n      %s [filename]",*(argv+0));
    printf("\n");
    printf("\n");
    return;
  }

  /* open the file */
  file_ptr = open_input_file(filename);
  if (file_ptr == NULL)
    return;

  /* read the file one character at a time */
  while ((new_char = fgetc(file_ptr)) != EOF)
  {
    /* COMMENT - skip until end of comment */
    if (comment != FALSE)
    {
      /* conditions to end the comment */
      if ((last_char == '*') && (new_char == '/'))
      {
        comment = FALSE;
      }
    } /* end of comment */

    /* QUOTES - skip until end of quotation */
    else if (quotation != FALSE)
    {
      /* conditions to end the quotation */
      if (control_code != FALSE)
        control_code = FALSE;
      else if (new_char == '\\')
        control_code = TRUE;
      else if (new_char == '"')
        quotation = FALSE;
    }

    /* SINGLE QUOTES - skip until end of quotes */
    else if (single_quote != FALSE)
    {
      /* conditions to end the quotation */
      if (control_code != FALSE)
        control_code = FALSE;
      else if (new_char == '\\')
        control_code = TRUE;
      else if (new_char == '\'')
        single_quote = FALSE;
    }

    /* PRE-COMPILER - skip until end of line but not \ eol */
    else if (precompiler != FALSE)
    {
      /* conditions to end the quotation */
      if ((last_char != '\\') && (new_char == '\n'))
        precompiler = FALSE;
    }

    else
    {
      /* Turn on Comment Flag */
      if ((new_char == '*') && (last_char == '/'))
      {
        comment = TRUE;
        /* shrink token by 1 to remove / */
        token_len = strlen(token);
        switch(token_len)
        {
          case 0:
            break;
          case 1:
            token[0] = 0;
            break;
          default:
            token[token_len-1] = 0;
            check_token(token,&loc_count);
            break;
        }
      }

      /* Turn on Quotation Flag */
      else if (new_char == '"')
      {
        quotation = TRUE;
        check_token(token,&loc_count);
      }

      /* Turn on Single Quotation Flag */
      else if (new_char == '\'')
      {
        single_quote = TRUE;
        check_token(token,&loc_count);
      }

      /* Turn on Pre-compiler Flag */
      else if (new_char == '#')
      {
        precompiler = TRUE;
        sprintf(token2,"%c",new_char);
        check_token(token2,&loc_count);
      }

      /* Force token check - EOL */
      else if (new_char == '\n')
        check_token(token,&loc_count);

      /* Force token check - WHITE SPACE */
      else if (isspace(new_char))
        check_token(token,&loc_count);

      /* Force token check - PUNCTUATION */
      else if (ispunct(new_char))
      {
        /* VALID NON-DELIMITER */
        if (new_char == '_')
        {
          /* valid character - include with token */
          sprintf(token,"%s%c",token,new_char);
        }
        /* DELIMITER FOUND */
        else
        {
          check_token(token,&loc_count);
          /* check punct to see if it is a countable token */
          sprintf(token2,"%c",new_char);
          check_token(token2,&loc_count);
        } /* end of token delimiter */
      }
      /* BUILD TOKEN */
      else
      {
        sprintf(token,"%s%c",token,new_char);
      }
    }

    /* update any previous variables */
    last_char = new_char;
  }

  /* close the file */
  fclose(file_ptr);

  /* Print the results */
  printf("%lu",loc_count);
  printf("\n");

  return;
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
* Function:    check_token
*
* Description: Compares tokens that contain something to the keywords and
*              increments a counter if there is a match.
*
* Parameters:  token - reference to a string that contains some characters.
*              count - reference to a counter that gets incremented upon a 
*                  match.
*
* Globals:     none
*
* Locals:      keyword_compare function.
*
* Return:      none
*
**************************************************************************/
void check_token(char *token,COUNTER *count)
{
  if (token[0] != 0)
  {
    if (keyword_compare(token))
      (*count)++;
    token[0] = 0;
  }
}

/**************************************************************************
*
* Function:    keyword_compare
*
* Description: Compares a string with a list of valid strings and checks
*              for a match.
*
* Parameters:  word - reference to a string that contains some characters.
*
* Globals:     none
*
* Locals:      MAX_KEYWORDS - number of keywords in the structure.
*              c_keywords - structure containing C keywords
*
* Return:      status - TRUE if the keyword match is found.
*                       FALSE if no match is found.
*
**************************************************************************/
int keyword_compare(char *word)
{
  int status;      /* TRUE if keyword match found */
  int key_index;   /* index into the keyword structure */

  status = FALSE;

  for (key_index=0;key_index < MAX_KEYWORDS;key_index++)
  {
    if (c_keywords[key_index].valid_flag)
    {
      if (strcmp(word,c_keywords[key_index].data) == 0)
        status = TRUE;
    }
  }

  return status;
}

/**************************************************************************
*
* Function:    keyword_print
*
* Description: Prints a list of valid strings on the screen 5 across.
*
* Parameters:  none
*
* Globals:     none
*
* Locals:      MAX_KEYWORDS - number of keywords in the structure.
*              c_keywords - structure containing C keywords
*
* Return:      none
*
**************************************************************************/
void keyword_print(void)
{
  int key_index;   /* index into the keyword structure */
  int count;       /* counter that counts number of words across screen */

  count = 0;
  
  for (key_index=0;key_index < MAX_KEYWORDS;key_index++)
  {
    if (c_keywords[key_index].valid_flag)
    {
      printf("%s",c_keywords[key_index].data);
      count++;
      if (count > 4)
      {
        printf("\n");
        count = 0;
      }
      else
        printf("\t");
    }
  }

  return;
}

