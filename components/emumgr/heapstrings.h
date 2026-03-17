#pragma once

#define MAX_ITEM_LEN (256)  // max length of a entry in PSRAM string list

int symbol_rank(char c);
void clear_item_buffer(char* buffer);
char* get_item(int item, char* buffer);
int get_item_cnt(char* buffer);

char* get_item_aligned(int item, char* buffer);
int get_item_cnt_aligned(char* buffer);
void add_item_aligned(char* item, char* buffer);
bool insert_item_aligned(int itm_no, char* item, char* buffer);

const char terminatorstring = 255;
const char nullstring = 0;
const char fillerstring = 1;
