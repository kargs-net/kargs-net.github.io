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
// History:  1: 20-Feb-2005: Created file, using the tutorials and code
//                           from Jari Komppa for the lines, circles,
//                           and rectangles.
//                           http://sol.planet-d.net/
//
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "SDL.h" // Simple DirectMultimedia Library
#include "SDL_ttf.h" // SDL True Type Font library
#include "graphics.h"

// A single global for this one screen
static SDL_Surface *screen = NULL;

void graphics_init(const char *window_title, int max_x, int max_y)
{
  static bool sdl_initialized = false;

  // only initialize SDL once
  if (!sdl_initialized)
  {
    sdl_initialized = true;
    if (SDL_Init (SDL_INIT_VIDEO) < 0)
    {
      fprintf (stderr, "Unable to init SDL: %s\n", SDL_GetError ());
      exit (1);
    }
    atexit(SDL_Quit);

    /* Set window title */
    SDL_WM_SetCaption(window_title, window_title);

    screen = SDL_SetVideoMode (
      max_x,
      max_y,
      0,
      SDL_SWSURFACE | SDL_ANYFORMAT);
    if (screen == NULL)
    {
      fprintf (stderr, "Unable to set 640x480 video: %s\n", SDL_GetError ());
      exit (2);
    }
    
    if (TTF_Init () == -1)
    {
      fprintf (stderr, "TTF_Init: %s\n", TTF_GetError ());
      exit (3);
    }
    atexit(TTF_Quit);
  }
}

int graphics_lock(void)
{
  // Lock surface if needed
  if (SDL_MUSTLOCK(screen))
    return SDL_LockSurface(screen);
  
  return 0;
}

void graphics_unlock(void)
{
  // Unlock if needed
  if (SDL_MUSTLOCK(screen))
    SDL_UnlockSurface(screen);
}

void graphics_delay(long ticks)
{
  SDL_Delay(ticks);
}

/*
    Nothing you draw on a software surface is visible until it has been copied
from memory to the display buffer on the video card. SDL provides two ways to do
that: SDL_Flip() and SDL_UpdateRect(). SDL_Flip() copies the entire software
surface to the screen. If your screen is set to 640x480 at 4 bytes per pixel,
SDL_Flip() will copy 1.2 megabytes per frame and your frame rate will be limited
by how fast your computer can copy images to the screen.


SDL_UpdateRect() is designed to let you use a "dirty pixels" scheme. It lets you
specify a list of rectangular areas that have been changed and only copies those
areas to the screen. This technique is ideal for a game with a complex
background but only a small number of moving or changing items. Tracking dirty
pixels can give you a dramatic improvement in performance.
*/
void graphics_update(void)
{
  SDL_Flip(screen);
  //SDL_UpdateRect(screen, 0,0,0,0);
}

static void graphics_color_convert(COLORS color, SDL_Color *sdl_color)
{
  sdl_color->unused = 0;
  switch (color)
  {
    case BLACK:
      sdl_color->r = 0;
      sdl_color->g = 0;
      sdl_color->b = 0;
      break;
    case BLUE:
      sdl_color->r = 0;
      sdl_color->g = 0;
      sdl_color->b = 255;
      break;
    case GREEN:
      sdl_color->r = 0;
      sdl_color->g = 128;
      sdl_color->b = 0;
      break;
    case CYAN:
      sdl_color->r = 128;
      sdl_color->g = 255;
      sdl_color->b = 255;
      break;
    case RED:
      sdl_color->r = 255;
      sdl_color->g = 0;
      sdl_color->b = 0;
      break;
    case MAGENTA:
      sdl_color->r = 255;
      sdl_color->g = 0;
      sdl_color->b = 64;
      break;
    case BROWN:
      sdl_color->r = 125;
      sdl_color->g = 64;
      sdl_color->b = 0;
      break;
    case LIGHTGRAY:
      sdl_color->r = 196;
      sdl_color->g = 196;
      sdl_color->b = 196;
      break;
    case DARKGRAY:
      sdl_color->r = 128;
      sdl_color->g = 128;
      sdl_color->b = 128;
      break;
    case LIGHTBLUE:
      sdl_color->r = 0;
      sdl_color->g = 128;
      sdl_color->b = 255;
      break;
    case LIGHTGREEN:
      sdl_color->r = 0;
      sdl_color->g = 255;
      sdl_color->b = 0;
      break;
    case LIGHTCYAN:
      sdl_color->r = 0;
      sdl_color->g = 255;
      sdl_color->b = 255;
      break;
    case LIGHTRED:
      sdl_color->r = 255;
      sdl_color->g = 128;
      sdl_color->b = 128;
      break;
    case LIGHTMAGENTA:
      sdl_color->r = 255;
      sdl_color->g = 0;
      sdl_color->b = 128;
      break;
    case YELLOW:
      sdl_color->r = 255;
      sdl_color->g = 255;
      sdl_color->b = 0;
      break;
    case WHITE:
      sdl_color->r = 255;
      sdl_color->g = 255;
      sdl_color->b = 255;
      break;
  }
}

static void write_text(
  TTF_Font *font, 
  const char *text, 
  int x, 
  int y, 
  SDL_Color color,
  TEXT_JUSTIFY justify)
{
  int width = 10, height = 10; // for actual height and width of string
  SDL_Rect rect; // a rectangle place to draw the text within
  int offset = 0;

  TTF_SizeText(font, text, &width, &height);
  rect.y = y;
  rect.w = width;
  rect.h = height;
  switch (justify)
  { 
    case TEXT_JUSTIFY_CENTER:
     offset = width/2;
     if (offset > x)
       offset = x;
     rect.x = x - offset;
     break;
   case TEXT_JUSTIFY_RIGHT:
     offset = width;
     if (offset > x)
       offset = x;
     rect.x = x - offset;
     break;
   case TEXT_JUSTIFY_LEFT:
   default:
     rect.x = x;
     break;
  }
  SDL_Surface *sText = TTF_RenderText_Solid(font, text, color);
  SDL_BlitSurface( sText, NULL, screen, &rect );
  SDL_FreeSurface( sText );
  SDL_UpdateRect(screen, rect.x, rect.y, rect.w, rect.h);
}

// size in points
void graphics_text(int size, COLORS color, TEXT_JUSTIFY justify,
  int x, int y, const char *text)
{
  static TTF_Font *font = NULL;
  static int old_size = 0;
  SDL_Color sdl_text_color; // color to draw the text in
  int lineSkip = 0;
  char *line = NULL;
  int line_len = 0;
  int line_start = 0;
  const char *fontname = "FreeSans.ttf";
   
  // load font.ttf at size into font
  if (!font)
  {
    old_size = size;
    font = TTF_OpenFont (fontname, size);
  }
  // already open - are we requesting the same size?
  if (font && (old_size != size))
  {
    TTF_CloseFont(font);
    old_size = size;
    font = TTF_OpenFont (fontname, size);
  }
  // unable to open the font.
  if (!font)
  {
    fprintf (stderr, "TTF_OpenFont: %s\n", TTF_GetError ());
    exit(1);
  }
  lineSkip = TTF_FontLineSkip(font);
  // bound checking
  if (x > screen->w)
    x = screen->w;
  if (y > screen->h)
    y = screen->h;
  graphics_color_convert(color,&sdl_text_color);
  // parse the text and look for NEWLINE characters
  // which will give us more than one line
  line = strdup(text);
  if (line)
  {
    line_len = strlen(text) + 1;
    for (int i = 0; i < line_len; i++)
   {
      // end of line?
     if (text[i] == '\n')
     {
      line[i] = 0;
        write_text(font, &line[line_start], x, y, sdl_text_color,justify);
      line_start = i + 1;
      y += lineSkip;
     } 
     // end of string?
     if (text[i] == 0)
        write_text(font, &line[line_start], x, y, sdl_text_color,justify);
   }
   free(line);
  }
}

//----------------------------------------------------------

// A set of very useful macros
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define abs(a) (((a)<0) ? -(a) : (a))
#define sign(a) (((a)<0) ? -1 : (a)>0 ? 1 : 0)
#define PUTPIXEL(s,x,y,c) \
  ((unsigned int*)s->pixels)[(x) + (y) * (s->pitch / 4)] = (c);

//----------------------------------------------------------
// The following code implements a Bresenham line drawing
// algorithm. There are 4 separate routines each optimized
// for one of the four pixel depths supported by SDL. SDL
// support many pixel formats, but it only support 8, 16,
// 24, and 32 bit pixels.
//----------------------------------------------------------

// Draw lines in 8 bit surfaces.
static void line8(SDL_Surface *s,
                  int x1, int y1, 
                  int x2, int y2, 
                  Uint32 color)
{
  int d;
  int x;
  int y;
  int ax;
  int ay;
  int sx;
  int sy;
  int dx;
  int dy;

  Uint8 *lineAddr;
  Sint32 yOffset;

  dx = x2 - x1;  
  ax = abs(dx) << 1;  
  sx = sign(dx);

  dy = y2 - y1;  
  ay = abs(dy) << 1;  
  sy = sign(dy);
  yOffset = sy * s->pitch;

  x = x1;
  y = y1;

  lineAddr = ((Uint8 *)(s->pixels)) + (y * s->pitch);
  if (ax>ay)
  {                      /* x dominant */
    d = ay - (ax >> 1);
    for (;;)
    {
      *(lineAddr + x) = (Uint8)color;

      if (x == x2)
      {
        return;
      }
      if (d>=0)
      {
        y += sy;
        lineAddr += yOffset;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {                      /* y dominant */
    d = ax - (ay >> 1);
    for (;;)
    {
      *(lineAddr + x) = (Uint8)color;

      if (y == y2)
      {
        return;
      }
      if (d>=0) 
      {
        x += sx;
        d -= ay;
      }
      y += sy;
      lineAddr += yOffset;
      d += ax;
    }
  }
}

//----------------------------------------------------------
// Draw lines in 16 bit surfaces. Note that this code will
// also work on 15 bit surfaces.
static void line16(SDL_Surface *s,
                   int x1, int y1, 
                   int x2, int y2, 
                   Uint32 color)
{
  int d;
  int x;
  int y;
  int ax;
  int ay;
  int sx;
  int sy;
  int dx;
  int dy;

  Uint8 *lineAddr;
  Sint32 yOffset;

  dx = x2 - x1;  
  ax = abs(dx) << 1;  
  sx = sign(dx);

  dy = y2 - y1;  
  ay = abs(dy) << 1;  
  sy = sign(dy);
  yOffset = sy * s->pitch;

  x = x1;
  y = y1;

  lineAddr = ((Uint8 *)s->pixels) + (y * s->pitch);
  if (ax>ay)
  {                      /* x dominant */
    d = ay - (ax >> 1);
    for (;;)
    {
      *((Uint16 *)(lineAddr + (x << 1))) = (Uint16)color;

      if (x == x2)
      {
        return;
      }
      if (d>=0)
      {
        y += sy;
        lineAddr += yOffset;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {                      /* y dominant */
    d = ax - (ay >> 1);
    for (;;)
    {
      *((Uint16 *)(lineAddr + (x << 1))) = (Uint16)color;

      if (y == y2)
      {
        return;
      }
      if (d>=0) 
      {
        x += sx;
        d -= ay;
      }
      y += sy;
      lineAddr += yOffset;
      d += ax;
    }
  }
}

//----------------------------------------------------------

// Draw lines in 24 bit surfaces. 24 bit surfaces require
// special handling because the pixels don't fall on even
// address boundaries. Instead of being able to store a
// single byte, word, or long you have to store 3
// individual bytes. As a result 24 bit graphics is slower
// than the other pixel sizes.

static void line24(SDL_Surface *s, 
                   int x1, int y1, 
                   int x2, int y2, 
                   Uint32 color)
{
  int d;
  int x;
  int y;
  int ax;
  int ay;
  int sx;
  int sy;
  int dx;
  int dy;

  Uint8 *lineAddr;
  Sint32 yOffset;

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
  color <<= 8;
#endif

  dx = x2 - x1;  
  ax = abs(dx) << 1;  
  sx = sign(dx);

  dy = y2 - y1;  
  ay = abs(dy) << 1;  
  sy = sign(dy);
  yOffset = sy * s->pitch;

  x = x1;
  y = y1;

  lineAddr = ((Uint8 *)(s->pixels)) + (y * s->pitch);
  if (ax>ay)
  {                      /* x dominant */
    d = ay - (ax >> 1);
    for (;;)
    {
      Uint8 *p = (lineAddr + (x * 3));
      memcpy(p, &color, 3);

      if (x == x2)
      {
        return;
      }
      if (d>=0)
      {
        y += sy;
        lineAddr += yOffset;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {                      /* y dominant */
    d = ax - (ay >> 1);
    for (;;)
    {
      Uint8 *p = (lineAddr + (x * 3));
      memcpy(p, &color, 3);

      if (y == y2)
      {
        return;
      }
      if (d>=0) 
      {
        x += sx;
        d -= ay;
      }
      y += sy;
      lineAddr += yOffset;
      d += ax;
    }
  }
}

//----------------------------------------------------------

// Draw lines in 32 bit surfaces. Note that this routine
// ignores alpha values. It writes them into the surface
// if they are included in the pixel, but does nothing
// else with them.

static void line32(SDL_Surface *s, 
                   int x1, int y1, 
                   int x2, int y2, 
                   Uint32 color)
{
  int d;
  int x;
  int y;
  int ax;
  int ay;
  int sx;
  int sy;
  int dx;
  int dy;

  Uint8 *lineAddr;
  Sint32 yOffset;

  dx = x2 - x1;  
  ax = abs(dx) << 1;  
  sx = sign(dx);

  dy = y2 - y1;  
  ay = abs(dy) << 1;  
  sy = sign(dy);
  yOffset = sy * s->pitch;

  x = x1;
  y = y1;

  lineAddr = ((Uint8 *)(s->pixels)) + (y * s->pitch);
  if (ax>ay)
  {                      /* x dominant */
    d = ay - (ax >> 1);
    for (;;)
    {
      *((Uint32 *)(lineAddr + (x << 2))) = (Uint32)color;

      if (x == x2)
      {
        return;
      }
      if (d>=0)
      {
        y += sy;
        lineAddr += yOffset;
        d -= ax;
      }
      x += sx;
      d += ay;
    }
  }
  else
  {                      /* y dominant */
    d = ax - (ay >> 1);
    for (;;)
    {
      *((Uint32 *)(lineAddr + (x << 2))) = (Uint32)color;

      if (y == y2)
      {
        return;
      }
      if (d>=0) 
      {
        x += sx;
        d -= ay;
      }
      y += sy;
      lineAddr += yOffset;
      d += ax;
    }
  }
}

//----------------------------------------------------------

// Examine the depth of a surface and select a line
// drawing routine optimized for the bytes/pixel of the
// surface.

static void line(SDL_Surface *s, 
                 int x1, int y1, 
                 int x2, int y2, 
                 Uint32 color)
{
  // clipping
  if (y1 >= s->h)
    y1 = s->h - 1;
  if (y2 >= s->h)
    y2 = s->h - 1;
  if (x1 >= s->w)
    x1 = s->w - 1;
  if (x2 >= s->w)
    x2 = s->w - 1;

  // handle line based on color depth
  switch (s->format->BytesPerPixel)
  {
    case 1:
      line8(s, x1, y1, x2, y2, color);
      break;
    case 2:
      line16(s, x1, y1, x2, y2, color);
      break;
    case 3:
      line24(s, x1, y1, x2, y2, color);
      break;
    case 4:
      line32(s, x1, y1, x2, y2, color);
      break;
  }
}

void graphics_line(COLORS color,
  int x1, int y1,
  int x2, int y2)
{
  SDL_Color scolor;
  int width = 0, height = 0;
  
  graphics_color_convert(color, &scolor);
  line(screen, x1, y1, x2, y2,
    SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
  // update the area that is changed
  if (x1 > x2)
    width = x1 - x2;
  else
    width = x2 - x1;
  if (y1 > y2)
    height = y1 - y2;
  else
    height = y2 - y1;
  SDL_UpdateRect(screen, x1, y1, width , height);
}

#define PI 3.1415926535897932384626433832795f

static void drawcircle(SDL_Surface *s, int x, int y, int r, Uint32 color)
{
  int i, j;
  for (i = 0; i < 2 * r; i++)
  {
    // vertical clipping: (top and bottom)
    if ((y - r + i) >= 0 && (y - r + i) < s->h)
    {
      int len = (int)(sqrt(cos(0.5f * PI * (i - r) / r)) * r * 2);
      int xofs = x - len / 2;
      
      // left border
      if (xofs < 0)
      {
        len += xofs;
        xofs = 0;
      }
      
      // right border
      if (xofs + len >= s->w)
      {
        len -= (xofs + len) - s->w;
      }
      int ofs = (y - r + i) * (s->pitch/4) + xofs;
      
      // note that len may be 0 at this point, 
      // and no pixels get drawn!
      for (j = 0; j < len; j++)
        ((unsigned int*)s->pixels)[ofs + j] = color;
    }
  }
}

void graphics_circle(COLORS color, int x, int y, int r)
{
  SDL_Color scolor;
  int top_x = 0, top_y = 0, width = 0, height = 0;
  
  graphics_color_convert(color, &scolor);
  drawcircle(screen, x, y, r,
    SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
  // update the area that is changed
  if (r > x)
   top_x = 0;
  else
    top_x = x - r;
  if (r > y)
    top_y = 0;
  else
    top_y = y - r;
  height = width = r * 2;
  SDL_UpdateRect(screen, top_x, top_y, width , height);
}

static void drawrectfilled(SDL_Surface *s, int x, int y, int width, int height, Uint32 color)
{
  int i, j;
  for (i = 0; i < height; i++)
  {
    // vertical clipping: (top and bottom)
    if ((y + i) >= 0 && (y + i) < s->h)
    {
      int len = width;
      int xofs = x;

      // left border
      if (xofs < 0)
      {
        len += xofs;
        xofs = 0;
      }

      // right border
      if (xofs + len >= s->w)
      {
        len -= (xofs + len) - s->w;
      }
      int ofs = (i + y) * (s->pitch/4) + xofs;

      // note that len may be 0 at this point, 
      // and no pixels get drawn!
      for (j = 0; j < len; j++)
        ((unsigned int*)s->pixels)[ofs + j] = color;
    }
  }
}


// x,y is the upper left coordinate, width to the right, height going down
void graphics_rectangle(COLORS color, bool fill, int x, int y, int width, int height)
{
  SDL_Color scolor;

  graphics_color_convert(color, &scolor);
  if (fill)
    drawrectfilled(screen, x, y, width, height,
      SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
  else
  {
    line(screen, x, y, x + width, y,
      SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
    line(screen, x + width, y, x + width, y + height,
      SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
    line(screen, x + width, y + height, x, y + height,
      SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
    line(screen, x, y + height, x, y,
      SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
  }  
  SDL_UpdateRect(screen, x, y, width, height);
}

void graphics_background(COLORS color)
{   
  SDL_Rect* dstrect = NULL;
  SDL_Color scolor;

  graphics_color_convert(color, &scolor);
  SDL_FillRect(
    screen,
    dstrect,
    SDL_MapRGB(screen->format, scolor.r, scolor.g, scolor.b));
  // causes flicker
  //SDL_UpdateRect(screen, 0,0,0,0);
}    

/* Used to unit test this module */
#ifdef TEST
void graphics_test(int color, char ch)
{
  static char text[80] = "";

  graphics_text(36,LIGHTMAGENTA, TEXT_JUSTIFY_CENTER,
    3 * screen->w/4, screen->h/5,
    "Graphics Test\n"
    "Joshua\n"
    "Mary\n"
    "Anna\n"
    "Christopher");
  if (ch)
  {
    if (text[0])
      graphics_text(72,BLACK, TEXT_JUSTIFY_LEFT,
        screen->w/4, screen->h/5, text);
    sprintf(text,"%c",ch);
    graphics_text(72,MAGENTA, TEXT_JUSTIFY_LEFT,
      screen->w/4, screen->h/5, text);
  }
  // draw a line from each of the four corners to the center
  graphics_line(RED, 0, 0,
    screen->w/2, screen->h/2);
  graphics_line(GREEN, 0, screen->h-1,
    screen->w/2, screen->h/2);
  graphics_line(BLUE, screen->w-1, 0,
    screen->w/2, screen->h/2);
  graphics_line(YELLOW, screen->w-1, screen->h-1,
    screen->w/2, screen->h/2);
    
  graphics_rectangle (BROWN, true, screen->w/2, screen->h/2, 50, 50);
  graphics_rectangle (CYAN, false, screen->w/2, screen->h/2, 50, 50);
  graphics_circle((COLORS)color, screen->w/2, screen->h/2, 10);
}

#endif
#ifdef TEST_GRAPHICS
void render(char ch)
{
  static int color = 0;
  static int last_tick = 0;

// Lock surface if needed
// don't lock surface since primatives will be updating screen
  //if (SDL_MUSTLOCK(screen))
  //  if (SDL_LockSurface(screen) < 0)
  //    return;

  #if 0
  // Frame rate independence
  int tick = SDL_GetTicks();
  if (tick <= last_tick)
  {
    SDL_Delay(1);
    return;
  }
  while (last_tick < tick)
  {
    // actually do some stuff
    graphics_test(color, ch);
    color++;
    if (color > 15)
      color = 0;
    // give us 10 ticks per frame, or 100 FPS
    last_tick += 10;
  }
  #else
  SDL_Delay(100);
  graphics_test(color, ch);
  color++;
  if (color > 15)
    color = 0;
  #endif

  // Unlock if needed
  //if (SDL_MUSTLOCK(screen))
  //  SDL_UnlockSurface(screen);
  /*
    Nothing you draw on a software surface is visible until it has been copied
from memory to the display buffer on the video card. SDL provides two ways to do
that: SDL_Flip() and SDL_UpdateRect(). SDL_Flip() copies the entire software
surface to the screen. If your screen is set to 640x480 at 4 bytes per pixel,
SDL_Flip() will copy 1.2 megabytes per frame and your frame rate will be limited
by how fast your computer can copy images to the screen.


SDL_UpdateRect() is designed to let you use a "dirty pixels" scheme. It lets you
specify a list of rectangular areas that have been changed and only copies those
areas to the screen. This technique is ideal for a game with a complex
background but only a small number of moving or changing items. Tracking dirty
pixels can give you a dramatic improvement in performance.
  */
  SDL_Flip(screen);
  //SDL_UpdateRect(screen, 0,0,0,0);
}

int main (void)
{
  SDL_Event event; /* Event structure */
  char ch = 0;
  
  graphics_init("Graphics Test",640,480);
  graphics_background(BLACK);
  
  /* Check for events */
  for ( ; ; )
  {
    render(ch);    
    //look for an event, process them all
    while (SDL_PollEvent(&event))
    {
      //an event was found
      switch(event.type)
      {
        case SDL_KEYDOWN:
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym)
          {
            case  SDLK_ESCAPE:
              return 0;
            case SDLK_0:
              ch = '0';
              break;
            case SDLK_1:
              ch = '1';
              break;
            case SDLK_2:
              ch = '2';
              break;
            case SDLK_3:
              ch = '3';
              break;
            case SDLK_4:
              ch = '4';
              break;
            case SDLK_5:
              ch = '5';
              break;
            case SDLK_6:
              ch = '6';
              break;
            case SDLK_7:
              ch = '7';
              break;
            case SDLK_8:
              ch = '8';
              graphics_background(DARKGRAY);
              break;
            case SDLK_9:
              ch = '9';
              graphics_background(BLACK);
              break;
            default:
              break;
          }
          break;
        case SDL_QUIT:
          return 0;
        default:
          break;
      }
    }
  }//end of message pump
  
  return 0;
}
#endif
