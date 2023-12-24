// ***********************************************************************
//
// Copyright (c) 2005 Stephen Karg
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Technical
// Description: This module is responsible for graphic text, lines, circles,
//              and rectangles
//
// History:  1: 20-Feb-2005: Created file. - Steve Karg
//
// ***********************************************************************

#ifndef GRAPHICS_H
#define GRAPHICS_H
/* 16 colors */
enum _color{
  BLACK = 0, 
  BLUE, 
  GREEN,
  CYAN,
  RED,
  MAGENTA,
  BROWN,
  LIGHTGRAY, 
  DARKGRAY,
  LIGHTBLUE, 
  LIGHTGREEN, 
  LIGHTCYAN, 
  LIGHTRED, 
  LIGHTMAGENTA, 
  YELLOW, 
  WHITE
};
typedef enum _color COLORS;

enum _text_justify
{
  TEXT_JUSTIFY_LEFT,
  TEXT_JUSTIFY_RIGHT,
  TEXT_JUSTIFY_CENTER
};
typedef enum _text_justify TEXT_JUSTIFY;

void graphics_init(const char *window_title, int max_x, int max_y);
// size in points
void graphics_text(int size, COLORS color, TEXT_JUSTIFY justify,
  int x, int y, const char *text);
void graphics_line(COLORS color,
  int x1, int y1,
  int x2, int y2);
void graphics_circle(COLORS color, int x, int y, int r);
// x,y is the upper left coordinate, width to the right, height going down
void graphics_rectangle(COLORS color, bool fill, int x, int y, int width, int height);
void graphics_background(COLORS color);
void graphics_unlock(void);
int graphics_lock(void);
void graphics_update(void);
void graphics_delay(long ticks);

#endif
