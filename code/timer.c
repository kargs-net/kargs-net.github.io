/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2003 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <windows.h>
#include "timer.h"

//#define TEST
//#define TEST_TIMER

void OS_TimerMark(OS_Timer *timer)
{
  *timer = GetTickCount();
}

static unsigned long timer_difference(
  unsigned long current_time,
  unsigned long marked_time)
{
  unsigned long difference;

  if (current_time >= marked_time)
    difference = (current_time - marked_time);
  else
    difference = ((~0UL) - marked_time + current_time);

  return difference;
}

unsigned long OS_TimerElapsedSeconds(OS_Timer timer)
{
  unsigned long current_time = GetTickCount();

  return timer_difference(current_time,timer) / 1000;
}

unsigned long OS_TimerElapsedMilliSecs(OS_Timer timer)
{
  unsigned long current_time = GetTickCount();

  return timer_difference(current_time,timer);
}

void OS_Delay(unsigned long millisecs)
{
  Sleep(millisecs);
}

void OS_DelayUntil(unsigned long millisecs)
{
  Sleep(millisecs);
}

#ifdef TEST
#include <assert.h>
#include <conio.h>
#include "ctest.h"
/* since the sleep function is non-deterministic, we need to
   add a little slop into our comparison */
static int sloppy_compare(
  unsigned long value1,
  unsigned long value2,
  unsigned long slop)/* amount of slop */
{
  int compare = 1;

  if (value1 == value2)
    compare = 0;
  else if ((value1 > value2) && ((value1 - value2) <= slop))
    compare = 0;
  else if ((value2 > value1) && ((value2 - value1) <= slop))
    compare = 0;

  if (compare)
    printf("%lu != %lu (%lu slop)\r\n",value1,value2,slop);
  return compare;
}

void testTimedDuration(Test* pTest)
{
  unsigned long seconds;
  unsigned long milliseconds;
  unsigned long test_millisecs;
  OS_Timer timer;


  for (test_millisecs = 100; test_millisecs < 5000; test_millisecs += 1000)
  {
    OS_TimerMark(&timer);
    OS_Delay(test_millisecs);
    milliseconds = OS_TimerElapsedMilliSecs(timer);
    seconds = OS_TimerElapsedSeconds(timer);

    ct_test(pTest, seconds == test_millisecs/1000);
    ct_test(pTest, sloppy_compare(milliseconds,test_millisecs,10) == 0);
  }

  return;
}
#endif

#ifdef TEST_TIMER
int main(void)
{
  Test *pTest;
  bool rc;

  pTest = ct_create("timer", NULL);
  assert(pTest != NULL);

  /* individual tests */
  rc = ct_addTestFunction(pTest, testTimedDuration);
  assert(rc);

  /* configure output, and run! */
  ct_setStream(pTest, stdout);
  ct_run(pTest);
  (void)ct_report(pTest);

  ct_destroy(pTest);

  printf("Press key to quit");
  (void)getch();

  return 0;
}
#endif

