#include <cstring>
#include "heapstrings.h"

int symbol_rank(char c) {
  if (c >= 97 && c <= 122) {
    // letters (lower case)
    return c - 32;
  }

  return c;
}

void clear_item_buffer(char* buffer) {
  buffer[0] = terminatorstring;

  // clear the rest of the item
  for (int i = 1; i < MAX_ITEM_LEN; i++)
    buffer[i] = 1;
}

char* get_item(int item, char* buffer) {
  int pos = 0;
  int cur_entry = 0;

  while(1) {
    if (item == cur_entry)
      return &buffer[pos];
    if(buffer[pos] == terminatorstring)
      break;
    if (buffer[pos] == nullstring)
      cur_entry++;

    while (buffer[++pos] == fillerstring);
  }

  return NULL;
}

int get_item_cnt(char* buffer) {
  int item_cnt = 0;
  int i = 0;

  //for (int i = 0; i < DIRECTORY_BUFFER_SIZE; i++) { // TODO: fix limits
  while (1) {

    if (buffer[i] == terminatorstring)
      return item_cnt;
    if (buffer[i] == nullstring)
      item_cnt++;
    
    i++;

  }

  return 0;
}

char* get_item_aligned(int item, char* buffer) {
  return &buffer[MAX_ITEM_LEN * item];
}

int get_item_cnt_aligned(char* buffer) {
  int item_cnt = 0;

  //for (int i = 0; i < DIRECTORY_BUFFER_SIZE; i++) { // TODO: fix limits
  while (1) {
    if (buffer[item_cnt * MAX_ITEM_LEN] == terminatorstring)
      break;
  
    item_cnt++;
  }

  return item_cnt;
}

void add_item_aligned(char* item, char* buffer) {
  char* buf_dst = &buffer[get_item_cnt_aligned(buffer) * MAX_ITEM_LEN];
  strcpy(buf_dst, item);

  // clear the rest of the item
  for (int i = strlen(item) + 1; i < MAX_ITEM_LEN; i++)
    buf_dst[i] = fillerstring;

  buf_dst[MAX_ITEM_LEN] = terminatorstring;
}

bool insert_item_aligned(int itm_no, char* item, char* buffer) {
  int itm_cnt = get_item_cnt_aligned(buffer);
  char* buf_tmp;

  for (int i = itm_cnt; i > itm_no; i--)
    memcpy(get_item_aligned(i, buffer), get_item_aligned(i - 1, buffer), MAX_ITEM_LEN);

  buf_tmp = get_item_aligned(itm_no, buffer);
  memcpy(buf_tmp, item, strlen(item) + 1);

  for (int i = strlen(item) + 1; i < MAX_ITEM_LEN; i++)
    buf_tmp[i] = fillerstring;

  buffer[MAX_ITEM_LEN * (itm_cnt + 1)] = terminatorstring;

  return true;
}