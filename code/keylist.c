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

// Keyed Linked List Library
//
// This is a standard singly linked list
// It is sorted, and indexed, and keyed.
// It stores a pointer to data, which you must
// malloc and free on your own, or just use
// static data

#include <stdlib.h>

#include "keylist.h" // check for valid prototypes

/////////////////////////////////////////////////////////////////////
// Generic node routines
/////////////////////////////////////////////////////////////////////

// grab memory for a node
static OS_Keylist NodeCreate(void)
{
  return calloc(1,sizeof(struct Keylist_Node));
}

// find the next available key in the list of lists
static KEY NodeNextKey(
  OS_Keylist head,// head of the list
  KEY key)// starting key you wish to use - try zero
{
  OS_Keylist node;
  int found;

  if (head)
  {
    do
    {
      found = 0;
      node = head->next;
      while (node != NULL)
      {
        if (node->key == key)
        {
          key++;
          found = 1;
          break;
        }
        node = node->next;
      }
    } while (found);
  }

  return key;
}

// add the node and data to the list where
// the key indicates it should go
// return the place in the list where it went
static int NodeAddByKey(
  OS_Keylist head,// head of the list
  KEY key,
  void *data)
{
  int index = -1;
  OS_Keylist node;
  OS_Keylist prev;
  OS_Keylist next;

  if (head)
  {
    node = NodeCreate();
    if (node)
    {
      node->key = key;
      node->data = data;

      // Find the sorted place for this node.
      // The insertion takes place as soon as
      // we determine that the present key in
      // the list was larger than this one.
      index = 0;
      prev = head;
      next = head->next;
      while (next)
      {
        if (next->key > key)
          break;
        prev = next;
        next = next->next;
        index++;
      }
      // case: middle of the list
      if (next)
      {
        node->next = next->next;
        next->next = node;
      }
      // case: end of the list
      else
      {
        node->next = NULL;
        prev->next = node;
      }
    }
  }

  return index;
}

// search through list and find the first matching key
// return the node
static OS_Keylist NodeByKey(
  OS_Keylist head,// head of the list
  KEY key) // key to find a match
{
  OS_Keylist next = NULL; // return value

  if (head)
  {
    next = head->next;
    while (next != NULL)
    {
      if (next->key == key)
        break;
      next = next->next;
    }
  }

  return next;
}

// search through list and find the node with the
// correct index
static OS_Keylist NodeByIndex(
  OS_Keylist head,// head of the list
  int index) // position in list to find
{
  OS_Keylist next = NULL; // used for return value

  if (head && (index >= 0))
  {
    next = head->next;
    while (next != NULL)
    {
      if (index == 0)
        break;
      index--;
      next = next->next;
    }
  }

  return next;
}

// returns the last node, and removes it from the list
static OS_Keylist NodePop(
  OS_Keylist head)// head of the list
{
  OS_Keylist next = NULL; // used for return value

  if (head)
  {
    next = head->next;
    if (next)
      head->next = next->next;
    else
      head->next = NULL;
  }

  return next;
}

// removes node specified by key on the list of lists
// and returns it
static OS_Keylist NodeRemoveByKey(
  OS_Keylist head,// head of the list
  KEY key) // key to find a match
{
  OS_Keylist next = NULL;// return value
  OS_Keylist prev;

  if (head)
  {
    prev = head;
    next = head->next;
    while (next)
    {
      // found it!
      if (next->key == key)
        break;
      prev = next;
      next = next->next;
    }

    // didn't find it
    if (next != NULL)
    {
      // patch over the link in the list
      prev->next = next->next;
    }
  }

  return next;
}

// removes node specified by key on the list of lists
// and returns it
static OS_Keylist NodeRemoveByIndex(
  OS_Keylist head,// head of the list
  int index) // key to find a match
{
  OS_Keylist next = NULL;// return value
  OS_Keylist prev;

  if (head)
  {
    prev = head;
    next = head->next;
    while (next)
    {
      // found it!
      if (index == 0)
        break;
      index--;
      prev = next;
      next = next->next;
    }

    // didn't find it
    if (next != NULL)
    {
      // patch over the link in the list
      prev->next = next->next;
    }
  }

  return next;
}

// removes node specified by key on the list of lists
// and returns it
static OS_Keylist NodeRemoveByData(
  OS_Keylist head,// head of the list
  void *data) // key to find a match
{
  OS_Keylist next = NULL;// return value
  OS_Keylist prev;

  if (head)
  {
    prev = head;
    next = head->next;
    while (next)
    {
      // found it!
      if (next->data == data)
        break;
      prev = next;
      next = next->next;
    }

    // didn't find it
    if (next != NULL)
    {
      // patch over the link in the list
      prev->next = next->next;
    }
  }

  return next;
}

// returns the number of nodes in the list
static int NodeCount(
  OS_Keylist head)// head of the list
{
  OS_Keylist next;
  int count = 0; // return value

  if (head)
  {
    next = head->next;
    while (next != NULL)
    {
      count++;
      next = next->next;
    }
  }

  return count;
}

/////////////////////////////////////////////////////////////////////
// List of Lists functions
/////////////////////////////////////////////////////////////////////


// returns head of the list or NULL on failure.
OS_Keylist Keylist_Create(void)
{
  // create the new list head
  return NodeCreate();
}

// delete specified list
void Keylist_Delete(
  OS_Keylist list) // list number to be deleted
{
  // FIXME: delete all the members?
  if (list)
    free(list);

  return;
}

/////////////////////////////////////////////////////////////////////
// list functions
/////////////////////////////////////////////////////////////////////

// inserts a node into its sorted position
int Keylist_Data_Add(
  OS_Keylist list,
  KEY key,
  void *data)
{
  return NodeAddByKey(list,key,data);
}

// deletes a node specified by its key
// returns the data from the node
void *Keylist_Data_Delete(
  OS_Keylist list,
  KEY key)
{
  OS_Keylist node;
  void *data = NULL; // return value

  node = NodeRemoveByKey(list,key);
  if (node)
  {
    data = node->data;
    free(node);
  }

  return data;
}

// deletes a node specified by its index
// returns the data from the node
void *Keylist_Data_Delete_By_Index(
  OS_Keylist list,
  int index)
{
  OS_Keylist node;
  void *data = NULL;

  node = NodeRemoveByIndex(list,index);
  if (node)
  {
    data = node->data;
    free(node);
  }

  return data;
}

// deletes a node specified by its index
// returns the data from the node
void *Keylist_Data_Delete_By_Data(
  OS_Keylist list,
  void *data)
{
  OS_Keylist node;

  node = NodeRemoveByData(list,data);
  if (node)
    free(node);

  return data;
}

// returns the data from the node specified by key
void *Keylist_Data(
  OS_Keylist list,
  KEY key)
{
  OS_Keylist node;

  node = NodeByKey(list,key);

  return node ? node->data : NULL;
}

// returns the next empty key from the list
int Keylist_Next_Empty_Key(
  OS_Keylist list,
  KEY key)
{
  return NodeNextKey(list,key);
}

// returns the data specified by key
void *Keylist_Data_Index(
  OS_Keylist list,
  int index)
{
  OS_Keylist node;

  node = NodeByIndex(list,index);

  return node ? node->data : NULL;
}

// returns the data from last node, and removes it from the list
void *Keylist_Data_Pop(
  OS_Keylist list)
{
  OS_Keylist node;

  node = NodePop(list);

  return node ? node->data : NULL;
}

// return the number of nodes in this list
int Keylist_Count(
  OS_Keylist list)
{
  return NodeCount(list);
}

#ifdef TEST
#include <assert.h>
#include <string.h>

#include "ctest.h"

// test the encode and decode macros
void testKeySample(Test* pTest)
{
  int type, id;
  int type_list[] = 
    {0,1,KEY_TYPE_MAX/2,KEY_TYPE_MAX-2,KEY_TYPE_MAX-1,-1};
  int id_list[] = 
    {0,1,KEY_ID_MAX/2,KEY_ID_MAX-2,KEY_ID_MAX-1,-1};
  int type_index = 0;
  int id_index = 0;
  int decoded_type, decoded_id;
  KEY key;

  while (type_list[type_index] != -1)
  {
    while (id_list[id_index] != -1)
    {
      type = type_list[type_index];
      id = id_list[id_index];
      key = KEY_ENCODE(type,id);
      decoded_type = KEY_DECODE_TYPE(key);
      decoded_id = KEY_DECODE_ID(key);
      ct_test(pTest,decoded_type == type);
      ct_test(pTest,decoded_id == id);
      
      id_index++;
    }
    id_index = 0;
    type_index++;
  }

  return;
}

// test the FIFO
void testKeyListPop(Test* pTest)
{
  OS_Keylist list;
  KEY key;
  int index;
  char *data1 = "Joshua";
  char *data2 = "Anna";
  char *data3 = "Mary";
  char *data;

  list = Keylist_Create();
  ct_test(pTest,list != NULL);

  key = 0;
  index = Keylist_Data_Add(list,key,data1);
  ct_test(pTest,index == 0);
  index = Keylist_Data_Add(list,key,data2);
  ct_test(pTest,index == 1);
  index = Keylist_Data_Add(list,key,data3);
  ct_test(pTest,index == 2);

  ct_test(pTest,Keylist_Count(list) == 3);

  data = Keylist_Data_Pop(list);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data1) == 0);
  data = Keylist_Data_Pop(list);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data2) == 0);
  data = Keylist_Data_Pop(list);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);
  data = Keylist_Data_Pop(list);
  ct_test(pTest,data == NULL);
  data = Keylist_Data_Pop(list);
  ct_test(pTest,data == NULL);

  Keylist_Delete(list);

  return;
}

void testKeyListDataKey(Test* pTest)
{
  OS_Keylist list;
  KEY key;
  int index;
  char *data1 = "Joshua";
  char *data2 = "Anna";
  char *data3 = "Mary";
  char *data;

  list = Keylist_Create();
  ct_test(pTest,list != NULL);

  key = 1;
  index = Keylist_Data_Add(list,key,data1);
  ct_test(pTest,index == 0);
  key = 2;
  index = Keylist_Data_Add(list,key,data2);
  ct_test(pTest,index == 1);
  key = 3;
  index = Keylist_Data_Add(list,key,data3);
  ct_test(pTest,index == 2);

  ct_test(pTest,Keylist_Count(list) == 3);

  // look at the data
  data = Keylist_Data(list,2);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data2) == 0);

  data = Keylist_Data(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data1) == 0);

  data = Keylist_Data(list,3);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);

  // work the data
  data = Keylist_Data_Delete(list,2);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data2) == 0);

  data = Keylist_Data_Delete(list,2);
  ct_test(pTest,data == NULL);

  ct_test(pTest,Keylist_Count(list) == 2);

  data = Keylist_Data(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data1) == 0);

  data = Keylist_Data(list,3);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);

  // cleanup
  do
  {
    data = Keylist_Data_Pop(list);
  } while (data);

  Keylist_Delete(list);

  return;
}

void testKeyListDataIndex(Test* pTest)
{
  OS_Keylist list;
  KEY key;
  int index;
  char *data1 = "Joshua";
  char *data2 = "Anna";
  char *data3 = "Mary";
  char *data;

  list = Keylist_Create();
  ct_test(pTest,list != NULL);

  key = 0;
  index = Keylist_Data_Add(list,key,data1);
  ct_test(pTest,index == 0);
  index = Keylist_Data_Add(list,key,data2);
  ct_test(pTest,index == 1);
  index = Keylist_Data_Add(list,key,data3);
  ct_test(pTest,index == 2);


  ct_test(pTest,Keylist_Count(list) == 3);

  // look at the data
  data = Keylist_Data_Index(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data2) == 0);

  data = Keylist_Data_Index(list,0);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data1) == 0);

  data = Keylist_Data_Index(list,2);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);

  // work the data
  data = Keylist_Data_Delete_By_Index(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data2) == 0);

  ct_test(pTest,Keylist_Count(list) == 2);

  data = Keylist_Data_Index(list,0);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data1) == 0);

  data = Keylist_Data_Index(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);

  data = Keylist_Data_Delete_By_Index(list,1);
  ct_test(pTest,data != NULL);
  ct_test(pTest,strcmp(data,data3) == 0);

  data = Keylist_Data_Delete_By_Index(list,1);
  ct_test(pTest,data == NULL);
  
  // cleanup
  do
  {
    data = Keylist_Data_Pop(list);
  } while (data);

  Keylist_Delete(list);

  return;
}

#ifdef TEST_KEYLIST
int main(void)
{
  Test *pTest;
  bool rc;

  pTest = ct_create("keylist", NULL);

  /* individual tests */
  rc = ct_addTestFunction(pTest, testKeyListPop);
  assert(rc);
  rc = ct_addTestFunction(pTest, testKeyListDataKey);
  assert(rc);
  rc = ct_addTestFunction(pTest, testKeySample);
  assert(rc);
  rc = ct_addTestFunction(pTest, testKeyListDataIndex);
  assert(rc);

  ct_setStream(pTest, stdout);
  ct_run(pTest);
  (void)ct_report(pTest);

  ct_destroy(pTest);

  return 0;
}
#endif /* LOCAL_TEST */
#endif
